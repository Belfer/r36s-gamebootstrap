#pragma once

#include <types.hpp>
#include <cmath>

struct vec2
{
	vec2() : data{ 0, 0 } {}
	vec2(f32 x, f32 y) : data{ x, y } {}
	vec2(const vec2& v) : data{ v.x, v.y } {}
	union { struct { f32 x, y; }; f32 data[2]; };
	inline vec2 operator+(const vec2& v) const { return { x + v.x, y + v.y }; }
	inline vec2 operator-(const vec2& v) const { return { x - v.x, y - v.y }; }
	inline vec2 operator*(f32 v) const { return { x * v, y * v }; }
	inline vec2 operator/(f32 v) const { return *this * (1.f / v); }
	inline vec2& operator+=(const vec2& v) { *this = *this + v; return *this; }
	inline vec2& operator-=(const vec2& v) { *this = *this - v; return *this; }
	inline vec2& operator*=(f32 v) { *this = *this * v; return *this; }
	inline vec2& operator/=(f32 v) { *this = *this / v; return *this; }
};

struct vec3
{
	vec3() : data{ 0, 0, 0 } {}
	vec3(f32 x, f32 y, f32 z) : data{ x, y, z } {}
	vec3(const vec3& v) : data{ v.x, v.y, v.z } {}
	union { struct { f32 x, y, z; }; f32 data[3]; };
	inline vec3 operator+(const vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
	inline vec3 operator-(const vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
	inline vec3 operator*(f32 v) const { return { x * v, y * v, z * v }; }
	inline vec3 operator/(f32 v) const { return *this * (1.f / v); }
	inline vec3& operator+=(const vec3& v) { *this = *this + v; return *this; }
	inline vec3& operator-=(const vec3& v) { *this = *this - v; return *this; }
	inline vec3& operator*=(f32 v) { *this = *this * v; return *this; }
	inline vec3& operator/=(f32 v) { *this = *this / v; return *this; }
};

struct vec4
{
	vec4() : data{ 0, 0, 0, 0 } {}
	vec4(f32 x, f32 y, f32 z, f32 w) : data{ x, y, z, w } {}
	vec4(const vec4& v) : data{ v.x, v.y, v.z, v.w } {}
	union { struct { f32 x, y, z, w; }; f32 data[4]; };
	inline vec4 operator+(const vec4& v) const { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
	inline vec4 operator-(const vec4& v) const { return { x - v.x, y - v.y, z - v.z, w + v.w }; }
	inline vec4 operator*(f32 v) const { return { x * v, y * v, z * v, w * v }; }
	inline vec4 operator/(f32 v) const { return *this * (1.f / v); }
	inline vec4& operator+=(const vec4& v) { *this = *this + v; return *this; }
	inline vec4& operator-=(const vec4& v) { *this = *this - v; return *this; }
	inline vec4& operator*=(f32 v) { *this = *this * v; return *this; }
	inline vec4& operator/=(f32 v) { *this = *this / v; return *this; }
};

struct mat2
{
	mat2() {}
	mat2(const vec2& x, const vec2& y) : data{ x, y } {}
	mat2(const mat2& m) : data{ m.data[0], m.data[1] } {}
	vec2 data[2];
	inline mat2 operator+(const mat2& m) const { return { data[0] + m.data[0], data[1] + m.data[1] }; }
	inline mat2 operator-(const mat2& m) const { return { data[0] - m.data[0], data[1] - m.data[1] }; }
	inline mat2 operator*(f32 v) const { return { data[0] * v, data[1] * v }; }
	inline mat2 operator/(f32 v) const { return *this * (1.f / v); }
	inline mat2& operator+=(const mat2& v) { *this = *this + v; return *this; }
	inline mat2& operator-=(const mat2& v) { *this = *this - v; return *this; }
	inline mat2& operator*=(f32 v) { *this = *this * v; return *this; }
	inline mat2& operator/=(f32 v) { *this = *this / v; return *this; }
};

struct mat3
{
	mat3() {}
	mat3(const vec3& x, const vec3& y, const vec3& z) : data{ x, y, z } {}
	mat3(const mat3& m) : data{ m.data[0], m.data[1], m.data[2] } {}
	vec3 data[3];
	inline mat3 operator+(const mat3& m) const { return { data[0] + m.data[0], data[1] + m.data[1], data[2] + m.data[2] }; }
	inline mat3 operator-(const mat3& m) const { return { data[0] - m.data[0], data[1] - m.data[1], data[2] - m.data[2] }; }
	inline mat3 operator*(f32 v) const { return { data[0] * v, data[1] * v, data[2] * v }; }
	inline mat3 operator/(f32 v) const { return *this * (1.f / v); }
	inline mat3& operator+=(const mat3& v) { *this = *this + v; return *this; }
	inline mat3& operator-=(const mat3& v) { *this = *this - v; return *this; }
	inline mat3& operator*=(f32 v) { *this = *this * v; return *this; }
	inline mat3& operator/=(f32 v) { *this = *this / v; return *this; }
};

struct mat4
{
	mat4() {}
	mat4(const vec4& x, const vec4& y, const vec4& z, const vec4& w) : data{ x, y, z, w } {}
	mat4(const mat4& m) : data{ m.data[0], m.data[1], m.data[2], m.data[3] } {}
	vec4 data[4];
	inline mat4 operator+(const mat4& m) const { return { data[0] + m.data[0], data[1] + m.data[1], data[2] + m.data[2], data[3] + m.data[3] }; }
	inline mat4 operator-(const mat4& m) const { return { data[0] - m.data[0], data[1] - m.data[1], data[2] - m.data[2], data[3] - m.data[3] }; }
	inline mat4 operator*(f32 v) const { return { data[0] * v, data[1] * v, data[2] * v, data[3] * v }; }
	inline mat4 operator/(f32 v) const { return *this * (1.f / v); }
	inline mat4& operator+=(const mat4& v) { *this = *this + v; return *this; }
	inline mat4& operator-=(const mat4& v) { *this = *this - v; return *this; }
	inline mat4& operator*=(f32 v) { *this = *this * v; return *this; }
	inline mat4& operator/=(f32 v) { *this = *this / v; return *this; }
};

inline f32 eps() { return 1e-6f; }

inline bool is_zero(f32 v) { return abs(v) < eps(); }
inline bool is_zero(const vec2& v) { return is_zero(v.x) && is_zero(v.y); }
inline bool is_zero(const vec3& v) { return is_zero(v.x) && is_zero(v.y) && is_zero(v.z); }
inline bool is_zero(const vec4& v) { return is_zero(v.x) && is_zero(v.y) && is_zero(v.z) && is_zero(v.w); }
inline bool is_zero(const mat2& m) { return is_zero(m.data[0]) && is_zero(m.data[1]); }
inline bool is_zero(const mat3& m) { return is_zero(m.data[0]) && is_zero(m.data[1]) && is_zero(m.data[2]); }
inline bool is_zero(const mat4& m) { return is_zero(m.data[0]) && is_zero(m.data[1]) && is_zero(m.data[2]) && is_zero(m.data[3]); }

inline f32 clamp(f32 v, f32 min, f32 max) { return fmin(fmax(v, min), max); }
inline vec2 clamp(const vec2& v, f32 min, f32 max) { return { clamp(v.x, min, max), clamp(v.y, min, max) }; }
inline vec3 clamp(const vec3& v, f32 min, f32 max) { return { clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max) }; }
inline vec4 clamp(const vec4& v, f32 min, f32 max) { return { clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max), clamp(v.w, min, max) }; }

inline f32 dot(const vec2& a, const vec2& b) { return a.x * b.x + a.y * b.y; }
inline f32 dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline f32 dot(const vec4& a, const vec4& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

inline f32 length(const vec2& v) { return sqrt(dot(v, v)); }
inline f32 length(const vec3& v) { return sqrt(dot(v, v)); }
inline f32 length(const vec4& v) { return sqrt(dot(v, v)); }

inline vec2 normalized(const vec2& v) { return v / length(v); }
inline vec3 normalized(const vec3& v) { return v / length(v); }
inline vec4 normalized(const vec4& v) { return v / length(v); }

inline vec3 cross(const vec3& a, const vec3& b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }