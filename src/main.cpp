#include <device.hpp>
#include <batch.hpp>

struct vertex_t { vec2 pos{}; u32 col{ 0 }; };

#define TRAIL_SIZE 0.005f
#define TRAIL_LENGTH 100
struct trail_t
{
    u32 cur{ 0 };
    vec2 pos{};
    vec2 trail[TRAIL_LENGTH];
    vertex_t vertices[TRAIL_LENGTH * 2]{};

    void init(const vec2& start)
    {
        pos = start;
        for (uint32_t i = 0; i < TRAIL_LENGTH; i++)
            trail[i] = start;
    }

    void update(f32 dt)
    {
        trail[cur] = pos;

        for (u32 i = 0; i < TRAIL_LENGTH; i++)
        {
            u32 idx = (cur + i + 1) % TRAIL_LENGTH;
            u32 prev_idx = (idx == 0) ? TRAIL_LENGTH - 1 : idx - 1;

            u32 v_idx = 2 * i;
            auto& vb = vertices[v_idx];
            auto& va = vertices[v_idx + 1];

            f32 t = (f32)(i - 1) / (f32)TRAIL_LENGTH;
            u32 alpha = (u32)(t * 255.f);
            u32 col = (alpha << 24) | 0x00FFFFFF;
            va.col = vb.col = col;

            const vec2 a2b = trail[idx] - trail[prev_idx];
            if (!is_zero(dot(a2b, a2b)))
            {
                const vec2 dir = normalized(a2b);
                va.pos = trail[idx] + vec2{ -dir.y, dir.x } * TRAIL_SIZE * t;
                vb.pos = trail[idx] + vec2{ dir.y,-dir.x } * TRAIL_SIZE * t;
            }
            else
            {
                va.pos = trail[idx];
                vb.pos = trail[idx];
            }
        }

        cur = (cur + 1) % TRAIL_LENGTH;
    }
};

#define MAX_TRAILS 10
static trail_t trails[MAX_TRAILS];

static const char* vsrc = R"(
#version 100
attribute vec2 aPos;
attribute vec4 aCol;
uniform mat4 uMVP;
varying vec4 vCol;
void main() {
	gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
	vCol = aCol;
})";

static const char* fsrc = R"(
#version 100
precision mediump float;
varying vec4 vCol;
void main() {
	gl_FragColor = vCol;
})";

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

    batch_t batch{ sizeof(vertex_t), MAX_TRAILS * TRAIL_LENGTH * 2, MAX_TRAILS, 2 };

    GLuint vao;
    glGenVertexArraysX(1, &vao);
    glBindVertexArrayX(vao);

    glBindBuffer(GL_ARRAY_BUFFER, batch.get_vbo());

    GLuint prg = create_program(vsrc, fsrc);
    const GLint apos = glGetAttribLocation(prg, "aPos");
    const GLint acol = glGetAttribLocation(prg, "aCol");
    GLint umvp = glGetUniformLocation(prg, "uMVP");

    glEnableVertexAttribArray(apos);
    glVertexAttribPointer(apos, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)0);
    glEnableVertexAttribArray(acol);
    glVertexAttribPointer(acol, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_t), (void*)(2 * sizeof(f32)));

    glBindVertexArrayX(0);
    
    i32 w, h;
    screen_size(&w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool init = true;

    f32 time = 0.f;
    f32 fps_timer = 0.f;
    i32 fps_frames = 0;

    f64 last_time = get_time();
	while (begin_frame())
	{
        if (is_button_pressed(GP_BTN_START)) close();

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

        for (u32 i = 0; i < MAX_TRAILS; i++)
        {
            auto& trail = trails[i];

            f32 speed = 5.0f + 0.2f;
            f32 freq_x = 1.0f + 0.3f * i;
            f32 freq_y = 0.8f + 0.25f * i;
            f32 phase = i * 1.5f;

            f32 amp_x = 0.3f + 0.1f * (i % 3);
            f32 amp_y = 0.3f + 0.1f * ((i + 1) % 3);

            trail.pos.x = cos(time * speed * freq_x + phase) * amp_x
                + sin(time * speed * freq_x * 1.5f + phase * 2.0f) * 0.05f;

            trail.pos.y = sin(time * speed * freq_y + phase) * amp_y
                + cos(time * speed * freq_y * 1.2f + phase * 1.5f) * 0.05f;

            if (init) trail.init(trail.pos);
            trail.update(dt);

            batch.begin();
            batch.add_vertices((u8*)&trail.vertices[0], sizeof(trail.vertices));
            batch.end();
        }
        if (init) init = false;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        const mat4 mvp{{ 1,0,0,0 }, { 0,1,0,0 }, { 0,0,1,0 }, { 0,0,0,1 }};
        glBindVertexArrayX(vao);
        glUseProgram(prg);
        glUniformMatrix4fv(umvp, 1, GL_FALSE, &mvp.data[0].x);

        batch.submit();
        batch.draw(GL_TRIANGLE_STRIP);
        batch.clear();

        glUseProgram(0);
        glBindVertexArrayX(0);

		end_frame();
	}

    glDeleteProgram(prg);
    glDeleteVertexArraysX(1, &vao);

	shutdown();
	return 0;
}