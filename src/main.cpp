#include <device.hpp>
#include <particles.hpp>
#include <vector>
#include <memory>

static std::vector<std::shared_ptr<emitter_t>> emitters{};

static emitter_t* create_base_emitter()
{
    GLuint tex = load_texture(ASSETS_PATH"/particles/star_08.png");
    emitter_t* emitter = new emitter_t{ 5000 };
    emitter->set_texture(tex);
    emitter->spawn_time_rate = 0.1f;
    return emitter;
}

#ifdef IMGUI
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

static char texture_path[256]{ "/particles/star_08.png" };

static void draw_bezier_curve(const char* label, cubic_bezier_t& curve, const ImVec2& size = { 300,150 })
{
    ImGui::Text("%s", label);

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Reserve canvas and make it the active item
    ImGui::InvisibleButton(label, size);
    bool is_active = ImGui::IsItemActive();
    ImVec2 mouse_pos_in_canvas = ImGui::GetIO().MousePos - canvas_pos;

    draw_list->AddRectFilled(canvas_pos, canvas_pos + size, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_pos, canvas_pos + size, IM_COL32(255, 255, 255, 255));

    auto to_canvas = [&](const vec2& p) -> ImVec2 {
        return ImVec2(canvas_pos.x + p.x * size.x, canvas_pos.y + (1.f - p.y) * size.y);
        };

    ImVec2 p0 = to_canvas(curve.a1);
    ImVec2 p1 = to_canvas(curve.a2);
    ImVec2 p2 = to_canvas(curve.b1);
    ImVec2 p3 = to_canvas(curve.b2);

    // Draw curve
    const i32 steps = 50;
    ImVec2 last = p0;
    for (i32 i = 1; i <= steps; i++)
    {
        const f32 t = i / (f32)steps;
        const vec2 p = curve.eval(t);
        ImVec2 pt = to_canvas({ p.x, p.y });
        draw_list->AddLine(last, pt, IM_COL32(0, 255, 0, 255), 2.0f);
        last = pt;
    }

    draw_list->AddLine(p0, p1, IM_COL32(255, 255, 0, 150), 1.0f);
    draw_list->AddLine(p1, p2, IM_COL32(255, 255, 0, 150), 1.0f);
    draw_list->AddLine(p2, p3, IM_COL32(255, 255, 0, 150), 1.0f);

    struct ControlPoint { vec2* point; ImVec2 pos; bool x_fixed; };
    ControlPoint points[] = {
        { &curve.a1, p0, true },
        { &curve.a2, p1, false },
        { &curve.b1, p2, false },
        { &curve.b2, p3, true }
    };

    static i32 active_point = -1; // local to this function instance

    ImGuiIO& io = ImGui::GetIO();

    if (is_active)
    {
        // Pick a point to drag if none active yet
        if (active_point == -1)
        {
            for (i32 i = 0; i < 4; i++)
            {
                ImVec2 delta = io.MousePos - points[i].pos;
                if (delta.x * delta.x + delta.y * delta.y < 100.f) // radius 10px
                {
                    active_point = i;
                    break;
                }
            }
        }

        // Drag the active point
        if (active_point != -1 && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            vec2& p = *points[active_point].point;
            f32 x = points[active_point].x_fixed ? p.x : clamp((io.MousePos.x - canvas_pos.x) / size.x, 0.f, 1.f);
            f32 y = 1.f - clamp((io.MousePos.y - canvas_pos.y) / size.y, 0.f, 1.f);
            p.x = x; p.y = y;
        }
    }

    // Reset if mouse released or moved off this widget
    if (!is_active)
        active_point = -1;

    // Draw points
    for (i32 i = 0; i < 4; i++)
        draw_list->AddCircleFilled(points[i].pos, 5.0f, IM_COL32(255, 0, 0, 255));
}

