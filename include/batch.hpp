#pragma once

#include <types.hpp>
#include <glad/glad.h>

struct batch_t
{
	batch_t(u32 prim_size, u32 capacity, u32 max_drawcalls, u32 buffer_count);
	~batch_t();

	// nocopy
	batch_t(const batch_t&) = delete;
	batch_t& operator=(const batch_t&) = delete;

	// nomove
	batch_t(batch_t&&) = delete;
	batch_t& operator=(batch_t&&) = delete;

	void begin();
	void add_vertices(u8* data, u32 size);
	void end();

	void submit() const;
	void draw(GLenum mode) const;
	void clear();

	inline GLuint get_vbo() const { return vbo; }

private:
	bool updating{ false };

	u32 prim_size{ 0 };
	u32 capacity{ 0 };
	u32 max_drawcalls{ 0 };

	u32 cursor{ 0 };
	u32 frame{ 0 };

	GLuint vbo{ 0 };
	u8* vert_data{ nullptr };

	u32 drawcount{ 0 };
	u32 buffer_count{ 0 };
	struct drawcalls_t* drawcalls{ nullptr };
};