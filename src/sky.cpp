#include <sky.hpp>
#include <device.hpp>

static GLuint vao{ 0 };
static GLuint vbo{ 0 };
static GLuint prg{ 0 };
static GLuint tex{ 0 };

static GLint apos_loc{ 0 };
static GLint uivp_loc{ 0 };
static GLint usundir_loc{ 0 };
static GLint usuncol_loc{ 0 };
static GLint usunsize_loc{ 0 };
static GLint uskytop_loc{ 0 };
static GLint uskyhor_loc{ 0 };
static GLint uhorstr_loc{ 0 };
static GLint ucloudscl_loc{ 0 };
static GLint ucloudt_loc{ 0 };

static const char* vsrc = R"(
#version 100
attribute vec2 aPos;
uniform mat4 uIVP;
varying vec3 vDir;
void main() {
    vec4 clip = vec4(aPos.xy, 1.0, 1.0);
    vec4 world = uIVP * clip;
    vDir = normalize(world.xyz / world.w);
    gl_Position = vec4(aPos, 0.0, 1.0);
})";

static const char* fsrc = R"(
#version 100
precision highp float;
varying vec3 vDir;

uniform vec3 uSunDir;       // normalized sun direction
uniform vec3 uSunColor;     // sun color
uniform float uSunSize;     // angular radius (radians)

// Sky colors
uniform vec3 uSkyTop;       // zenith color
uniform vec3 uSkyHorizon;   // horizon color

// Horizon haze (Mie scattering)
uniform float uHorizonStrength;

// Optional clouds
uniform float uCloudScale;
uniform float uCloudTime;

// Simple 2D noise for clouds
float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898,78.233)))*43758.5453); }
float noise(vec2 p) { 
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash(i); float b = hash(i + vec2(1.0,0.0));
    float c = hash(i + vec2(0.0,1.0)); float d = hash(i + vec2(1.0,1.0));
    vec2 u = f*f*(3.0-2.0*f);
    return mix(a,b,u.x) + (c-a)*u.y*(1.0-u.x) + (d-b)*u.x*u.y;
}

void main() {
    vec3 dir = normalize(vDir);

    // Rayleigh gradient
    float t = clamp(dir.y*0.5 + 0.5, 0.0, 1.0);
    vec3 sky = mix(uSkyHorizon, uSkyTop, pow(t, 0.5));

    // Sun disk
    float sunDot = dot(dir, normalize(uSunDir));
    float sunLobe = smoothstep(cos(uSunSize), 1.0, sunDot);
    vec3 sun = uSunColor * sunLobe;

    // Horizon haze (Mie)
    float mie = pow(1.0 - dir.y, 4.0) * uHorizonStrength; 
    sky = mix(sky, vec3(1.0,0.9,0.8), mie);

    // Day/night factor based on sun height
    float sunHeight = clamp(uSunDir.y*0.5 + 0.5, 0.0, 1.0);
    sky *= sunHeight;
    sun *= sunHeight;

    // Clouds
    //float cloudMask = clamp(dir.y, 0.0, 1.0);  // 0 at horizon, 1 at zenith
    //vec2 cloudUV = dir.xz * uCloudScale + vec2(uCloudTime * 0.05);
    //float clouds = smoothstep(0.4, 0.6, noise(cloudUV) + 0.2 * noise(cloudUV * 2.0));
    //clouds *= cloudMask;
    //sky = mix(sky, vec3(1.0), clouds * 0.5);

    // Combine sun + sky
    vec3 color = sky + sun;

    // Tone mapping
    color = 1.0 - exp(-color);

    gl_FragColor = vec4(color, 1.0);
})";

bool sky_init()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    prg = create_program(vsrc, fsrc);
    apos_loc = glGetAttribLocation(prg, "aPos");
    uivp_loc = glGetUniformLocation(prg, "uIVP");
    usundir_loc = glGetUniformLocation(prg, "uSunDir");
    usuncol_loc = glGetUniformLocation(prg, "uSunColor");
    usunsize_loc = glGetUniformLocation(prg, "uSunSize");
    uskytop_loc = glGetUniformLocation(prg, "uSkyTop");
    uskyhor_loc = glGetUniformLocation(prg, "uSkyHorizon");
    uhorstr_loc = glGetUniformLocation(prg, "uHorizonStrength");
    ucloudscl_loc = glGetUniformLocation(prg, "uCloudScale");
    ucloudt_loc = glGetUniformLocation(prg, "uCloudTime");

    static const f32 vertices[12] = { -1.f, -1.f, 1.f, -1.f, -1.f,  1.f, -1.f,  1.f, 1.f, -1.f, 1.f,  1.f };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(apos_loc);
    glVertexAttribPointer(apos_loc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

    glBindVertexArray(0);

    tex = load_texture(ASSETS_PATH"/common/Noise/perlin-noise.png");

    return true;
}

void sky_shutdown()
{
    glDeleteProgram(prg);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &tex);
}

sky_t::sky_t() {}
sky_t::~sky_t() {}

void sky_t::update(f32 dt)
{
    cloud_time += dt;
}

void sky_t::draw(const mat4& ivp) const
{
    glBindVertexArray(vao);
    glUseProgram(prg);

    glUniformMatrix4fv(uivp_loc, 1, GL_FALSE, &ivp.data[0].x);
    glUniform3fv(usundir_loc, 1, &sun_dir.x);
    glUniform3fv(usuncol_loc, 1, &sun_color.x);
    glUniform1f(usunsize_loc, sun_size);
    glUniform3fv(uskytop_loc, 1, &sky_color_top.x);
    glUniform3fv(uskyhor_loc, 1, &sky_color_horizon.x);
    glUniform1f(uhorstr_loc, horizon_strength);
    glUniform1f(ucloudscl_loc, cloud_scale);
    glUniform1f(ucloudt_loc, cloud_time);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glUseProgram(0);
    glBindVertexArray(0);
}
