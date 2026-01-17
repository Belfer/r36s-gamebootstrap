#pragma once

#include <math.hpp>
#include <batch.hpp>
#include <ringbuff.hpp>

struct particle_t
{
	f32 life{ 0.f };
	vec3 pos{};
	vec3 vel{};
	vec3 size{};
	vec3 rot{};
	vec4 col{};
	vec3 force{};
};

enum struct particle_spawn_shape_t
{
	CIRCLE, LINE, RECT, SPHERE, HEMISPHERE, CONE, DONUT, BOX
};

struct emitter_t
{
	emitter_t(u32 max_particles, GLuint tex);
	~emitter_t();

	void update(f32 dt);
	void draw(const mat4& mvp, const mat4& view);

	u32 seed{ 0 };
	vec3 origin{ 0.f, 0.f, 0.f };

	f32 duration{ 1.f };
	bool looping{ true };
	bool prewarm{ false };
	f32 start_delay{ 0.f };
	f32 start_lifetime{ 1.f };
	
	bool use_random_vel{ false };
	bool use_vel_curve{ false };
	vec3 start_vel{ 0.f, 0.f, 0.f };
	vec3 end_vel{ 0.f, 0.f, 0.f };
	cubic_bezier_t vel_curve{};

	bool use_random_size{ false };
	bool use_size_curve{ false };
	vec3 start_size{ 1.f, 1.f, 1.f };
	vec3 end_size{ 1.f, 1.f, 1.f };
	cubic_bezier_t size_curve{};

	bool use_random_rotation{ false };
	bool use_rotation_curve{ false };
	vec3 start_rotation{ 0.f, 0.f, 0.f };
	vec3 end_rotation{ 0.f, 0.f, 0.f };
	cubic_bezier_t rotation_curve{};
	
	bool use_random_color{ false };
	bool use_color_curve{ false };
	vec4 start_color{ 1.f, 1.f, 1.f, 1.f };
	vec4 end_color{ 1.f, 1.f, 1.f, 1.f };
	cubic_bezier_t color_curve{};

	bool use_random_force{ false };
	bool use_force_curve{ false };
	vec3 start_force{ 0.0f, -9.8f, 0.0f };
	vec3 end_force{ 0.f, -9.8f, 0.f };
	cubic_bezier_t force_curve{};

	f32 spawn_time_rate{ 1.f };
	f32 spawn_distance_rate{ 0.f };
	f32 spawn_burst_time{ 0.f };
	u32 spawn_burst_count{ 0 };
	u32 spawn_burst_cycles{ 0 };
	f32 spawn_burst_interval{ 0.f };
	f32 spawn_burst_probability{ 0.f };

	particle_spawn_shape_t spawn_shape{};
	vec3 spawn_size{ 1.f, 1.f, 1.f };

private:
	batch_t batch;
	ring_buff_t<particle_t> particles;
	u32 max_particles{ 0 };
	GLuint tex{ 0 };
	GLuint vao{ 0 };
	GLuint prg{ 0 };

	f32 timer{ 0.f };
	f32 spawn_timer{ 0.f };
};