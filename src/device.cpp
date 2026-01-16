#include <device.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

GLuint load_texture(const char* path)
{
    int w, h, channels;
    stbi_uc* pixels = stbi_load(path, &w, &h, &channels, 4);
    if (!pixels)
    {
        LOG_ERROR("Failed to load texture: %s", path);
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(pixels);
    return tex;
}

mat4 lookat(const vec3& eye, const vec3& forward, const vec3& up)
{
    vec3 f = normalized(forward);
    vec3 r = normalized(cross(f, up));
    vec3 u = cross(r, f);

    return mat4{
        vec4{ r.x,  u.x, -f.x, 0.f },
        vec4{ r.y,  u.y, -f.y, 0.f },
        vec4{ r.z,  u.z, -f.z, 0.f },
        vec4{ -dot(r, eye), -dot(u, eye), dot(f, eye), 1.f }
    };
}

mat4 orthographic(f32 width, f32 height, f32 near, f32 far)
{
    f32 right = width * 0.5f;
    f32 left = -right;
    f32 top = height * 0.5f;
    f32 bottom = -top;

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

mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
    f32 f = 1.f / tanf(fov * 0.5f);
    return mat4{
        vec4{ f / aspect, 0.f, 0.f, 0.f },
        vec4{ 0.f,      f,   0.f, 0.f },
        vec4{ 0.f,      0.f, (far + near) / (near - far), -1.f },
        vec4{ 0.f,      0.f, (2.f * far * near) / (near - far), 0.f }
    };
}