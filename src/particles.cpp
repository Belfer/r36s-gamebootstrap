#include <particles.hpp>
#include <device.hpp>

struct vertex_t
{
    vec3 pos{};
    vec3 size{};
    vec2 uv{};
    u32 col{ 0 };
};

static const char* vsrc = R"(
#version 100
attribute vec3 aPos;
attribute vec3 aSize;
attribute vec2 aUv;
attribute vec4 aCol;
uniform mat4 uMVP;
uniform mat4 uView;
varying vec2 vUv;
varying vec4 vCol;
void main() {
    vec3 right = vec3(uView[0][0], uView[1][0], uView[2][0]);
    vec3 up = vec3(uView[0][1], uView[1][1], uView[2][1]);
    vec2 co = aUv - 0.5;
    float s = sin(aSize.z);
    float c = cos(aSize.z);
    vec3 offset = (co.x * c - co.y * s) * aSize.x * right + (co.x * s + co.y * c) * aSize.y * up;
    gl_Position = uMVP * vec4(aPos + offset, 1.0);
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

emitter_t::emitter_t(u32 max_particles)
    : batch(sizeof(vertex_t), max_particles, max_particles, 2)
    , particles(max_particles)
    , max_particles(max_particles)
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, batch.get_vbo());

    prg = create_program(vsrc, fsrc);
    const GLint apos_loc = glGetAttribLocation(prg, "aPos");
    const GLint asize_loc = glGetAttribLocation(prg, "aSize");
    const GLint auv_loc = glGetAttribLocation(prg, "aUv");
    const GLint acol_loc = glGetAttribLocation(prg, "aCol");

    const GLsizei stride = sizeof(vertex_t);
    glEnableVertexAttribArray(apos_loc);
    glVertexAttribPointer(apos_loc, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(asize_loc);
    glVertexAttribPointer(asize_loc, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(f32)));
    glEnableVertexAttribArray(auv_loc);
    glVertexAttribPointer(auv_loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(f32)));
    glEnableVertexAttribArray(acol_loc);
    glVertexAttribPointer(acol_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)(8 * sizeof(f32)));

    glBindVertexArray(0);
}

emitter_t::~emitter_t()
{
    glDeleteProgram(prg);
    glDeleteVertexArrays(1, &vao);
}

static void add_quad(batch_t& batch, const particle_t& p)
{
    const u32 c{ vec4_to_rgba(p.col) };
    const vec3 s{ p.size.x, p.size.y, radians(p.rot.z) };
    batch.begin();
    vertex_t v[4]
    {
        { p.pos, s, vec2{ 0.f, 0.f }, c },
        { p.pos, s, vec2{ 0.f, 1.f }, c },
        { p.pos, s, vec2{ 1.f, 0.f }, c },
        { p.pos, s, vec2{ 1.f, 1.f }, c }
    };
    batch.add_vertices((u8*)v, sizeof(v));
    batch.end();
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
            if (idx != tidx) swap(particles[idx], particles[tidx]);
            dead_count++;
            continue;
        }

        const f32 t = 1.f - (p.life / start_lifetime);

        if (use_color_curve)
            p.col = lerp(start_color, end_color, color_curve.eval(t).y);

        if (use_rotation_curve)
            p.rot = lerp(start_rotation, end_rotation, rotation_curve.eval(t).y);

        if (use_size_curve)
            p.size = lerp(start_size, end_size, size_curve.eval(t).y);

        if (use_force_curve)
            p.force = lerp(start_force, end_force, force_curve.eval(t).y);

        if (use_vel_curve)
            p.vel = lerp(start_vel, end_vel, vel_curve.eval(t).y);
        else
            p.vel += p.force * dt;

        p.pos += p.vel * dt;

        add_quad(batch, p);
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

    if (near_zero(spawn_time_rate))
        return;

    spawn_timer += dt;
    while (spawn_timer >= spawn_time_rate)
    {
        spawn_timer -= spawn_time_rate;
        if (particles.count() < max_particles)
        {
            vec3 pos = rand_inside_unit_sphere();
            pos = vec3{ pos.x * spawn_size.x, pos.y * spawn_size.y , pos.z * spawn_size.z };

            particle_t p{};
            p.life = start_lifetime;
            p.pos = origin + pos;

            if (use_random_vel)
                p.vel = lerp(start_vel, end_vel, { randf(), randf(), randf() });
            else
                p.vel = start_vel;
            
            if (use_random_vel)
                p.size = lerp(start_size, end_size, { randf(), randf(), randf() });
            else
                p.size = start_size;

            if (use_random_vel)
                p.rot = lerp(start_rotation, end_rotation, { randf(), randf(), randf() });
            else
                p.rot = start_rotation;

            if (use_random_vel)
                p.col = lerp(start_color, end_color, { randf(), randf(), randf(), randf() });
            else
                p.col = start_color;

            if (use_random_vel)
                p.force = lerp(start_force, end_force, randf());
            else
                p.force = start_force;

            particles.add(p);
        }
    }
}

void emitter_t::draw(const mat4& mvp, const mat4& view)
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
    const GLint uview_loc = glGetUniformLocation(prg, "uView");
    const GLint utex_loc = glGetUniformLocation(prg, "uTex");

    glUniformMatrix4fv(umvp_loc, 1, GL_FALSE, mvp.data_ptr());
    glUniformMatrix4fv(uview_loc, 1, GL_FALSE, view.data_ptr());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(utex_loc, 0);

    batch.draw(GL_TRIANGLE_STRIP);

    glUseProgram(0);
    glBindVertexArrayX(0);

    batch.clear();
}