#include <device.hpp>
#include <sky.hpp>

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
    config.audio_callback = [](i16*, i32, void*) {};
    config.audio_userdata = nullptr;

    if (!init(config) || !sky_init())
        return EXIT_FAILURE;

    sky_t sky{};

    i32 w, h;
    screen_size(&w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    vec2 camrot{ 0.f, 0.f };
    vec3 campos{ 0.f, 0.f, 0.f };
    const mat4 proj = perspective(radians(60.f), (f32)w / h, 0.1f, 100.f);

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

        vec2 look{};
        look.x = fabs(get_axis_value(GP_AXIS_RX)) < 0.2f ? 0.f : -get_axis_value(GP_AXIS_RX);
        look.y = fabs(get_axis_value(GP_AXIS_RY)) < 0.2f ? 0.f : -get_axis_value(GP_AXIS_RY);

        vec3 move{};
        move.x = fabs(get_axis_value(GP_AXIS_LX)) < 0.2f ? 0.f : -get_axis_value(GP_AXIS_LX);
        move.z = fabs(get_axis_value(GP_AXIS_LY)) < 0.2f ? 0.f : get_axis_value(GP_AXIS_LY);
        if (is_button_pressed(GP_BTN_L1)) move.y -= 1.f;
        if (is_button_pressed(GP_BTN_R1)) move.y += 1.f;

        camrot += look * dt * 100.f;
        camrot.y = clamp(camrot.y, -89.0f, 89.f);
        const mat4 view = lookat(campos, direction_from_yaw_pitch(radians(camrot.x), radians(camrot.y)), vec3{ 0.f, 1.f, 0.f });
        const vec3 right{ view[0].x, view[0].y, view[0].z };
        const vec3 up{ view[1].x, view[1].y, view[1].z };
        const vec3 forward{ view[2].x, view[2].y, view[2].z };
        campos += (right * move.x) + (up * move.y) + (forward * move.z);
        const mat4 vp = proj * view;
        const mat4 ivp = inverse(vp);

        sky.update(dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        sky.draw(ivp);
        
        end_frame();
    }

    sky_shutdown();
    shutdown();
    return 0;
}