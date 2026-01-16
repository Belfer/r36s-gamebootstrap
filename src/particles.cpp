#include <particles.hpp>
#include <device.hpp>

static const char* vsrc = R"(
#version 100
attribute vec4 aPos;
attribute vec2 aUv;
attribute vec4 aCol;
uniform mat4 uMVP;
varying vec2 vUv;
varying vec4 vCol;
void main() {
    gl_Position = uMVP * vec4(aPos.xyz, 1.0);
    vUv = aUv;
    vCol = aCol;
})";

static const char* fsrc = R"(
#version 100
precision mediump float;
uniform sampler2D uTex;
varying vec2 vUv;
varying vec4 vCol;
void main() {
    gl_FragColor = vCol * texture2D(uTex, vUv);
})";

emitter_t::emitter_t(u32 max_particles, GLuint tex)
    : batch(sizeof(particle_t::vertex_t), max_particles, max_particles, 2)
    , particles(max_particles)
    , max_particles(max_particles)
    , tex(tex)
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, batch.get_vbo());

    prg = create_program(vsrc, fsrc);
    const GLint apos_loc = glGetAttribLocation(prg, "aPos");
    const GLint auv_loc = glGetAttribLocation(prg, "aUv");
    const GLint acol_loc = glGetAttribLocation(prg, "aCol");

    const GLsizei stride = sizeof(particle_t::vertex_t);
    glEnableVertexAttribArray(apos_loc);
    glVertexAttribPointer(apos_loc, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(auv_loc);
    glVertexAttribPointer(auv_loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(f32)));
    glEnableVertexAttribArray(acol_loc);
    glVertexAttribPointer(acol_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)(6 * sizeof(f32)));

    glBindVertexArray(0);
}

emitter_t::~emitter_t()
{
    glDeleteProgram(prg);
    glDeleteVertexArrays(1, &vao);
}

void emitter_t::add_quad(const vec3& pos, const vec3& size, const vec4& col)
{
    const u32 c = vec4_to_rgba(col);
    batch.begin();
    particle_t::vertex_t v[4];
    v[0].pos = vec4{ (-0.5f * size.x) + pos.x, (-0.5f * size.y) + pos.y, pos.z, 1.f};
    v[0].uv = vec2{ 0.f, 0.f };
    v[0].col = c;
    v[1].pos = vec4{ (-0.5f * size.x) + pos.x, (0.5f * size.y) + pos.y, pos.z, 1.f };
    v[1].uv = vec2{ 0.f, 1.f };
    v[1].col = c;
    v[2].pos = vec4{ (0.5f * size.x) + pos.x, (-0.5f * size.y) + pos.y, pos.z, 1.f };
    v[2].uv = vec2{ 1.f, 0.f };
    v[2].col = c;
    v[3].pos = vec4{ (0.5f * size.x) + pos.x, (0.5f * size.y) + pos.y, pos.z, 1.f };
    v[3].uv = vec2{ 1.f, 1.f };
    v[3].col = c;
    batch.add_vertices((u8*)v, sizeof(v));
    batch.end();
}

static vec3 rand_in_sphere()
{
    f32 u = randf(), v = randf(), w = randf();
    f32 r = cbrt(u), z = 1.f - 2.f * v, t = 2.f * pi() * w, s = sqrt(1.f - z * z);
    return { r * s * cos(t), r * s * sin(t), r * z };
}

template <typename T>
static void swap(T& a, T& b)
{
    T tmp = b;
    b = a;
    a = tmp;
}

void emitter_t::update(f32 dt)
{
    u32 dead_count = 0;
    for (u32 i = 0; i < particles.count(); i++)
    {
        const u32 idx = (particles.get_start() + i) % particles.get_capacity();

        auto& p = particles[idx];
        p.life -= dt;
        if (p.life <= 0.f)
        {
            const u32 tidx = (particles.get_start() + dead_count) % particles.get_capacity();
            swap(particles[idx], particles[tidx]);
            dead_count++;
            continue;
        }

        const f32 t = 1.f - (p.life / start_lifetime);

        if (use_color_curve)
            p.col = lerp(start_color, end_color, color_curve.eval(t));

        if (use_rotation_curve)
            p.rot = lerp(start_rotation, end_rotation, rotation_curve.eval(t));

        if (use_size_curve)
            p.size = lerp(start_size, end_size, size_curve.eval(t));

        if (use_force_curve)
            p.force = lerp(start_force, end_force, force_curve.eval(t));

        if (use_vel_curve)
            p.vel = lerp(start_vel, end_vel, vel_curve.eval(t));
        else
            p.vel += p.force * dt;

        p.pos += p.vel * dt;

        add_quad(p.pos, p.size, p.col);
    }
    particles.consume(dead_count);

    if (timer >= duration)
    {
        if (looping)
            timer = fmod(timer, duration);
        else
            return;
    }

    timer += dt;
    spawn_timer += dt;

    while (spawn_timer >= spawn_time_rate)
    {
        spawn_timer -= spawn_time_rate;
        if (particles.count() < max_particles)
        {
            vec3 pos = rand_in_sphere();
            pos = vec3{ pos.x * spawn_size.x, pos.y * spawn_size.y , pos.z * spawn_size.z };

            particle_t p{};
            p.life = start_lifetime;
            p.pos = origin + pos;
            p.vel = start_vel;
            p.size = start_size;
            p.rot = start_rotation;
            p.col = start_color;
            p.force = start_force;

            particles.add(p);
        }
    }
}

void emitter_t::draw(const mat4& mvp)
{
    batch.submit();

    glDisable(GL_CULL_FACE);

    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBindVertexArrayX(vao);
    glUseProgram(prg);
    const GLint umvp_loc = glGetUniformLocation(prg, "uMVP");
    const GLint utex_loc = glGetUniformLocation(prg, "uTex");

    glUniformMatrix4fv(umvp_loc, 1, GL_FALSE, (f32*)&mvp);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(utex_loc, 0);

    batch.draw(GL_TRIANGLE_STRIP);

    glUseProgram(0);
    glBindVertexArrayX(0);

    batch.clear();
}