#include <device.hpp>

void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    LOG_WARN("OpenGL Debug Message:\n  Source: 0x%x\n  Type: 0x%x\n  ID: %u\n  Severity: 0x%x\n  Message: %s\n", source, type, id, severity, message);
}

i32 main(i32 argc, char** args)
{
    config_t config{};
    config.display_title = "Game";
    config.display_width = 800;
    config.display_height = 600;
    config.display_vsync = true;
    config.audio_sample_rate = 44100;
    config.audio_channels = 2;
    config.audio_frame_count = 256;
    config.audio_callback = [](i16*, i32, void*) {};
    config.audio_userdata = nullptr;

    if (!init(config))
        return EXIT_FAILURE;

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_debug_callback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    i32 w, h;
    screen_size(&w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    f32 time = 0.f;
    f32 fps_timer = 0.f;
    i32 fps_frames = 0;

    const f32 min_dt = 1.f / 10.f;
    f64 ts = get_time();
    while (begin_frame())
    {
        if (is_button_pressed(GP_BTN_START)) close();
        const f32 elapsed = (f32)(get_time() - ts); ts = get_time();
        const f32 dt = fmin(elapsed, min_dt);

        time += elapsed;
        fps_timer += elapsed;
        fps_frames++;
        if (fps_timer >= 1.f)
        {
            f32 fps = fps_frames / fps_timer;
            f32 frame_time = fps_timer / fps_frames;
            LOG_INFO("%.2f fps, %.4f s/frame", fps, frame_time);
            fps_timer = fmod(fps_timer, 1.f);
            fps_frames = 0;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        end_frame();
    }

	shutdown();
	return 0;
}