#include <device.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <linux/input.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <signal.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

// Timing
static struct timespec start_ts;

// Display
static i32 drm_fd;
static drmModeRes* res;
static drmModeConnector* conn;
static drmModeModeInfo mode;
static drmModeEncoder* enc;
static gbm_device* gbm;
static gbm_surface* surface;
static EGLDisplay egl_display;
static EGLContext context;
static EGLSurface egl_surface;
static struct gbm_bo* previous_bo;
static u32 previous_fb;
static bool should_close;

static void page_flip_handler(i32 fd, u32 frame, u32 sec, u32 usec, u32 crtc_id, void* data)
{
    i32* flip_done = (i32*)data;
    *flip_done = 1;
}

static i32 glad_init();

static bool display_init(bool vsync)
{
    INFO("Opening DRM device...");
    drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    FAIL_CHECK(drm_fd < 0, "open /dev/dri/card0");
    INFO("[OK] DRM fd=%d", drm_fd);

    INFO("Querying DRM resources...");
    res = drmModeGetResources(drm_fd);
    FAIL_CHECK(!res, "drmModeGetResources");
    INFO("[OK] DRM resources found");

    INFO("Searching for connected connector...\n");
    conn = nullptr;
    for (i32 i = 0; i < res->count_connectors; i++)
    {
        conn = drmModeGetConnector(drm_fd, res->connectors[i]);
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0)
            break;
        drmModeFreeConnector(conn);
        conn = nullptr;
    }

    FAIL_CHECK(!conn, "No connected connector");
    mode = conn->modes[0];
    INFO("[OK] Connector %u, mode %dx%d", conn->connector_id, mode.hdisplay, mode.vdisplay);

    INFO("Getting encoder...");
    enc = drmModeGetEncoder(drm_fd, conn->encoder_id);
    FAIL_CHECK(!enc, "drmModeGetEncoder");
    INFO("[OK] Encoder %u, CRTC %u", enc->encoder_id, enc->crtc_id);

    INFO("Creating GBM device...");
    gbm = gbm_create_device(drm_fd);
    FAIL_CHECK(!gbm, "gbm_create_device");
    INFO("[OK] GBM device created");

    INFO("Creating GBM surface...");
    surface = gbm_surface_create(gbm, mode.hdisplay, mode.vdisplay,
        GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    FAIL_CHECK(!surface, "gbm_surface_create");
    INFO("[OK] GBM surface %dx%d", mode.hdisplay, mode.vdisplay);

    INFO("Initializing EGL...");
    egl_display = eglGetDisplay((EGLNativeDisplayType)gbm);
    FAIL_CHECK(egl_display == EGL_NO_DISPLAY, "eglGetDisplay");

    bool eglInit = eglInitialize(egl_display, nullptr, nullptr);
    FAIL_CHECK(!eglInit, "eglInitialize");
    INFO("[OK] EGL initialized");

    INFO("EGL Vendor: %s", eglQueryString(egl_display, EGL_VENDOR));
    INFO("EGL Version: %s", eglQueryString(egl_display, EGL_VERSION));
    INFO("EGL Extensions:\n%s", eglQueryString(egl_display, EGL_EXTENSIONS));

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
    eglChooseConfig(egl_display, cfg, &config, 1, &num);
    INFO("[OK] EGL config chosen (%d configs)", num);

    static const EGLint ctx[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, ctx);
    FAIL_CHECK(context == EGL_NO_CONTEXT, "eglCreateContext");
    INFO("[OK] EGL context created");

    egl_surface = eglCreateWindowSurface(egl_display, config, (EGLNativeWindowType)surface, nullptr);
    FAIL_CHECK(egl_surface == EGL_NO_SURFACE, "eglCreateWindowSurface");

    eglMakeCurrent(egl_display, egl_surface, egl_surface, context);
    INFO("[OK] EGL context made current");

    INFO("GL Vendor: %s", glGetString(GL_VENDOR));
    INFO("GL Renderer: %s", glGetString(GL_RENDERER));
    INFO("GL Version: %s", glGetString(GL_VERSION));
    INFO("GL Shading Language Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    INFO("GL Extensions:\n%s", glGetString(GL_EXTENSIONS));

    eglSwapInterval(egl_display, vsync);

    previous_bo = nullptr;
    previous_fb = 0;

    INFO("Initializing GLAD...");
    if (!glad_init())
        ERROR("Failed to initialize GLAD");
    INFO("[OK] GLAD initialized.");

    should_close = false;
    return true;
}

static void display_shutdown()
{
    if (previous_bo)
    {
        gbm_surface_release_buffer(surface, previous_bo);
        drmModeRmFB(drm_fd, previous_fb);
    }

    eglDestroySurface(egl_display, egl_surface);
    eglDestroyContext(egl_display, context);
    eglTerminate(egl_display);
    gbm_surface_destroy(surface);
    gbm_device_destroy(gbm);
    drmModeFreeEncoder(enc);
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    close(drm_fd);
}

static void display_present()
{
    eglSwapBuffers(egl_display, egl_surface);
    struct gbm_bo* bo = gbm_surface_lock_front_buffer(surface);
    if (bo)
    {
        u32 handle = gbm_bo_get_handle(bo).u32;
        u32 stride = gbm_bo_get_stride(bo);
        u32 fb;

        if (!drmModeAddFB(drm_fd, mode.hdisplay, mode.vdisplay, 24, 32, stride, handle, &fb))
        {
            i32 flip_done = 0;
            drmModePageFlip(drm_fd, enc->crtc_id, fb, DRM_MODE_PAGE_FLIP_EVENT, &flip_done);
            drmEventContext evctx = { DRM_EVENT_CONTEXT_VERSION, nullptr, nullptr, page_flip_handler };

            while (!flip_done)
            {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(drm_fd, &fds);
                select(drm_fd + 1, &fds, nullptr, nullptr, nullptr);
                drmHandleEvent(drm_fd, &evctx);
            }

            if (previous_bo)
            {
                gbm_surface_release_buffer(surface, previous_bo);
                drmModeRmFB(drm_fd, previous_fb);
            }
            previous_bo = bo;
            previous_fb = fb;
        }
        else
        {
            gbm_surface_release_buffer(surface, bo);
        }
    }
}

// Audio
#define PCM_DEVICE "default"
#define PCM_FRAMES 256

static snd_pcm_t* pcm;
static u32 audio_sample_rate;
static i32 audio_channels;
static pthread_t audio_thread;
static config_t::audio_callback_t audio_callback;
static void* audio_userdata;
static volatile sig_atomic_t audio_running = 0;

static void* audio_thread_func(void* arg)
{
    i16* buffer = new i16[PCM_FRAMES * audio_channels];

    while (audio_running)
    {
        audio_callback(buffer, PCM_FRAMES, audio_userdata);

        i32 rc = snd_pcm_writei(pcm, buffer, PCM_FRAMES);
        if (rc < 0)
            snd_pcm_prepare(pcm); // recover from underrun
    }

    snd_pcm_drain(pcm);
    return NULL;
}

static bool audio_init(u32 sample_rate, i32 channels, config_t::audio_callback_t callback, void* userdata)
{
    INFO("Initializing audio...");
    if (callback == nullptr)
        return false;

    if (snd_pcm_open(&pcm, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0) < 0)
        return false;

    snd_pcm_hw_params_t* hw;
    snd_pcm_hw_params_alloca(&hw);

    snd_pcm_hw_params_any(pcm, hw);
    snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, hw, channels);

    snd_pcm_hw_params_set_rate_near(pcm, hw, &sample_rate, 0);

    snd_pcm_uframes_t period = PCM_FRAMES;
    snd_pcm_uframes_t buffer = period * 4;

    snd_pcm_hw_params_set_period_size_near(pcm, hw, &period, 0);
    snd_pcm_hw_params_set_buffer_size_near(pcm, hw, &buffer);

    if (snd_pcm_hw_params(pcm, hw) > 0)
    {
        //fprintf(stderr, "snd_pcm_hw_params: %s\n", snd_strerror(rc));
        snd_pcm_close(pcm);
        pcm = nullptr;
        return false;
    }
    snd_pcm_prepare(pcm);

    audio_sample_rate = sample_rate;
    audio_channels = channels;
    audio_callback = callback;
    audio_userdata = userdata;

    audio_running = 1;
    pthread_create(&audio_thread, nullptr, audio_thread_func, nullptr);

    INFO("[OK] Audio initialized.");
    return true;
}

