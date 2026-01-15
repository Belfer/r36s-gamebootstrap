#include <device.hpp>
#include <debug.hpp>

void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    LOG_WARN("OpenGL Debug Message:\n  Source: 0x%x\n  Type: 0x%x\n  ID: %u\n  Severity: 0x%x\n  Message: %s\n", source, type, id, severity, message);
}

int main(int argc, char** args)
{
    config_t config{};
    config.display_title = "Game";
    config.display_width = 800;
    config.display_height = 600;
    config.display_vsync = true;
    config.audio_sample_rate = 44100;
    config.audio_channels = 2;
    config.audio_frame_count = 256;
    config.audio_callback = [](i16*, i32, void*){};
    config.audio_userdata = nullptr;

	if (!init(config) || !debug_init())
		return -1;

    LOG_INFO("GL Vendor: %s", glGetString(GL_VENDOR));
    LOG_INFO("GL Renderer: %s", glGetString(GL_RENDERER));
    LOG_INFO("GL Version: %s", glGetString(GL_VERSION));
    LOG_INFO("GL Shading Language Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    GLint extensionCount = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
    LOG_INFO("GL Extensions:");
    for (GLint i = 0; i < extensionCount; ++i)
        LOG_INFO("  %s", (const char*)glGetStringi(GL_EXTENSIONS, i));

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

    f64 last_time = get_time();
	while (begin_frame())
	{
        f64 curr_time = get_time();
        f32 dt = (f32)(curr_time - last_time);
        last_time = curr_time;

        time += dt;
        fps_timer += dt;
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

        f32 t = sin(time);
        debug_begin();
        debug_point({ .0f, .5f*t, .0f}, 0xFF0000FF);
        debug_point({ .0f, -.5f*t, .0f }, 0xFF0000FF);
        debug_line({ -.5f*t, -.5f*t, .0f }, { .5f*t, .5f*t, .0f }, 0xFF00FF00);
        debug_line({ .5f*t, -.5f*t, .0f }, { -.5f*t, .5f*t, .0f }, 0xFF00FF00);
        debug_tri({ .0f, .5f*t, .0f }, { -.5f*t, -.5f*t, .0f }, { .5f*t, -.5f*t, .0f }, 0xFFFF0000);
        debug_tri({ .0f, -.5f*t, .0f }, { -.5f*t, .5f*t, .0f }, { .5f*t, .5f*t, .0f }, 0xFFFF0000);
        debug_end();

        const mat4 mvp{{ 1,0,0,0 }, { 0,1,0,0 }, { 0,0,1,0 }, { 0,0,0,1 }};
        debug_draw(mvp);

		end_frame();
	}

    debug_shutdown();
	shutdown();
	return 0;
}