#pragma once

#include <device.hpp>

bool debug_init();
void debug_shutdown();

void debug_begin();
void debug_point(const vec3& a, u32 color);
void debug_line(const vec3& a, const vec3& b, u32 color);
void debug_tri(const vec3& a, const vec3& b, const vec3& c, u32 color);
void debug_end();

void debug_draw(const mat4& mvp);