static void audio_shutdown()
{
    if (!audio_running)
        return;

    audio_running = 0;
    pthread_join(audio_thread, nullptr);

    snd_pcm_close(pcm);
    pcm = nullptr;
}

// Input
static i32 gp_fd;
static bool buttons[GP_BTN_COUNT];
static f32 axes[GP_AXIS_COUNT];

static i32 map_button(i32 code)
{
    switch (code)
    {
    case BTN_EAST:              return GP_BTN_A;
    case BTN_SOUTH:             return GP_BTN_B;
    case BTN_NORTH:             return GP_BTN_X;
    case BTN_WEST:              return GP_BTN_Y;
    case BTN_TL:                return GP_BTN_L1;
    case BTN_TL2:               return GP_BTN_L2;
    case BTN_TRIGGER_HAPPY3:    return GP_BTN_L3;
    case BTN_TR:                return GP_BTN_R1;
    case BTN_TR2:               return GP_BTN_R2;
    case BTN_TRIGGER_HAPPY4:    return GP_BTN_R3;
    case BTN_TRIGGER_HAPPY1:    return GP_BTN_SELECT;
    case BTN_TRIGGER_HAPPY2:    return GP_BTN_START;
    case BTN_TRIGGER_HAPPY5:    return GP_BTN_MODE;
    case BTN_DPAD_UP:           return GP_BTN_UP;
    case BTN_DPAD_DOWN:         return GP_BTN_DOWN;
    case BTN_DPAD_LEFT:         return GP_BTN_LEFT;
    case BTN_DPAD_RIGHT:        return GP_BTN_RIGHT;
    }
    return -1;
}

