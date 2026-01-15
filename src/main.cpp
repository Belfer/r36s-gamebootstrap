#include <device.hpp>
#include <debug.hpp>

static void draw_rect(vec2 min, vec2 max, u32 col)
{
    debug_line({ min.x, min.y, 0.f }, { max.x, min.y, 0.f }, col);
    debug_line({ max.x, min.y, 0.f }, { max.x, max.y, 0.f }, col);
    debug_line({ max.x, max.y, 0.f }, { min.x, max.y, 0.f }, col);
    debug_line({ min.x, max.y, 0.f }, { min.x, min.y, 0.f }, col);
}

static void draw_grid(vec2 min, vec2 max, i32 divisions, u32 col, vec2(*mapping_fn)(const vec2&))
{
    vec2 size = { max.x - min.x, max.y - min.y };
    vec2 half = { size.x * 0.5f, size.y * 0.5f };
    vec2 center = { min.x + half.x, min.y + half.y };

    for (i32 i = 1; i < divisions; ++i)
    {
        f32 t = -1.0f + 2.0f * (f32)i / (f32)divisions;

        vec2 px1 = mapping_fn({ t, -1.f });
        vec2 px2 = mapping_fn({ t,  1.f });
        debug_line(
            { center.x + px1.x * half.x, center.y + px1.y * half.y, 0.f },
            { center.x + px2.x * half.x, center.y + px2.y * half.y, 0.f },
            col
        );

        vec2 py1 = mapping_fn({ -1.f, t });
        vec2 py2 = mapping_fn({ 1.f, t });
        debug_line(
            { center.x + py1.x * half.x, center.y + py1.y * half.y, 0.f },
            { center.x + py2.x * half.x, center.y + py2.y * half.y, 0.f },
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

    const mat4 mvp = orthographic(0, w, h, 0, -10.0f, 10.f);
    //const mat4 mvp{ {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1} };

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

        if (is_button_pressed(GP_BTN_START)) close();

        vec2 ljoy{ get_axis_value(GP_AXIS_LX), -get_axis_value(GP_AXIS_LY) };
        vec2 rjoy{ get_axis_value(GP_AXIS_RX), get_axis_value(GP_AXIS_RY) };

        vec2 raw = clamp(ljoy, -1.f, 1.f);
        vec2 mapped = map_square_to_circle(raw);

        debug_begin();
        draw_rect(sq_min, sq_max, col_border);
        draw_grid(sq_min, sq_max, 10, col_grid_raw, [](const vec2& v) { return v; });
        draw_grid(sq_min, sq_max, 10, col_grid_mapped, [](const vec2& v) { return map_square_to_circle(v); });
        
        vec2 raw_pos = input_to_square(raw, sq_min, sq_max);
        vec2 mapped_pos = input_to_square(mapped, sq_min, sq_max);
        
        const f32 marker_size = 5.0f;
        draw_marker(raw_pos, marker_size, col_raw_marker);
        draw_marker(mapped_pos, marker_size, col_map_marker);
        debug_end();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        debug_draw(mvp);

        end_frame();
    }

    debug_shutdown();
	shutdown();
	return 0;
}