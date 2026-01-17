#include <device.hpp>
#include <particles.hpp>

#define DEG2RAD 0.017453292519943295f

vec3 direction_from_yaw_pitch(f32 yaw, f32 pitch)
{
    const f32 cy = cosf(yaw);
    const f32 sy = sinf(yaw);
    const f32 cp = cosf(pitch);
    const f32 sp = sinf(pitch);
    return normalized({ cy * cp, sp, -sy * cp });
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
    config.audio_callback = [](i16*, i32, void*) {};
    config.audio_userdata = nullptr;

    if (!init(config))
        return EXIT_FAILURE;

    GLuint tex = load_texture(ASSETS_PATH"/particles/star_08.png");

    emitter_t emitter{ 5000, tex };
    
    emitter.spawn_time_rate = 0.001f;

    emitter.spawn_size = vec3{ 5.f, 5.f, 5.f };

    emitter.use_size_curve = true;
    emitter.size_curve = cubic_bezier_t::ease(vec2{ 0.0f, 0.0f }, vec2{ 1.0f, 1.0f });
    emitter.start_size = vec3{ 2.f, 2.f, 2.f };
    emitter.end_size = vec3{ 0.2f, 0.2f, 0.2f };

    emitter.use_rotation_curve = true;
    emitter.rotation_curve = cubic_bezier_t::ease(vec2{ 0.0f, 0.0f }, vec2{ 1.0f, 1.0f });
    emitter.start_rotation = vec3{ 0.f, 0.f, 0.f };
    emitter.end_rotation = vec3{ 0.f, 0.f, 10.f };

    emitter.use_color_curve = true;
    emitter.color_curve = cubic_bezier_t::ease(vec2{ 0.0f, 0.0f }, vec2{ 1.0f, 1.0f });
    emitter.start_color = vec4{ 0.1f, 0.5f, 0.8f, 1.f };
    emitter.end_color = vec4{ 0.4f, 0.05f, 0.25f, 0.f };

    i32 w, h;
    screen_size(&w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    f32 camdist = 10.f;
    vec2 camrot{ 90.f, 0.f };
    vec3 camoff{ 0.f, 0.f, 0.f };
    mat4 proj = perspective(DEG2RAD * 60.f, (f32)w / h, 0.1f, 100.f);

    f32 time = 0.f;
    f32 fps_timer = 0.f;
    i32 fps_frames = 0;

    f64 ts = get_time();
    while (begin_frame())
    {
        if (is_button_pressed(GP_BTN_START)) close();
        const f32 dt = (f32)(get_time() - ts); ts = get_time();

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
        vec3 camdir = direction_from_yaw_pitch(DEG2RAD * camrot.x, DEG2RAD * camrot.y);
        vec3 campos = camoff + camdir * camdist;
        mat4 view = lookat(campos, -camdir, vec3{ 0.f, 1.f, 0.f });
        mat4 mvp = proj * view;

        //emitter.origin = vec3{ campos.x, emitter.origin.y, campos.z };
        emitter.update(dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        emitter.draw(mvp, view);

        end_frame();
    }

    glDeleteTextures(1, &tex);

    shutdown();
    return EXIT_SUCCESS;
}