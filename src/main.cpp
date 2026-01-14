#include <device.hpp>

#define MAX_VERTICES 1024
struct vertex_t { vec2 pos; u32 col; };
static vertex_t vertices[MAX_VERTICES];
static u32 start_vert_idx = 0;
static u32 end_vert_idx = 0;

static u32 vertex_count()
{
    return end_vert_idx >= start_vert_idx
        ? end_vert_idx - start_vert_idx
        : MAX_VERTICES - start_vert_idx + end_vert_idx;
}

static bool check_buffer_size(u32 n)
{
    return vertex_count() + n > MAX_VERTICES;
}

static void add_vertex(const vec2& p, u32 c)
{
    if (vertex_count() == MAX_VERTICES) return;
    vertices[end_vert_idx].pos = p;
    vertices[end_vert_idx].col = c;
    end_vert_idx = (end_vert_idx + 1) % MAX_VERTICES;
}

static void consume_vertices(u32 n)
{
    start_vert_idx = (start_vert_idx + n) % MAX_VERTICES;
}

static void add_line(const vec2& a, const vec2& b, u32 c)
{
    add_vertex(a, c);
    add_vertex(b, c);
}

static void draw_rect(vec2 min, vec2 max, u32 col)
{
    add_line({ min.x, min.y }, { max.x, min.y }, col);
    add_line({ max.x, min.y }, { max.x, max.y }, col);
    add_line({ max.x, max.y }, { min.x, max.y }, col);
    add_line({ min.x, max.y }, { min.x, min.y }, col);
}

static void draw_grid(vec2 min, vec2 max, i32 divisions, u32 col,
    vec2(*mapping_fn)(const vec2&))
{
    vec2 size = { max.x - min.x, max.y - min.y };
    vec2 half = { size.x * 0.5f, size.y * 0.5f };
    vec2 center = { min.x + half.x, min.y + half.y };

    for (i32 i = 1; i < divisions; ++i)
    {
        f32 t = -1.0f + 2.0f * (f32)i / (f32)divisions;

        vec2 px1 = mapping_fn({ t, -1.f });
        vec2 px2 = mapping_fn({ t,  1.f });
        add_line(
            { center.x + px1.x * half.x, center.y + px1.y * half.y },
            { center.x + px2.x * half.x, center.y + px2.y * half.y },
            col
        );

        vec2 py1 = mapping_fn({ -1.f, t });
        vec2 py2 = mapping_fn({ 1.f, t });
        add_line(
            { center.x + py1.x * half.x, center.y + py1.y * half.y },
            { center.x + py2.x * half.x, center.y + py2.y * half.y },
            col
        );
    }
}


static void draw_marker(vec2 center, f32 r, u32 col)
{
    draw_rect(
        { center.x - r, center.y - r },
        { center.x + r, center.y + r },
        col
    );
}

static vec2 map_square_to_circle(const vec2& v)
{
    return
    {
        v.x * sqrtf(1.0f - 0.5f * v.y * v.y),
        v.y * sqrtf(1.0f - 0.5f * v.x * v.x)
    };
}

static vec2 input_to_square(vec2 input, vec2 min, vec2 max)
{
    vec2 half = { (max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f };
    vec2 center = { min.x + half.x, min.y + half.y };

    return {
        center.x + input.x * half.x,
        center.y + input.y * half.y
    };
}

static const char* vsrc = R"(
#version 100
attribute vec2 aPos;
attribute vec4 aColor;
uniform vec2 uSize;
varying vec4 vColor;
void main() {
    gl_Position = vec4((aPos - uSize * 0.5) / uSize, 0.0, 1.0);
    vColor = aColor;
})";

static const char* fsrc = R"(
#version 100
precision mediump float;
varying vec4 vColor;
void main() {
    gl_FragColor = vColor;
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
    config.audio_callback = [](i16*, i32, void*) {};
    config.audio_userdata = nullptr;

    if (!init(config))
        return -1;

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint program = create_program(vsrc, fsrc);
    GLint apos_loc = glGetAttribLocation(program, "aPos");
    GLint acolor_loc = glGetAttribLocation(program, "aColor");
    GLint usize_loc = glGetUniformLocation(program, "uSize");

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), nullptr, GL_STREAM_DRAW);

    glEnableVertexAttribArray(apos_loc);
    glVertexAttribPointer(apos_loc, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)0);
    glEnableVertexAttribArray(acolor_loc);
    glVertexAttribPointer(acolor_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_t), (void*)(2 * sizeof(GLfloat)));

    glBindVertexArray(0);

    i32 w, h;
    screen_size(&w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Layout
    const u32 col_border = 0xFFFFFFFF;
    const u32 col_grid_raw = 0xFFFF0000;
    const u32 col_grid_mapped = 0xFF00FF00;
    const u32 col_raw_marker = 0xFFFF4444;
    const u32 col_map_marker = 0xFF44AAFF;

    const f32 padding = 40.0f;
    const f32 size = fminf(w, h) - padding * 2.0f;

    const vec2 sq_min{ (w - size) * 0.5f, (h - size) * 0.5f };
    const vec2 sq_max{ sq_min.x + size, sq_min.y + size };

    while (begin_frame())
    {
        if (is_button_pressed(GP_BTN_START)) close();

        vec2 ljoy{ get_axis_value(GP_AXIS_LX), -get_axis_value(GP_AXIS_LY) };
        vec2 rjoy{ get_axis_value(GP_AXIS_RX), get_axis_value(GP_AXIS_RY) };

        vec2 raw = clamp(ljoy, -1.f, 1.f);
        vec2 mapped = map_square_to_circle(raw);

        draw_rect(sq_min, sq_max, col_border);
        draw_grid(sq_min, sq_max, 10, col_grid_raw, [](const vec2& v) { return v; });
        draw_grid(sq_min, sq_max, 10, col_grid_mapped, [](const vec2& v) { return map_square_to_circle(v); });

        vec2 raw_pos = input_to_square(raw, sq_min, sq_max);
        vec2 mapped_pos = input_to_square(mapped, sq_min, sq_max);

        const f32 marker_size = 5.0f;
        draw_marker(raw_pos, marker_size, col_raw_marker);
        draw_marker(mapped_pos, marker_size, col_map_marker);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        const u32 count = vertex_count();
        if (count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), nullptr, GL_STREAM_DRAW);

            if (start_vert_idx < end_vert_idx)
            {
                glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(vertex_t), &vertices[start_vert_idx]);
            }
            else
            {
                const u32 first_part = MAX_VERTICES - start_vert_idx;
                glBufferSubData(GL_ARRAY_BUFFER, 0, first_part * sizeof(vertex_t), &vertices[start_vert_idx]);
                glBufferSubData(GL_ARRAY_BUFFER, first_part * sizeof(vertex_t), end_vert_idx * sizeof(vertex_t), &vertices[0]);
            }

            glUseProgram(program);
            glUniform2f(usize_loc, w, h);
            glBindVertexArray(vao);
            glDrawArrays(GL_LINES, 0, count);
            glBindVertexArray(0);

            consume_vertices(count);
        }

        end_frame();
    }

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);

    shutdown();
    return 0;
}