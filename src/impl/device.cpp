#include <device.hpp>

static GLuint compile_shader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(shader, 512, nullptr, buf);
        LOG_ERROR("Shader compile failed: %s", buf);
    }

    return shader;
}

GLuint create_program(const char* vsrc, const char* fsrc)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vsrc);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
    GLuint prog = glCreateProgram();

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(prog, 512, nullptr, buf);
        LOG_ERROR("Program link failed: %s", buf);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

GLuint create_buffer(GLenum type, GLenum usage, GLsizei size, void* data)
{
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(type, vbo);
    glBufferData(type, size, data, usage);
    glBindBuffer(type, 0);
    return vbo;
}

mat4 orthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far)
{
    f32 dx = right - left;
    f32 dy = top - bottom;
    f32 dz = far - near;

    return mat4{
        vec4{ 2.f / dx, 0.f,    0.f,    0.f },
        vec4{ 0.f,    2.f / dy, 0.f,    0.f },
        vec4{ 0.f,    0.f,   -2.f / dz, 0.f },
        vec4{ -(right + left) / dx, -(top + bottom) / dy, -(far + near) / dz, 1.f }
    };
}