#include <device.hpp>

#define glGenVertexArraysX (glGenVertexArrays ? glGenVertexArrays : glGenVertexArraysOES ? glGenVertexArraysOES : nullptr)
#define glBindVertexArrayX (glBindVertexArray ? glBindVertexArray : glBindVertexArrayOES ? glBindVertexArrayOES : nullptr)
#define glDeleteVertexArraysX (glDeleteVertexArrays ? glDeleteVertexArrays : glDeleteVertexArraysOES ? glDeleteVertexArraysOES : nullptr)

#define PI 3.14159265

struct chord_synth_t
{
    f32 phase[4];
    f32 freq[4];
    f32 sample_rate;
    f32 lfo_phase[4];
};

static void audio_callback(i16* samples, i32 frames, void* userdata)
{
    chord_synth_t* s = (chord_synth_t*)userdata;

    const f32 lfo_speeds[4] = { 0.03f, 0.025f, 0.035f, 0.02f };
    const f32 lfo_amount = 0.2f;

    for (i32 i = 0; i < frames; i++)
    {
        f32 value = 0.0f;
        for (i32 n = 0; n < 3; n++)
        {
            f32 phase_offset = sinf(s->lfo_phase[n]) * lfo_amount;
            value += sinf(s->phase[n] + phase_offset);

            s->phase[n] += 2.0f * PI * s->freq[n] / s->sample_rate;
            if (s->phase[n] > 2.0f * PI)
                s->phase[n] -= 2.0f * PI;
        }

        value *= 0.25f;

        i16 sample = (i16)(value * 3000);
        samples[i * 2 + 0] = sample; // L
        samples[i * 2 + 1] = sample; // R
    }
}

void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    LOG_WARN("OpenGL Debug Message:\n  Source: 0x%x\n  Type: 0x%x\n  ID: %u\n  Severity: 0x%x\n  Message: %s\n", source, type, id, severity, message);
}

static const char* vertex_shader_src =
"#version 100\n"
"attribute vec2 aPos;\n"
"attribute vec4 aColor;\n"
"uniform float uTime;\n"
"uniform vec2 uPos;\n"
"varying vec4 vColor;\n"
"void main() {\n"
"  gl_Position = vec4(uPos + aPos * (1.0 + sin(uTime) * 0.1), 0.0, 1.0);\n"
"  vColor = aColor;\n"
"}";

static const char* fragment_shader_src =
"#version 100\n"
"precision mediump float;\n"
"varying vec4 vColor;\n"
"void main() {\n"
"  gl_FragColor = vColor;\n"
"}";

int main(int argc, char** args)
{
    chord_synth_t synth
    {
        {0,0,0,0},
        { 261.63f, 329.63f, 392.00f, 523.25f }, // C4-E4-G4-C5
        44100,
        { 0.05f,0.1f,0.001f,0.5f }
    };

    config_t config{};
    config.display_title = "Game";
    config.display_width = 800;
    config.display_height = 600;
    config.display_vsync = true;
    config.audio_sample_rate = 44100;
    config.audio_channels = 2;
    config.audio_frame_count = 256;
    config.audio_callback = audio_callback;
    config.audio_userdata = &synth;

	if (!init(config))
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

    struct vertex_t { f32 pos[2]; u32 col; };
    vertex_t vertices[] = { { {0.f,0.5f}, 0xFF0000FF }, { {-0.5f,-0.5f}, 0xFF00FF00 }, { {0.5f,-0.5f}, 0xFFFF0000 } };
    GLuint vbo = create_buffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, sizeof(vertices), vertices);

    GLuint vao;
    glGenVertexArraysX(1, &vao);
    glBindVertexArrayX(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    GLuint program = create_program(vertex_shader_src, fragment_shader_src);
    GLint apos_loc = glGetAttribLocation(program, "aPos");
    GLint acolor_loc = glGetAttribLocation(program, "aColor");
    GLint utime_loc = glGetUniformLocation(program, "uTime");
    GLint upos_loc = glGetUniformLocation(program, "uPos");

    glEnableVertexAttribArray(apos_loc);
    glVertexAttribPointer(apos_loc, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)0);
    glEnableVertexAttribArray(acolor_loc);
    glVertexAttribPointer(acolor_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_t), (void*)(2 * sizeof(GLfloat)));
    glBindVertexArrayX(0);

    f32 time = 0.f;
    f32 fps_timer = 0.f;
    i32 fps_frames = 0;

    vec2 pos{ 0.f, 0.f };

    f64 last_time = get_time();
	while (begin_frame())
	{
        if (is_button_pressed(GP_BTN_START)) close();

        f64 curr_time = get_time();
        f32 elapsed = (f32)(curr_time - last_time);
        last_time = curr_time;

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

        if (is_button_pressed(GP_BTN_A))      LOG_INFO("A pressed");
        if (is_button_pressed(GP_BTN_B))      LOG_INFO("B pressed");
        if (is_button_pressed(GP_BTN_X))      LOG_INFO("X pressed");
        if (is_button_pressed(GP_BTN_Y))      LOG_INFO("Y pressed");
        if (is_button_pressed(GP_BTN_L1))     LOG_INFO("L1 pressed");
        if (is_button_pressed(GP_BTN_L2))     LOG_INFO("L2 pressed");
        if (is_button_pressed(GP_BTN_L3))     LOG_INFO("L3 pressed");
        if (is_button_pressed(GP_BTN_R1))     LOG_INFO("R1 pressed");
        if (is_button_pressed(GP_BTN_R2))     LOG_INFO("R2 pressed");
        if (is_button_pressed(GP_BTN_R3))     LOG_INFO("R3 pressed");
        if (is_button_pressed(GP_BTN_START))  LOG_INFO("START pressed");
        if (is_button_pressed(GP_BTN_SELECT)) LOG_INFO("SELECT pressed");
        if (is_button_pressed(GP_BTN_UP))     LOG_INFO("UP pressed");
        if (is_button_pressed(GP_BTN_DOWN))   LOG_INFO("DOWN pressed");
        if (is_button_pressed(GP_BTN_LEFT))   LOG_INFO("LEFT pressed");
        if (is_button_pressed(GP_BTN_RIGHT))  LOG_INFO("RIGHT pressed");

        f32 lx = get_axis_value(GP_AXIS_LX);
        f32 ly = get_axis_value(GP_AXIS_LY);
        f32 rx = get_axis_value(GP_AXIS_RX);
        f32 ry = get_axis_value(GP_AXIS_RY);

        if (fabs(lx) > 0.1f || fabs(ly) > 0.1f)
            LOG_INFO("Left stick: (%.2f, %.2f)", lx, ly);

        if (fabs(rx) > 0.1f || fabs(ry) > 0.1f)
            LOG_INFO("Right stick: (%.2f, %.2f)", rx, ry);

        if (is_button_pressed(GP_BTN_START))
            close();

        vec2 input{ lx * sqrt(1.0f - 0.5f * ly * ly), -ly * sqrt(1.0f - 0.5f * lx * lx) };
        pos = clamp(pos + input * elapsed, -1.f, 1.f);

        i32 w, h;
        screen_size(&w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glUseProgram(program);
        glUniform1f(utime_loc, time);
        glUniform2f(upos_loc, pos.x, pos.y);
        glBindVertexArrayX(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArrayX(0);

		end_frame();
	}

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArraysX(1, &vao);

	shutdown();
	return 0;
}