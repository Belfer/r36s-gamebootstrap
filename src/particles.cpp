#include <particles.hpp>

GLuint vao;
GLuint vbo;
GLuint prg;

GLint apos_loc;
GLint acol_loc;
GLint umvp_loc;
GLint utex_loc;

ring_buff_t<particle_t::vertex_t, MAX_PARTICLES * 2> vertices{};

static const char* vsrc = R"(
#version 100
attribute vec4 aPos;
attribute vec4 aCol;
uniform mat4 uMVP;
varying vec4 vCol;
void main() {
    gl_Position = uMVP * vec4(aPos.xyz, 1.0);
    gl_PointSize = 100.0 / gl_Position.w;
    vCol = aCol;
    vCol.a = (1.0 - aPos.w);
})";

static const char* fsrc = R"(
#version 100
precision mediump float;
uniform sampler2D uTex;
varying vec4 vCol;
void main() {
    gl_FragColor = vCol * texture2D(uTex, gl_PointCoord);
})";

bool particles_init()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    prg = create_program(vsrc, fsrc);
    apos_loc = glGetAttribLocation(prg, "aPos");
    acol_loc = glGetAttribLocation(prg, "aCol");
    umvp_loc = glGetUniformLocation(prg, "uMVP");
    utex_loc = glGetUniformLocation(prg, "uTex");

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), nullptr, GL_STREAM_DRAW);

    const GLsizei stride = sizeof(particle_t::vertex_t);
    glEnableVertexAttribArray(apos_loc);
    glVertexAttribPointer(apos_loc, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(acol_loc);
    glVertexAttribPointer(acol_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)(4 * sizeof(f32)));

    glBindVertexArray(0);

    return true;
}

void particles_shutdown()
{
    glDeleteProgram(prg);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

emitter_t::emitter_t(const vec3& origin, f32 spawn_rate, GLuint texture)
    : origin(origin), spawn_rate(spawn_rate), texture(texture)
{
    particles = new ring_buff_t<particle_t, MAX_PARTICLES>{};
}

emitter_t::~emitter_t()
{
    delete particles;
}

void emitter_t::update(f32 dt)
{
    timer += dt;
    while (timer >= spawn_rate)
    {
        timer = fmod(timer, spawn_rate);

        particle_t p{};
        vec3 pos = vec3{ randf() * 2.f - 1.f, 0.f, randf() * 2.f - 1.f };
        pos *= 5.f;
        pos.y = randf();
        p.pos = origin + pos;
        p.vel = randdir() * randf() * 5.f;
        p.col = 0xFFE2D971;
        p.time = 0.f;
        particles->add(p);
    }

    u32 count = 0;
    for (u32 i = particles->start; i < particles->start + particles->count(); i++)
    {
        particle_t& p = particles->data[i % particles->size()];

        particle_t::vertex_t v{};
        v.pos = vec4(p.pos.x, p.pos.y, p.pos.z, p.time / lifetime);
        v.col = p.col;
        vertices.add(v);

        p.vel += gravity * dt;
        p.pos += p.vel * dt;
        p.time += dt;
        if (p.time >= lifetime)
            count++;
    }
    particles->consume(count);
}

void emitter_t::draw(const mat4& mvp)
{
    f32 range[2]{ 1.f, 1000.f };
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, range);

    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    const u32 count = vertices.count();
    if (count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices.data), nullptr, GL_STREAM_DRAW);

        const u32 vert_size = sizeof(particle_t::vertex_t);
        if (vertices.start < vertices.end)
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, count * vert_size, &vertices.data[vertices.start]);
        }
        else
        {
            const u32 first_part = vertices.size() - vertices.start;
            glBufferSubData(GL_ARRAY_BUFFER, 0, first_part * vert_size, &vertices.data[vertices.start]);
            glBufferSubData(GL_ARRAY_BUFFER, first_part * vert_size, vertices.end * vert_size, &vertices.data[0]);
        }

        glUseProgram(prg);
        glUniformMatrix4fv(umvp_loc, 1, GL_FALSE, (f32*)&mvp);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(utex_loc, 0);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, count);
        glBindVertexArray(0);

        vertices.consume(count);
    }
}