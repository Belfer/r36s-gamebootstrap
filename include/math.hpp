#pragma once

#include <types.hpp>
#include <cmath>

inline f32 pi() { return 3.1415f; }
inline f32 eps() { return 1e-6f; }
inline bool is_zero(f32 v, f32 e = eps()) { return fabs(v) < e; }
inline f32 clamp(f32 v, f32 min, f32 max) { return fmin(fmax(v, min), max); }
inline f32 lerp(f32 a, f32 b, f32 t) { return a + t * (b - a); }

inline f32 randf() { return (f32)rand() / (f32)RAND_MAX; }

inline f32 radians(f32 v) { return v * 0.017453292519943295f; }

struct vec2
{
	vec2() : data{ 0, 0 } {}
	vec2(f32 x, f32 y) : data{ x, y } {}
	vec2(const vec2& v) : data{ v.x, v.y } {}
	union { struct { f32 x, y; }; f32 data[2]; };
	inline f32& operator[](u8 i) { return data[i]; }
	inline const f32& operator[](u8 i) const { return data[i]; }
	inline vec2 operator+(const vec2& v) const { return { x + v.x, y + v.y }; }
	inline vec2 operator-(const vec2& v) const { return { x - v.x, y - v.y }; }
	inline vec2 operator-() const { return { -x, -y }; }
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
	inline f32& operator[](u8 i) { return data[i]; }
	inline const f32& operator[](u8 i) const { return data[i]; }
	inline vec3 operator+(const vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
	inline vec3 operator-(const vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
	inline vec3 operator-() const { return { -x, -y, -z }; }
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
	inline f32& operator[](u8 i) { return data[i]; }
	inline const f32& operator[](u8 i) const { return data[i]; }
	inline vec4 operator+(const vec4& v) const { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
	inline vec4 operator-(const vec4& v) const { return { x - v.x, y - v.y, z - v.z, w + v.w }; }
	inline vec4 operator-() const { return { -x, -y, -z, -w }; }
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
	inline vec2& operator[](u8 i) { return data[i]; }
	inline const vec2& operator[](u8 i) const { return data[i]; }
	inline mat2 operator+(const mat2& m) const { return { data[0] + m.data[0], data[1] + m.data[1] }; }
	inline mat2 operator-(const mat2& m) const { return { data[0] - m.data[0], data[1] - m.data[1] }; }
	inline mat2 operator*(f32 v) const { return { data[0] * v, data[1] * v }; }
	inline mat2 operator/(f32 v) const { return *this * (1.f / v); }
	inline mat2& operator+=(const mat2& v) { *this = *this + v; return *this; }
	inline mat2& operator-=(const mat2& v) { *this = *this - v; return *this; }
	inline mat2& operator*=(f32 v) { *this = *this * v; return *this; }
	inline mat2& operator/=(f32 v) { *this = *this / v; return *this; }

	inline mat2 operator*(const mat2& m) const
	{
		mat2 result{};
		for (u8 col = 0; col < 2; ++col) {
			for (u8 row = 0; row < 2; ++row) {
				f32 sum = 0.f;
				for (u8 k = 0; k < 2; ++k)
					sum += data[k][row] * m[col][k];
				result[col][row] = sum;
			}
		}
		return result;
	}
	inline mat2& operator*=(const mat2& m) { *this = *this * m; return *this; }

	static mat2 identity() { return mat2{ {1,0}, {0,1} }; }
};

struct mat3
{
	mat3() {}
	mat3(const vec3& x, const vec3& y, const vec3& z) : data{ x, y, z } {}
	mat3(const mat3& m) : data{ m.data[0], m.data[1], m.data[2] } {}
	vec3 data[3];
	inline vec3& operator[](u8 i) { return data[i]; }
	inline const vec3& operator[](u8 i) const { return data[i]; }
	inline mat3 operator+(const mat3& m) const { return { data[0] + m.data[0], data[1] + m.data[1], data[2] + m.data[2] }; }
	inline mat3 operator-(const mat3& m) const { return { data[0] - m.data[0], data[1] - m.data[1], data[2] - m.data[2] }; }
	inline mat3 operator*(f32 v) const { return { data[0] * v, data[1] * v, data[2] * v }; }
	inline mat3 operator/(f32 v) const { return *this * (1.f / v); }
	inline mat3& operator+=(const mat3& v) { *this = *this + v; return *this; }
	inline mat3& operator-=(const mat3& v) { *this = *this - v; return *this; }
	inline mat3& operator*=(f32 v) { *this = *this * v; return *this; }
	inline mat3& operator/=(f32 v) { *this = *this / v; return *this; }

	inline mat3 operator*(const mat3& m) const
	{
		mat3 result{};
		for (u8 col = 0; col < 3; ++col) {
			for (u8 row = 0; row < 3; ++row) {
				f32 sum = 0.f;
				for (u8 k = 0; k < 3; ++k)
					sum += data[k][row] * m[col][k];
				result[col][row] = sum;
			}
		}
		return result;
	}
	inline mat3& operator*=(const mat3& m) { *this = *this * m; return *this; }

	static mat3 identity() { return mat3{ {1,0,0}, {0,1,0}, {0,0,1} }; }
};

struct mat4
{
	mat4() {}
	mat4(const vec4& x, const vec4& y, const vec4& z, const vec4& w) : data{ x, y, z, w } {}
	mat4(const mat4& m) : data{ m.data[0], m.data[1], m.data[2], m.data[3] } {}
	vec4 data[4];
	inline vec4& operator[](u8 i) { return data[i]; }
	inline const vec4& operator[](u8 i) const { return data[i]; }
	inline mat4 operator+(const mat4& m) const { return { data[0] + m.data[0], data[1] + m.data[1], data[2] + m.data[2], data[3] + m.data[3] }; }
	inline mat4 operator-(const mat4& m) const { return { data[0] - m.data[0], data[1] - m.data[1], data[2] - m.data[2], data[3] - m.data[3] }; }
	inline mat4 operator*(f32 v) const { return { data[0] * v, data[1] * v, data[2] * v, data[3] * v }; }
	inline mat4 operator/(f32 v) const { return *this * (1.f / v); }
	inline mat4& operator+=(const mat4& v) { *this = *this + v; return *this; }
	inline mat4& operator-=(const mat4& v) { *this = *this - v; return *this; }
	inline mat4& operator*=(f32 v) { *this = *this * v; return *this; }
	inline mat4& operator/=(f32 v) { *this = *this / v; return *this; }

	inline mat4 operator*(const mat4& m) const
	{
		mat4 result{};
		for (u8 col = 0; col < 4; ++col) {
			for (u8 row = 0; row < 4; ++row) {
				f32 sum = 0.f;
				for (u8 k = 0; k < 4; ++k)
					sum += data[k][row] * m[col][k];
				result[col][row] = sum;
			}
		}
		return result;
	}
	inline mat4& operator*=(const mat4& m) { *this = *this * m; return *this; }

	static mat4 identity() { return mat4{ {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1} }; }
};

struct cubic_bezier_t
{
	cubic_bezier_t() {}
	cubic_bezier_t(const vec2& a1, const vec2& a2, const vec2& b1, const vec2& b2)
		: a1(a1), a2(a2), b1(b1), b2(b2)
	{
	}

	vec2 a1{}, a2{}, b1{}, b2{};

	inline f32 eval(f32 t) const
	{
		t = clamp(t, 0.f, 1.f);
		const f32 u = 1.f - t;
		const f32 tt = t * t;
		const f32 uu = u * u;
		return (uu * u) * a1.y + (3.f * uu * t) * a2.y + (3.f * u * tt) * b1.y + (tt * t) * b2.y;
	};

	static cubic_bezier_t linear(f32 a, f32 b)
	{
		return cubic_bezier_t{ { 0.f, a }, { 1.f / 3.f, a + (b - a) / 3.f }, { 2.f / 3.f, a + 2.f * (b - a) / 3.f }, { 1.f, b } };
	}

	static cubic_bezier_t ease(const vec2& a, const vec2& b)
	{
		return cubic_bezier_t{ { 0.f, 0.f }, a, b, { 1.f, 1.f } };
	}
};

inline bool is_zero(const vec2& v, f32 e = eps()) { return is_zero(v.x, e) && is_zero(v.y, e); }
inline bool is_zero(const vec3& v, f32 e = eps()) { return is_zero(v.x, e) && is_zero(v.y, e) && is_zero(v.z, e); }
inline bool is_zero(const vec4& v, f32 e = eps()) { return is_zero(v.x, e) && is_zero(v.y, e) && is_zero(v.z, e) && is_zero(v.w, e); }
inline bool is_zero(const mat2& m, f32 e = eps()) { return is_zero(m.data[0], e) && is_zero(m.data[1], e); }
inline bool is_zero(const mat3& m, f32 e = eps()) { return is_zero(m.data[0], e) && is_zero(m.data[1], e) && is_zero(m.data[2], e); }
inline bool is_zero(const mat4& m, f32 e = eps()) { return is_zero(m.data[0], e) && is_zero(m.data[1], e) && is_zero(m.data[2], e) && is_zero(m.data[3], e); }

inline vec2 clamp(const vec2& v, f32 min, f32 max) { return { clamp(v.x, min, max), clamp(v.y, min, max) }; }
inline vec3 clamp(const vec3& v, f32 min, f32 max) { return { clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max) }; }
inline vec4 clamp(const vec4& v, f32 min, f32 max) { return { clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max), clamp(v.w, min, max) }; }

inline vec2 lerp(const vec2& a, const vec2& b, f32 t) { return { lerp(a.x, b.x, t), lerp(a.y, b.y, t) }; }
inline vec3 lerp(const vec3& a, const vec3& b, f32 t) { return { lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t) }; }
inline vec4 lerp(const vec4& a, const vec4& b, f32 t) { return { lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t), lerp(a.w, b.w, t) }; }

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

inline vec3 randdir() { return normalized(vec3{ randf() * 2.f - 1.f, randf() * 2.f - 1.f, randf() * 2.f - 1.f }); }

inline vec4 rgba_to_vec4(u32 c) { return vec4{ (c & 0xFF) / 255.f, ((c >> 8) & 0xFF) / 255.f, ((c >> 16) & 0xFF) / 255.f, ((c >> 24) & 0xFF) / 255.f }; }
inline u32 vec4_to_rgba(const vec4& c) { return (u32(clamp(c.w, 0.f, 1.f) * 255.f) << 24) | (u32(clamp(c.z, 0.f, 1.f) * 255.f) << 16) | (u32(clamp(c.y, 0.f, 1.f) * 255.f) << 8) | (u32(clamp(c.x, 0.f, 1.f) * 255.f)); }

mat4 lookat(const vec3& eye, const vec3& forward, const vec3& up);
mat4 orthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);
mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far);
mat4 inverse(const mat4& m);