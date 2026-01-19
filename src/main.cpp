#include <device.hpp>
#include <batch.hpp>

struct vertex_t { vec2 pos{}; u32 col{ 0 }; };

#define TRAIL_SIZE 0.01f
#define TRAIL_LENGTH 20
#define MAX_TRAILS 500

struct trail_t
{
    f32 seg_length{ 0.01f };
    vec2 pos{};

    u32 cur{ 0 };
    vec2 trail[TRAIL_LENGTH];

    u32 vert_count{ 0 };
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

        vert_count = 0;
        for (u32 i = 0; i < TRAIL_LENGTH; i++)
        {
            u32 idx = (cur + i + 1) % TRAIL_LENGTH;
            u32 prev_idx = (idx == 0) ? TRAIL_LENGTH - 1 : idx - 1;

            const vec2 a2b = trail[idx] - trail[prev_idx];
            if (!near_zero(dot(a2b, a2b)))
            {
                auto& va = vertices[vert_count];
                auto& vb = vertices[vert_count + 1];
                vert_count += 2;

                f32 t = (f32)i / (f32)(TRAIL_LENGTH - 1);
                u32 alpha = (u32)(t * 255.f);
                u32 col = (alpha << 24) | 0x00FFFFFF;
                va.col = vb.col = col;

                const vec2 dir = normalized(a2b);
                va.pos = trail[idx] + vec2{ dir.y, -dir.x } * TRAIL_SIZE * t;
                vb.pos = trail[idx] + vec2{ -dir.y, dir.x } * TRAIL_SIZE * t;
            }
        }

        u32 idx = cur;
        u32 prev_idx = (idx == 0) ? TRAIL_LENGTH - 1 : idx - 1;
        const vec2 a2b = trail[idx] - trail[prev_idx];
        if (dot(a2b, a2b) >= seg_length * seg_length)
        {
            cur = (cur + 1) % TRAIL_LENGTH;
        }
    }
};

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
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool init = true;
    bool freeze = false;

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

        if (time > 5.f)
            freeze = true;

        if (!freeze)
        {
            f32 speed = 1.0f;
            f32 min_amp = 0.2f;
            f32 max_amp = 0.8f;
            f32 offset_x = 0.05f;
            f32 offset_y = 0.07f;
            for (u32 i = 0; i < MAX_TRAILS; i++)
            {
                auto& trail = trails[i];

                f32 phase = f32(i) * 1.3f;
                f32 shape_x = 0.8f + 0.2f * f32(i % 5);
                f32 shape_y = 0.6f + 0.25f * f32(i % 7);
                f32 amp_x = min_amp + fmod(f32(i) * 0.13f, max_amp - min_amp);
                f32 amp_y = min_amp + fmod(f32(i) * 0.17f, max_amp - min_amp);
                f32 t = time * speed;

                f32 x = cos(t + phase) * amp_x
                    + sin(t * 1.5f + phase * 2.0f) * offset_x * shape_x;
                f32 y = sin(t + phase) * amp_y
                    + cos(t * 1.2f + phase * 1.5f) * offset_y * shape_y;

                f32 origin_bias = std::sin(t * 0.2f + i) * 0.1f;
                x *= (1.0f - origin_bias);
                y *= (1.0f - origin_bias);
                trail.pos.x = x;
                trail.pos.y = y;

                if (init)
                    trail.init(trail.pos);

                trail.update(dt);
            }
        }

        if (init)
            init = false;

        for (u32 i = 0; i < MAX_TRAILS; i++)
        {
            auto& trail = trails[i];
            batch.begin();
            batch.add_vertices((u8*)&trail.vertices[0], trail.vert_count * sizeof(vertex_t));
            batch.end();
        }

        //batch.begin();
        //vertex_t vertices[4];
        //vertices[0].pos = vec2{ -.5f, -.5f };
        //vertices[0].col = 0x0FFFFFFF;
        //vertices[1].pos = vec2{ -.5f, .5f };
        //vertices[1].col = 0x0FFFFFFF;
        //vertices[2].pos = vec2{ .5f, -.5f };
        //vertices[2].col = 0x0FFFFFFF;
        //vertices[3].pos = vec2{ .5f, .5f };
        //vertices[3].col = 0x0FFFFFFF;
        //batch.add_vertices((u8*)&vertices[0], sizeof(vertices));
        //batch.end();

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