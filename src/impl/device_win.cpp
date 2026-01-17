#include "device.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <atomic>
#include <vector>
#include <cstring>
#include <cassert>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// Display
static GLFWwindow* display_window = nullptr;

static void glfw_error_callback(int error, const char* description)
{
    LOG_ERROR("GLFW Error %d: %s", error, description);
}

static bool display_init(i32 width, i32 height, const char* title, bool vsync)
{
    LOG_INFO("Initializing GLFW...");
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        LOG_ERROR("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    LOG_INFO("Creating GLFW window...");
    const f32 main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    display_window = glfwCreateWindow((i32)(width * main_scale), (i32)(height * main_scale), title, nullptr, nullptr);
    ASSERT(display_window != nullptr, "Failed to create GLFW window");

    glfwMakeContextCurrent(display_window);
    glfwSwapInterval(vsync ? 1 : 0);

    LOG_INFO("Initializing GLAD...");
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        LOG_ERROR("Failed to initialize GLAD");

    LOG_INFO("GLFW window and OpenGL context initialized successfully.");
    return true;
}

static void display_shutdown()
{
    if (display_window)
    {
        glfwDestroyWindow(display_window);
        display_window = nullptr;
    }
    glfwTerminate();
}

// Audio
#define AUDIO_BUFFERS 3
static u32 audio_sample_rate;
static i32 audio_channels;
static i32 audio_frame_count;
static std::thread audio_thread;
static std::atomic<bool> audio_running(false);
static config_t::audio_callback_t audio_callback;
static void* audio_userdata;

static std::vector<std::vector<i16>> audio_buffers(AUDIO_BUFFERS);
static int audio_current_buffer = 0;
static HWAVEOUT audio_hWaveOut = nullptr;
static WAVEHDR audio_waveHdrs[AUDIO_BUFFERS];

static void audio_thread_func()
{
    while (audio_running)
    {
        // Fill current buffer
        audio_callback(audio_buffers[audio_current_buffer].data(), audio_frame_count, audio_userdata);

        MMRESULT res = waveOutWrite(audio_hWaveOut, &audio_waveHdrs[audio_current_buffer], sizeof(WAVEHDR));
        if (res != MMSYSERR_NOERROR)
        {
            fprintf(stderr, "waveOutWrite error: %d\n", res);
            break;
        }

        // Wait until this buffer is done
        while (!(audio_waveHdrs[audio_current_buffer].dwFlags & WHDR_DONE))
            Sleep(1);

        audio_waveHdrs[audio_current_buffer].dwFlags &= ~WHDR_DONE;

        audio_current_buffer = (audio_current_buffer + 1) % AUDIO_BUFFERS;
    }

    // Cleanup buffers
    waveOutReset(audio_hWaveOut);
    for (auto& hdr : audio_waveHdrs)
        waveOutUnprepareHeader(audio_hWaveOut, &hdr, sizeof(WAVEHDR));
    waveOutClose(audio_hWaveOut);
}

static bool audio_init(u32 sample_rate, i32 channels, i32 frame_count,
    config_t::audio_callback_t cb, void* userdata)
{
    LOG_INFO("Initializing audio subsystem...");
    ASSERT(cb != nullptr, "Audio callback cannot be null");
    if (audio_running) return false;

    audio_sample_rate = sample_rate;
    audio_channels = channels;
    audio_frame_count = frame_count;
    audio_callback = cb;
    audio_userdata = userdata;

    WAVEFORMATEX wfx{};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = channels;
    wfx.nSamplesPerSec = sample_rate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    MMRESULT res = waveOutOpen(&audio_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    ASSERT(res == MMSYSERR_NOERROR, "Failed to open audio device");

    // Allocate and prepare audio buffers
    u32 buffer_size = frame_count * channels;
    for (u8 i = 0; i < AUDIO_BUFFERS; ++i)
    {
        audio_buffers[i].resize(buffer_size);

        memset(&audio_waveHdrs[i], 0, sizeof(WAVEHDR));
        audio_waveHdrs[i].lpData = reinterpret_cast<LPSTR>(audio_buffers[i].data());
        audio_waveHdrs[i].dwBufferLength = buffer_size * sizeof(i16);
        audio_waveHdrs[i].dwFlags = 0;

        res = waveOutPrepareHeader(audio_hWaveOut, &audio_waveHdrs[i], sizeof(WAVEHDR));
        ASSERT(res == MMSYSERR_NOERROR, "Failed to prepare audio header");
    }

    audio_running = true;
    audio_thread = std::thread(audio_thread_func);

    LOG_INFO("Audio subsystem initialized successfully.");
    return true;
}

static void audio_shutdown()
{
    if (!audio_running) return;

    audio_running = false;
    if (audio_thread.joinable())
        audio_thread.join();
}

// Public API
bool init(const config_t& config)
{
    if (!display_init(config.display_width, config.display_height, config.display_title, config.display_vsync))
        return false;

    //if (!audio_init(config.audio_sample_rate, config.audio_channels, config.audio_frame_count,
    //                config.audio_callback, config.audio_userdata))
    //    return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    const f32 main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 3
    io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.
#endif

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(display_window, true);
    ImGui_ImplOpenGL3_Init("#version 100");

    LOG_INFO("Device initialized successfully.");
    return true;
}

void shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    display_shutdown();
    //audio_shutdown();

    LOG_INFO("Device shutdown complete.");
}

bool begin_frame()
{
    if (glfwWindowShouldClose(display_window))
        return false;

    glfwPollEvents();
    //if (glfwGetWindowAttrib(display_window, GLFW_ICONIFIED) != 0)
    //{
    //    ImGui_ImplGlfw_Sleep(10);
    //    continue;
    //}

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    return true;
}

void end_frame()
{
    ImGui::Render();
    i32 display_w, display_h;
    glfwGetFramebufferSize(display_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(display_window);
}

void close()
{
    glfwSetWindowShouldClose(display_window, true);
}

void screen_size(i32* width, i32* height)
{
    glfwGetFramebufferSize(display_window, width, height);
}

f64 get_time()
{
    return glfwGetTime();
}

bool is_button_pressed(u8 btn)
{
    if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
        return false;

    GLFWgamepadstate state;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
        return false;

    switch (btn)
    {
    case GP_BTN_A:      return state.buttons[GLFW_GAMEPAD_BUTTON_A];
    case GP_BTN_B:      return state.buttons[GLFW_GAMEPAD_BUTTON_B];
    case GP_BTN_X:      return state.buttons[GLFW_GAMEPAD_BUTTON_X];
    case GP_BTN_Y:      return state.buttons[GLFW_GAMEPAD_BUTTON_Y];
    case GP_BTN_L1:     return state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
    case GP_BTN_L2:     return state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
    case GP_BTN_L3:     return state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];
    case GP_BTN_R1:     return state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
    case GP_BTN_R2:     return state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
    case GP_BTN_R3:     return state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB];
    case GP_BTN_SELECT: return state.buttons[GLFW_GAMEPAD_BUTTON_BACK];
    case GP_BTN_START:  return state.buttons[GLFW_GAMEPAD_BUTTON_START];
    case GP_BTN_UP:     return state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
    case GP_BTN_DOWN:   return state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
    case GP_BTN_LEFT:   return state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
    case GP_BTN_RIGHT:  return state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
    default: return false;
    }
}

f32 get_axis_value(u8 axis)
{
    if (!glfwJoystickPresent(GLFW_JOYSTICK_1))
        return 0.f;

    i32 count = 0;
    const f32* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);
    if (!axes || axis >= count)
        return 0.f;

    return axes[axis];
}