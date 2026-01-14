#pragma once

#include <ringbuff.hpp>

#define MAX_PARTICLES 2048

bool particles_init();
void particles_shutdown();

struct particle_t
{
	vec3 pos{};
	vec3 vel{};
	u32 col{ 0 };
	f32 time{ 0.f };

	struct vertex_t { vec4 pos{}; u32 col{ 0 }; };
};

struct emitter_t
{
	emitter_t(const vec3& origin, f32 spawn_rate, GLuint texture);
	~emitter_t();

	void update(f32 dt);
	void draw(const mat4& mvp);

	vec3 origin{};
	vec3 gravity{ 0.0f, -9.8f, 0.0f };
	f32 spawn_rate{ 0.5f };
	f32 lifetime{ 1.0f };

private:
	f32 timer{ 0.f };
	GLuint texture{ 0 };
	ring_buff_t<particle_t, MAX_PARTICLES>* particles{ nullptr };
};