#pragma once

#include <math.hpp>

bool sky_init();
void sky_shutdown();

struct sky_t
{
	sky_t();
	~sky_t();

	void update(f32 dt);
	void draw(const mat4& ivp) const;

	vec3 sun_dir{ 0.f, 0.8f, 0.6f };
	vec3 sun_color{ 1.0f, 0.97f, 0.85f };
	f32 sun_size = 0.015f;
	vec3 sky_color_top{ 0.1f, 0.4f, 0.9f };
	vec3 sky_color_horizon{ 0.6f, 0.7f, 0.9f };
	f32 horizon_strength{ 1.0f };
	f32 cloud_scale{ 2.0f };
	f32 cloud_time{ 0.f };
};