static void draw_emitter_editor()
{
    ImGui::Begin("Particles VFX");

    ImGui::InputText("Texture Path", texture_path, 256);

    if (ImGui::Button("Add Emitter"))
    {
        emitters.emplace_back(create_base_emitter());
    }

    for (size_t i = 0; i < emitters.size(); ++i)
    {
        auto& e = emitters[i];
        ImGui::PushID(i);
        if (ImGui::CollapsingHeader("Emitter"))
        {
            ImGui::Text("General");
            ImGui::DragFloat3("Origin", &e->origin.x, 0.1f);
            ImGui::DragFloat("Duration", &e->duration, 0.01f);
            ImGui::Checkbox("Looping", &e->looping);
            ImGui::Checkbox("Prewarm", &e->prewarm);
            ImGui::DragFloat("Start Delay", &e->start_delay, 0.01f);
            ImGui::DragFloat("Start Lifetime", &e->start_lifetime, 0.01f);

            if (ImGui::Button("Load Texture"))
            {
                GLuint tex = e->get_texture();
                glDeleteTextures(1, &tex);
                char tbuff[256];
                strcpy(tbuff, ASSETS_PATH);
                strcat(tbuff, texture_path);
                tex = load_texture(tbuff);
                e->set_texture(tex);
            }

            ImGui::Separator();
            ImGui::Text("Velocity");
            ImGui::Checkbox("Random Vel", &e->use_random_vel);
            ImGui::Checkbox("Use Vel Curve", &e->use_vel_curve);
            ImGui::DragFloat3("Start Vel", &e->start_vel.x, 0.1f);
            ImGui::DragFloat3("End Vel", &e->end_vel.x, 0.1f);
            if (e->use_vel_curve) draw_bezier_curve("Velocity Curve", e->vel_curve);

            ImGui::Separator();
            ImGui::Text("Size");
            ImGui::Checkbox("Random Size", &e->use_random_size);
            ImGui::Checkbox("Use Size Curve", &e->use_size_curve);
            ImGui::DragFloat3("Start Size", &e->start_size.x, 0.1f);
            ImGui::DragFloat3("End Size", &e->end_size.x, 0.1f);
            if (e->use_size_curve) draw_bezier_curve("Size Curve", e->size_curve);

            ImGui::Separator();
            ImGui::Text("Rotation");
            ImGui::Checkbox("Random Rotation", &e->use_random_rotation);
            ImGui::Checkbox("Use Rotation Curve", &e->use_rotation_curve);
            ImGui::DragFloat3("Start Rotation", &e->start_rotation.x, 0.1f);
            ImGui::DragFloat3("End Rotation", &e->end_rotation.x, 0.1f);
            if (e->use_rotation_curve) draw_bezier_curve("Rotation Curve", e->rotation_curve);

            ImGui::Separator();
            ImGui::Text("Color");
            ImGui::Checkbox("Random Color", &e->use_random_color);
            ImGui::Checkbox("Use Color Curve", &e->use_color_curve);
            ImGui::ColorEdit4("Start Color", &e->start_color.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            ImGui::ColorEdit4("End Color", &e->end_color.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            if (e->use_color_curve) draw_bezier_curve("Color Curve", e->color_curve);

            ImGui::Separator();
            ImGui::Text("Force");
            ImGui::Checkbox("Random Force", &e->use_random_force);
            ImGui::Checkbox("Use Force Curve", &e->use_force_curve);
            ImGui::DragFloat3("Start Force", &e->start_force.x, 0.1f);
            ImGui::DragFloat3("End Force", &e->end_force.x, 0.1f);
            if (e->use_force_curve) draw_bezier_curve("Force Curve", e->force_curve);

            ImGui::Separator();
            ImGui::Text("Spawn");
            ImGui::DragFloat("Spawn Time Rate", &e->spawn_time_rate, 0.001f);
            ImGui::DragFloat("Spawn Distance Rate", &e->spawn_distance_rate, 0.01f);
            ImGui::DragFloat("Spawn Burst Time", &e->spawn_burst_time, 0.01f);
            ImGui::DragInt("Spawn Burst Count", (i32*)&e->spawn_burst_count);
            ImGui::DragInt("Spawn Burst Cycles", (i32*)&e->spawn_burst_cycles);
            ImGui::DragFloat("Spawn Burst Interval", &e->spawn_burst_interval, 0.01f);
            ImGui::DragFloat("Spawn Burst Probability", &e->spawn_burst_probability, 0.01f);

            ImGui::Separator();
            ImGui::Text("Shape");
            ImGui::DragFloat3("Spawn Size", &e->spawn_size.x, 0.1f);
            // Add more shape controls if needed
        }
        ImGui::PopID();
    }

    ImGui::End();
}
#endif

static vec3 direction_from_yaw_pitch(f32 yaw, f32 pitch)
{
    const f32 cy = cosf(yaw);
    const f32 sy = sinf(yaw);
    const f32 cp = cosf(pitch);
    const f32 sp = sinf(pitch);
    return normalized({ cy * cp, sp, -sy * cp });
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
    config.audio_callback = [](i16*, i32, void*){};
    config.audio_userdata = nullptr;

    if (!init(config))
        return EXIT_FAILURE;

    emitters.emplace_back(create_base_emitter());

    i32 w, h;
    screen_size(&w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    f32 camdist = 10.f;
    vec2 camrot{ 90.f, 0.f };
    vec3 camoff{ 0.f, 0.f, 0.f };
    mat4 proj = perspective(radians(60.f), (f32)w / h, 0.1f, 100.f);

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

        vec2 ljoy{ get_axis_value(GP_AXIS_LX), -get_axis_value(GP_AXIS_LY) };
        vec2 rjoy{ get_axis_value(GP_AXIS_RX), -get_axis_value(GP_AXIS_RY) };
        if (dot(ljoy, ljoy) < 0.1f) ljoy = vec2{};
        if (dot(rjoy, rjoy) < 0.1f) rjoy = vec2{};

        f32 move = 0.f;
        if (is_button_pressed(GP_BTN_L1)) move -= 1.f;
        if (is_button_pressed(GP_BTN_R1)) move += 1.f;

        camoff.y += move * dt * 5.f;
        camoff.y = clamp(camoff.y, -10.f, 10.f);
        camdist -= ljoy.y * dt * 10.f;
        camdist = clamp(camdist, 0.1f, 10.f);
        camrot += rjoy * dt * 100.f;
        camrot.y = clamp(camrot.y, -89.0f, 89.f);
        const vec3 camdir = direction_from_yaw_pitch(radians(camrot.x), radians(camrot.y));
        const vec3 campos = camoff + camdir * camdist;
        const mat4 view = lookat(campos, -camdir, vec3{ 0.f, 1.f, 0.f });
        const mat4 mvp = proj * view;

#ifdef IMGUI
        draw_emitter_editor();
#endif

        for (auto& e : emitters)
            e->update(dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        for (auto& e : emitters)
            e->draw(mvp, view);

        end_frame();
    }

    for (auto& e : emitters)
    {
        GLuint tex = e->get_texture();
        glDeleteTextures(1, &tex);
    }

    shutdown();
    return EXIT_SUCCESS;
}