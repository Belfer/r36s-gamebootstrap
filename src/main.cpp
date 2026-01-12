#include <device.hpp>
#include <glad/glad.h>
#include <math.h>

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
    WARN("OpenGL Debug Message:\n  Source: 0x%x\n  Type: 0x%x\n  ID: %u\n  Severity: 0x%x\n  Message: %s\n", source, type, id, severity, message);
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

static GLuint compile_shader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(shader, 512, nullptr, buf);
        ERROR("Shader compile failed: %s", buf);
    }

    return shader;
}

static GLuint create_program(const char* vsrc, const char* fsrc)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vsrc);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
    GLuint prog = glCreateProgram();

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(prog, 512, nullptr, buf);
        ERROR("Program link failed: %s", buf);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

static GLuint create_buffer(GLenum type, GLenum usage, GLsizei size, void* data)
{
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(type, vbo);
    glBufferData(type, size, data, usage);
    return vbo;
}

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
    config.audio_callback = audio_callback;
    config.audio_userdata = &synth;

	if (!device_init(config))
		return -1;

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_debug_callback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    struct vertex_t { f32 pos[2]; u32 col; };
    vertex_t vertices[] = { { {0.f,0.5f}, 0xFF0000FF }, { {-0.5f,-0.5f}, 0xFF00FF00 }, { {0.5f,-0.5f}, 0xFFFF0000 } };
    GLuint vbo = create_buffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, sizeof(vertices), vertices);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
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
    glBindVertexArray(0);

    f32 time = 0.f;
    f32 fps_timer = 0.f;
    i32 fps_frames = 0;

    f32 pos[2] = { 0.f, 0.f };

    f64 last_time = device_time();
	while (device_begin_frame())
	{
        f64 curr_time = device_time();
        f32 elapsed = (f32)(curr_time - last_time);
        last_time = curr_time;

        time += elapsed;
        fps_timer += elapsed;
        fps_frames++;

        if (fps_timer >= 1.f)
        {
            f32 fps = fps_frames / fps_timer;
            f32 frame_time = fps_timer / fps_frames;
            INFO("%.2f fps, %.4f s/frame", fps, frame_time);
            fps_timer = fmod(fps_timer, 1.f);
            fps_frames = 0;
        }

        if (device_button(GP_BTN_A))      INFO("A pressed");
        if (device_button(GP_BTN_B))      INFO("B pressed");
        if (device_button(GP_BTN_X))      INFO("X pressed");
        if (device_button(GP_BTN_Y))      INFO("Y pressed");
        if (device_button(GP_BTN_L1))     INFO("L1 pressed");
        if (device_button(GP_BTN_L2))     INFO("L2 pressed");
        if (device_button(GP_BTN_L3))     INFO("L3 pressed");
        if (device_button(GP_BTN_R1))     INFO("R1 pressed");
        if (device_button(GP_BTN_R2))     INFO("R2 pressed");
        if (device_button(GP_BTN_R3))     INFO("R3 pressed");
        if (device_button(GP_BTN_START))  INFO("START pressed");
        if (device_button(GP_BTN_SELECT)) INFO("SELECT pressed");
        if (device_button(GP_BTN_UP))     INFO("UP pressed");
        if (device_button(GP_BTN_DOWN))   INFO("DOWN pressed");
        if (device_button(GP_BTN_LEFT))   INFO("LEFT pressed");
        if (device_button(GP_BTN_RIGHT))  INFO("RIGHT pressed");

        f32 lx = device_axis(GP_AXIS_LX);
        f32 ly = device_axis(GP_AXIS_LY);
        f32 rx = device_axis(GP_AXIS_RX);
        f32 ry = device_axis(GP_AXIS_RY);

        if (fabs(lx) > 0.1f || fabs(ly) > 0.1f)
            INFO("Left stick: (%.2f, %.2f)", lx, ly);

        if (fabs(rx) > 0.1f || fabs(ry) > 0.1f)
            INFO("Right stick: (%.2f, %.2f)", rx, ry);

        if (device_button(GP_BTN_START))
            device_close();

        pos[0] += lx * elapsed;
        pos[0] = fmin(fmax(pos[0], -1.f), 1.f);
        pos[1] -= ly * elapsed;
        pos[1] = fmin(fmax(pos[1], -1.f), 1.f);

        i32 w, h;
        device_size(&w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glUseProgram(program);
        glUniform1f(utime_loc, time);
        glUniform2f(upos_loc, pos[0], pos[1]);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

		device_end_frame();
	}

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);

	device_shutdown();
	return 0;
}