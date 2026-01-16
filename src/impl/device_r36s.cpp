#include <device.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <linux/input.h>
#include <alsa/asoundlib.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <vector>
#include <thread>
#include <atomic>
#include <cassert>

// Timing
static struct timespec device_start_ts;

// Display
static i32 display_drm_fd;
static drmModeRes* display_drm_res = nullptr;
static drmModeConnector* display_drm_conn = nullptr;
static drmModeModeInfo display_drm_mode;
static drmModeEncoder* display_drm_enc = nullptr;
static gbm_device* display_gbm = nullptr;
static gbm_surface* display_gbm_surface = nullptr;
static EGLDisplay display_egl_display;
static EGLContext display_egl_context;
static EGLSurface display_egl_surface;
static struct gbm_bo* display_gbm_previous_bo = nullptr;
static u32 display_gbm_previous_fb = 0;
static bool display_should_close = false;

static void page_flip_handler(i32, u32, u32, u32, u32, void* data)
{
    auto* flip_done = reinterpret_cast<i32*>(data);
    *flip_done = 1;
}

static bool display_init(bool vsync)
{
    LOG_INFO("Opening DRM device...");
    display_drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    ASSERT(display_drm_fd >= 0, "Failed to open DRM device");

    LOG_INFO("Querying DRM resources...");
    display_drm_res = drmModeGetResources(display_drm_fd);
    ASSERT(display_drm_res != nullptr, "Failed to get DRM resources");

    LOG_INFO("Searching for connected display connector...");
    display_drm_conn = nullptr;
    for (i32 i = 0; i < display_drm_res->count_connectors; ++i)
    {
        display_drm_conn = drmModeGetConnector(display_drm_fd, display_drm_res->connectors[i]);
        if (display_drm_conn && display_drm_conn->connection == DRM_MODE_CONNECTED && display_drm_conn->count_modes > 0)
            break;
        drmModeFreeConnector(display_drm_conn);
        display_drm_conn = nullptr;
    }
    ASSERT(display_drm_conn != nullptr, "No connected connector found");

    display_drm_mode = display_drm_conn->modes[0];
    LOG_INFO("Using connector %u with resolution %dx%d", display_drm_conn->connector_id, display_drm_mode.hdisplay, display_drm_mode.vdisplay);

    LOG_INFO("Getting encoder...");
    display_drm_enc = drmModeGetEncoder(display_drm_fd, display_drm_conn->encoder_id);
    ASSERT(display_drm_enc != nullptr, "Failed to get DRM encoder");

    LOG_INFO("Creating GBM device...");
    display_gbm = gbm_create_device(display_drm_fd);
    ASSERT(display_gbm != nullptr, "Failed to create GBM device");

    LOG_INFO("Creating GBM display_gbm_surface...");
    display_gbm_surface = gbm_surface_create(display_gbm, display_drm_mode.hdisplay, display_drm_mode.vdisplay,
        GBM_FORMAT_XRGB8888,
        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    ASSERT(display_gbm_surface != nullptr, "Failed to create GBM display_gbm_surface");

    LOG_INFO("Initializing EGL...");
    display_egl_display = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(display_gbm));
    ASSERT(display_egl_display != EGL_NO_DISPLAY, "eglGetDisplay failed");

    bool eglInit = eglInitialize(display_egl_display, nullptr, nullptr);
    ASSERT(eglInit, "eglInitialize failed");

    LOG_INFO("EGL initialized: vendor=%s, version=%s",
        eglQueryString(display_egl_display, EGL_VENDOR),
        eglQueryString(display_egl_display, EGL_VERSION));

    static const EGLint cfg[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num;
    eglChooseConfig(display_egl_display, cfg, &config, 1, &num);
    LOG_INFO("EGL config chosen (%d configs available)", num);

    static const EGLint ctx[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    display_egl_context = eglCreateContext(display_egl_display, config, EGL_NO_CONTEXT, ctx);
    ASSERT(display_egl_context != EGL_NO_CONTEXT, "eglCreateContext failed");

    display_egl_surface = eglCreateWindowSurface(display_egl_display, config, reinterpret_cast<EGLNativeWindowType>(display_gbm_surface), nullptr);
    ASSERT(display_egl_surface != EGL_NO_SURFACE, "eglCreateWindowSurface failed");

    eglMakeCurrent(display_egl_display, display_egl_surface, display_egl_surface, display_egl_context);
    LOG_INFO("EGL context made current");

    eglSwapInterval(display_egl_display, vsync ? 1 : 0);

    display_gbm_previous_bo = nullptr;
    display_gbm_previous_fb = 0;

    LOG_INFO("Initializing GLAD...");
    if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(eglGetProcAddress)))
        LOG_ERROR("Failed to initialize GLAD");
    else
        LOG_INFO("GLAD initialized");

    display_should_close = false;
    return true;
}