static i32 map_axis(i32 code)
{
    switch (code)
    {
    case ABS_X:     return GP_AXIS_LX;
    case ABS_Y:     return GP_AXIS_LY;
    case ABS_RX:    return GP_AXIS_RX;
    case ABS_RY:    return GP_AXIS_RY;
    }
    return -1;
}

static f32 normalize_axis(i32 value, i32 min, i32 max)
{
    f32 f = (f32)(value - min) / (f32)(max - min);
    return f * 2.0f - 1.0f;
}

static void input_poll()
{
    struct input_event ev;
    while (read(gp_fd, &ev, sizeof(ev)) > 0)
    {
        if (ev.type == EV_KEY)
        {
            i32 b = map_button(ev.code);
            if (b >= 0)
                buttons[b] = (ev.value != 0);
        }
        else if (ev.type == EV_ABS)
        {
            i32 a = map_axis(ev.code);
            if (a >= 0)
                axes[a] = normalize_axis(ev.value, -1800, 1800);
        }
    }
}

static bool input_init()
{
    INFO("Opening input device...");
    gp_fd = open("/dev/input/event2", O_RDONLY | O_NONBLOCK);
    FAIL_CHECK(gp_fd < 0, "open /dev/input/event2");
    INFO("[OK] Input fd=%d", gp_fd);

    return true;
}

static void input_shutdown()
{
    close(gp_fd);
}

bool device_init(const config_t& config)
{
    if (!display_init(config.display_vsync))
        return false;

    if (!audio_init(config.audio_sample_rate, config.audio_channels, config.audio_callback, config.audio_userdata))
        return false;

    if (!input_init())
        return false;
    
    INFO("[DONE] Device initialize.");
    return true;
}

void device_shutdown()
{
    display_shutdown();
    audio_shutdown();
    input_shutdown();

    INFO("[DONE] Device shutdown.");
}

bool device_begin_frame()
{
    return !should_close;
}

void device_end_frame()
{
    input_poll();
    display_present();
}

void device_close()
{
    should_close = true;
}

f64 device_time()
{
    struct timespec cur_ts;
    clock_gettime(CLOCK_MONOTONIC, &cur_ts);
    return (cur_ts.tv_sec - start_ts.tv_sec) + (cur_ts.tv_nsec - start_ts.tv_nsec) * 1e-9;
}

void device_size(i32* width, i32* height)
{
    *width = mode.hdisplay;
    *height = mode.vdisplay;
}

bool device_button(u8 btn)
{
    return buttons[btn];
}

f32 device_axis(u8 axis)
{
    return axes[axis];
}

#include <glad/glad.h>
static i32 glad_init()
{
    return gladLoadGLES2Loader((GLADloadproc)eglGetProcAddress);
}