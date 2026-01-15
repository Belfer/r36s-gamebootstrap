#include <batch.hpp>
#include <device.hpp>

batch_t::batch_t(u32 prim_size, u32 capacity, u32 max_drawcalls, u32 buffer_count)
	: prim_size(prim_size), capacity(capacity), max_drawcalls(max_drawcalls), buffer_count(buffer_count)
{
	vert_data = new u8[prim_size * capacity];
	first = new GLint[max_drawcalls];
	count = new GLsizei[max_drawcalls];

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	if (glBufferStorageX == nullptr)
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)buffer_count * capacity * prim_size, nullptr, GL_DYNAMIC_DRAW);
	else
		glBufferStorageX(GL_ARRAY_BUFFER, (GLsizeiptr)buffer_count * capacity * prim_size, nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

batch_t::~batch_t()
{
	delete[] vert_data;
	delete[] first;
	delete[] count;

	glDeleteBuffers(1, &vbo);
}

void batch_t::begin()
{
	if ((drawcount + 1) > max_drawcalls)
	{
		LOG_WARN("Max draw calls reached, dropping batch draws!");
		return;
	}
	first[drawcount] = (frame * capacity) + cursor;
}

void batch_t::add_vertices(u8* data, u32 size)
{
	ASSERT((size % prim_size) == 0, "Adding primitive unaligned vertices!");

	const u32 next = cursor + (size / prim_size);
	if (next > capacity)
	{
		LOG_WARN("Batch buffer is full, dropping vertices!");
		return;
	}

	u8* ptr = vert_data + (cursor * prim_size);
	memcpy(ptr, data, size);
	cursor = next;
}

void batch_t::end()
{
	GLint curr = (frame * capacity) + cursor;
	if (curr == first[drawcount])
		return;

	count[drawcount] = curr - first[drawcount];
	drawcount++;
}

void batch_t::submit() const
{
	if (drawcount == 0) return;

	const GLsizeiptr size = (GLsizeiptr)cursor * prim_size;
	const GLintptr offset = (GLintptr)frame * capacity * prim_size;

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	u8* ptr = (u8*)glMapBufferRangeX(GL_ARRAY_BUFFER, offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	ASSERT(ptr != nullptr, "Failed to map buffer!");

	memcpy(ptr, vert_data, size);

	glUnmapBufferX(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void batch_t::draw(GLenum mode) const
{
	if (drawcount == 0) return;
	glMultiDrawArraysX(mode, first, count, drawcount);
}

void batch_t::clear()
{
	cursor = 0;
	drawcount = 0;
	frame = (frame + 1) % buffer_count;
}