static void display_shutdown()
{
    if (display_gbm_previous_bo)
    {
        gbm_surface_release_buffer(display_gbm_surface, display_gbm_previous_bo);
        drmModeRmFB(display_drm_fd, display_gbm_previous_fb);
    }

    eglDestroySurface(display_egl_display, display_egl_surface);
    eglDestroyContext(display_egl_display, display_egl_context);
    eglTerminate(display_egl_display);
    gbm_surface_destroy(display_gbm_surface);
    gbm_device_destroy(display_gbm);
    drmModeFreeEncoder(display_drm_enc);
    drmModeFreeConnector(display_drm_conn);
    drmModeFreeResources(display_drm_res);
    close(display_drm_fd);
}

static void display_present()
{
    eglSwapBuffers(display_egl_display, display_egl_surface);
    struct gbm_bo* bo = gbm_surface_lock_front_buffer(display_gbm_surface);
    if (!bo) return;

    u32 handle = gbm_bo_get_handle(bo).u32;
    u32 stride = gbm_bo_get_stride(bo);
    u32 fb;

    if (!display_gbm_previous_bo)
    {
        int ret = drmModeAddFB(display_drm_fd, display_drm_mode.hdisplay, display_drm_mode.vdisplay, 24, 32, stride, handle, &fb);
        ASSERT(ret == 0, "drmModeAddFB failed");

        ret = drmModeSetCrtc(display_drm_fd, display_drm_enc->crtc_id, fb, 0, 0, &display_drm_conn->connector_id, 1, &display_drm_mode);
        ASSERT(ret == 0, "drmModeSetCrtc failed");

        display_gbm_previous_bo = bo;
        display_gbm_previous_fb = fb;
        return;
    }

    if (!drmModeAddFB(display_drm_fd, display_drm_mode.hdisplay, display_drm_mode.vdisplay, 24, 32, stride, handle, &fb))
    {
        i32 flip_done = 0;
        drmModePageFlip(display_drm_fd, display_drm_enc->crtc_id, fb, DRM_MODE_PAGE_FLIP_EVENT, &flip_done);
        drmEventContext evctx = { DRM_EVENT_CONTEXT_VERSION, nullptr, nullptr, page_flip_handler };

        while (!flip_done)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(display_drm_fd, &fds);
            select(display_drm_fd + 1, &fds, nullptr, nullptr, nullptr);
            drmHandleEvent(display_drm_fd, &evctx);
        }

        if (display_gbm_previous_bo)
        {
            gbm_surface_release_buffer(display_gbm_surface, display_gbm_previous_bo);
            drmModeRmFB(display_drm_fd, display_gbm_previous_fb);
        }
        display_gbm_previous_bo = bo;
        display_gbm_previous_fb = fb;
    }
    else
    {
        gbm_surface_release_buffer(display_gbm_surface, bo);
    }
}

// Audio
static snd_pcm_t* pcm = nullptr;
static u32 audio_sample_rate;
static i32 audio_channels;
static i32 audio_frame_count;
static std::thread audio_thread;
static config_t::audio_callback_t audio_callback;
static void* audio_userdata = nullptr;
static std::atomic<bool> audio_running(false);

static void audio_thread_func()
{
    std::vector<i16> buffer(audio_frame_count * audio_channels);

    while (audio_running)
    {
        audio_callback(buffer.data(), audio_frame_count, audio_userdata);
        i32 rc = snd_pcm_writei(pcm, buffer.data(), audio_frame_count);
        if (rc < 0) snd_pcm_prepare(pcm);
    }

    snd_pcm_drain(pcm);
}

static bool audio_init(u32 sample_rate, i32 channels, i32 frame_count,
    config_t::audio_callback_t cb, void* userdata)
{
    LOG_INFO("Initializing audio subsystem...");
    if (cb == nullptr)
    {
        LOG_ERROR("Audio callback is null");
        return false;
    }

    i32 rc = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0)
    {
        LOG_ERROR("Failed to open audio device");
        return false;
    }

    snd_pcm_hw_params_t* hw;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(pcm, hw);
    snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, hw, channels);
    snd_pcm_hw_params_set_rate_near(pcm, hw, &sample_rate, nullptr);

    snd_pcm_uframes_t period = frame_count;
    snd_pcm_uframes_t buffer_size = period * 4;
    snd_pcm_hw_params_set_period_size_near(pcm, hw, &period, nullptr);
    snd_pcm_hw_params_set_buffer_size_near(pcm, hw, &buffer_size);

    rc = snd_pcm_hw_params(pcm, hw);
    ASSERT(rc == 0, "Failed to set audio hardware params");

    snd_pcm_prepare(pcm);

    audio_sample_rate = sample_rate;
    audio_channels = channels;
    audio_frame_count = frame_count;
    audio_callback = cb;
    audio_userdata = userdata;

    audio_running = true;
    audio_thread = std::thread(audio_thread_func);

    LOG_INFO("Audio subsystem initialized");
    return true;
}

