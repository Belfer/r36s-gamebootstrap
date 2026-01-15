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