static void audio_shutdown()
{
    if (!audio_running) return;

    audio_running = false;
    if (audio_thread.joinable())
        audio_thread.join();

    snd_pcm_close(pcm);
    pcm = nullptr;
}

// Input
static i32 gp_fd;
static bool buttons[GP_BTN_COUNT] = { false };
static f32 axes[GP_AXIS_COUNT] = { 0.0f };

static i32 map_button(i32 code)
{
    switch (code)
    {
    case BTN_EAST: return GP_BTN_A;
    case BTN_SOUTH: return GP_BTN_B;
    case BTN_NORTH: return GP_BTN_X;
    case BTN_WEST: return GP_BTN_Y;
    case BTN_TL: return GP_BTN_L1;
    case BTN_TL2: return GP_BTN_L2;
    case BTN_TRIGGER_HAPPY3: return GP_BTN_L3;
    case BTN_TR: return GP_BTN_R1;
    case BTN_TR2: return GP_BTN_R2;
    case BTN_TRIGGER_HAPPY4: return GP_BTN_R3;
    case BTN_TRIGGER_HAPPY1: return GP_BTN_SELECT;
    case BTN_TRIGGER_HAPPY2: return GP_BTN_START;
    case BTN_TRIGGER_HAPPY5: return GP_BTN_MODE;
    case BTN_DPAD_UP: return GP_BTN_UP;
    case BTN_DPAD_DOWN: return GP_BTN_DOWN;
    case BTN_DPAD_LEFT: return GP_BTN_LEFT;
    case BTN_DPAD_RIGHT: return GP_BTN_RIGHT;
    default: return -1;
    }
}

static i32 map_axis(i32 code)
{
    switch (code)
    {
    case ABS_X: return GP_AXIS_LX;
    case ABS_Y: return GP_AXIS_LY;
    case ABS_RX: return GP_AXIS_RX;
    case ABS_RY: return GP_AXIS_RY;
    default: return -1;
    }
}

static f32 normalize_axis(i32 value, i32 min, i32 max)
{
    return (static_cast<f32>(value - min) / (max - min)) * 2.0f - 1.0f;
}

static void input_poll()
{
    struct input_event ev;
    while (read(gp_fd, &ev, sizeof(ev)) > 0)
    {
        if (ev.type == EV_KEY)
        {
            i32 b = map_button(ev.code);
            if (b >= 0) buttons[b] = (ev.value != 0);
        }
        else if (ev.type == EV_ABS)
        {
            i32 a = map_axis(ev.code);
            if (a >= 0) axes[a] = normalize_axis(ev.value, -1800, 1800);
        }
    }
}

static bool input_init()
{
    LOG_INFO("Opening input device...");
    gp_fd = open("/dev/input/event2", O_RDONLY | O_NONBLOCK);
    if (gp_fd < 0)
    {
        LOG_ERROR("Failed to open input device");
        return false;
    }

    LOG_INFO("Input device initialized");
    return true;
}

static void input_shutdown()
{
    close(gp_fd);
}

// Public API
bool init(const config_t& config)
{
    if (!display_init(config.display_vsync))
    {
        LOG_ERROR("Display initialization failed. A functional display is required for operation.");
        return false;
    }

    if (!audio_init(config.audio_sample_rate, config.audio_channels,
        config.audio_frame_count, config.audio_callback, config.audio_userdata))
        LOG_WARN("Audio initialization failed. Continuing without audio support.");

    if (!input_init())
        LOG_WARN("Input system initialization failed. Continuing without input support.");

    LOG_INFO("Device initialization completed successfully.");
    return true;
}

void shutdown()
{
    display_shutdown();
    audio_shutdown();
    input_shutdown();

    LOG_INFO("Device shutdown complete");
}

bool begin_frame()
{
    return !display_should_close;
}

void end_frame()
{
    input_poll();
    display_present();
}

void close()
{
    display_should_close = true;
}

void screen_size(i32* width, i32* height)
{
    *width = display_drm_mode.hdisplay;
    *height = display_drm_mode.vdisplay;
}

f64 get_time()
{
    struct timespec cur_ts;
    clock_gettime(CLOCK_MONOTONIC, &cur_ts);
    return (cur_ts.tv_sec - device_start_ts.tv_sec) +
        (cur_ts.tv_nsec - device_start_ts.tv_nsec) * 1e-9;
}

bool is_button_pressed(u8 btn)
{
    return buttons[btn];
}

f32 get_axis_value(u8 axis)
{
    return axes[axis];
}