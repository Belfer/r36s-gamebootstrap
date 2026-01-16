#include <batch.hpp>
#include <device.hpp>

#define ENABLE_MULTI_DRAW (glMultiDrawArraysX != nullptr && true)
#define ENABLE_INDIRECT (glDrawArraysIndirect != nullptr && false)

#define IDX_MAX_VAL 0xEFFF
#define IDX_RESET_VAL 0xFFFF
#define IDX_TYPE u16
#define IDX_GL_TYPE GL_UNSIGNED_SHORT

struct drawcalls_t
{
	// multi draw (preferred)
	GLint* first{ nullptr };
	GLsizei* count{ nullptr };

	// draw indirect (overkill but use if available)
	struct cmd_t
	{
		GLuint count{ 0 };
		GLuint instanceCount{ 0 };
		GLuint first{ 0 };
		GLuint baseInstance{ 0 };
	};
	cmd_t* cmds{ nullptr };
	GLuint cbo{ 0 };

	// draw elements (memory inefficient, last fallback)
	IDX_TYPE idx_cursor{ 0 };
	u32 idx_passes{ 0 };
	u32 idx_count{ 0 };
	IDX_TYPE* indices{ nullptr };
	GLuint ebo{ 0 };
};

batch_t::batch_t(u32 prim_size, u32 capacity, u32 max_drawcalls, u32 buffer_count)
	: prim_size(prim_size), capacity(capacity), max_drawcalls(max_drawcalls), buffer_count(buffer_count)
{
	vert_data = new u8[capacity * prim_size];
	drawcalls = new drawcalls_t{};

	if (ENABLE_MULTI_DRAW)
	{
		drawcalls->first = new GLint[max_drawcalls];
		drawcalls->count = new GLsizei[max_drawcalls];
	}
	else if (ENABLE_INDIRECT)
	{
		drawcalls->cmds = new drawcalls_t::cmd_t[max_drawcalls];
		glGenBuffers(1, &drawcalls->cbo);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawcalls->cbo);

		if (glBufferStorageX == nullptr)
			glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)buffer_count * max_drawcalls * sizeof(drawcalls_t::cmd_t), nullptr, GL_DYNAMIC_DRAW);
		else
			glBufferStorageX(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)buffer_count * max_drawcalls * sizeof(drawcalls_t::cmd_t), nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
	}
	else
	{
		//if (vert_count > IDX_MAX_VAL)
		//{
		//	LOG_WARN("Batcher using draw arrays with indices, might dispatch multiple draws!", IDX_MAX_VAL);
		//}

		drawcalls->indices = new IDX_TYPE[capacity + max_drawcalls];
		glGenBuffers(1, &drawcalls->ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawcalls->ebo);

		if (glBufferStorageX == nullptr)
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)buffer_count * ((GLsizeiptr)capacity + max_drawcalls) * sizeof(IDX_TYPE), nullptr, GL_DYNAMIC_DRAW);
		else
			glBufferStorageX(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)buffer_count * ((GLsizeiptr)capacity + max_drawcalls) * sizeof(IDX_TYPE), nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
	}

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

	if (ENABLE_MULTI_DRAW)
	{
		delete[] drawcalls->first;
		delete[] drawcalls->count;
	}
	else if (ENABLE_INDIRECT)
	{
		delete[] drawcalls->cmds;
	}
	else
	{
		delete[] drawcalls->indices;
		glDeleteBuffers(1, &drawcalls->ebo);
	}

	delete drawcalls;

	glDeleteBuffers(1, &vbo);
}

void batch_t::begin()
{
	if ((drawcount + 1) > max_drawcalls)
	{
		LOG_WARN("Max draw calls reached, dropping batch draws!");
		return;
	}

	if (ENABLE_MULTI_DRAW)
	{
		drawcalls->first[drawcount] = (frame * capacity) + cursor;
	}
	else if (ENABLE_INDIRECT)
	{
		drawcalls->cmds[drawcount].first = (frame * capacity) + cursor;
	}

	updating = true;
}

void batch_t::add_vertices(u8* data, u32 size)
{
	ASSERT(updating, "Call begin before adding data!");
	ASSERT((size % prim_size) == 0, "Adding primitive unaligned vertices!");

	const u32 count = size / prim_size;
	const u32 next = cursor + count;
	if (next > capacity)
	{
		LOG_WARN("Batch buffer is full, dropping vertices!");
		return;
	}

	u8* ptr = vert_data + (cursor * prim_size);
	memcpy(ptr, data, size);
	cursor = next;

	if (!ENABLE_MULTI_DRAW && !ENABLE_INDIRECT)
	{
		if ((drawcalls->idx_cursor + count) >= IDX_MAX_VAL)
		{
			for (u32 i = drawcalls->idx_cursor; i <= IDX_MAX_VAL; i++)
				drawcalls->indices[drawcalls->idx_count++] = IDX_RESET_VAL;

			drawcalls->idx_passes++;
			drawcalls->idx_cursor = 0;
		}

		for (u32 i = 0; i < count; i++)
			drawcalls->indices[drawcalls->idx_count++] = (IDX_TYPE)(drawcalls->idx_cursor + i);
		drawcalls->idx_cursor += (IDX_TYPE)count;
	}
}

void batch_t::end()
{
	updating = false;
	const GLint curr = (frame * capacity) + cursor;

	if (ENABLE_MULTI_DRAW)
	{
		if (curr == drawcalls->first[drawcount])
			return;
		drawcalls->count[drawcount] = curr - drawcalls->first[drawcount];
	}
	else if (ENABLE_INDIRECT)
	{
		if (curr == drawcalls->cmds[drawcount].first)
			return;
		drawcalls->cmds[drawcount].count = curr - drawcalls->cmds[drawcount].first;
		drawcalls->cmds[drawcount].instanceCount = 1;
		drawcalls->cmds[drawcount].baseInstance = 0;
	}
	else
	{
		if (drawcalls->idx_count == 0)
			return;
		drawcalls->indices[drawcalls->idx_count++] = IDX_RESET_VAL;
	}
	
	drawcount++;
}

void batch_t::submit() const
{
	if (drawcount == 0) return;

	const GLsizeiptr vsize = (GLsizeiptr)cursor * prim_size;
	const GLintptr voffset = (GLintptr)frame * capacity * prim_size;

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	u8* vptr = (u8*)glMapBufferRangeX(GL_ARRAY_BUFFER, voffset, vsize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	ASSERT(vptr != nullptr, "Failed to map vertex buffer!");
	memcpy(vptr, vert_data, vsize);
	glUnmapBufferX(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (ENABLE_MULTI_DRAW)
	{
		// Nothing to do
	}
	else if (ENABLE_INDIRECT)
	{
		const GLsizeiptr csize = (GLsizeiptr)drawcount * sizeof(drawcalls_t::cmd_t);
		const GLintptr coffset = (GLintptr)frame * max_drawcalls * sizeof(drawcalls_t::cmd_t);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawcalls->cbo);
		u8* cptr = (u8*)glMapBufferRangeX(GL_DRAW_INDIRECT_BUFFER, coffset, csize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		ASSERT(cptr != nullptr, "Failed to map indirect buffer!");
		memcpy(cptr, drawcalls->cmds, csize);
		glUnmapBufferX(GL_DRAW_INDIRECT_BUFFER);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}
	else
	{
		const GLsizeiptr esize = (GLsizeiptr)drawcalls->idx_count * sizeof(IDX_TYPE);
		const GLintptr eoffset = (GLintptr)frame * ((GLintptr)capacity + max_drawcalls) * sizeof(IDX_TYPE);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawcalls->ebo);
		u8* eptr = (u8*)glMapBufferRangeX(GL_ELEMENT_ARRAY_BUFFER, eoffset, esize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		ASSERT(eptr != nullptr, "Failed to map index buffer!");
		memcpy(eptr, drawcalls->indices, esize);
		glUnmapBufferX(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

void batch_t::draw(GLenum mode) const
{
	if (drawcount == 0) return;

	if (ENABLE_MULTI_DRAW)
	{
		glMultiDrawArraysX(mode, drawcalls->first, drawcalls->count, drawcount);
	}
	else if (ENABLE_INDIRECT)
	{
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawcalls->cbo);
		if (glMultiDrawArraysIndirect != nullptr)
		{
			glMultiDrawArraysIndirect(mode, nullptr, drawcount, sizeof(drawcalls_t::cmd_t));
		}
		else
		{
			for (u32 i = 0; i < max_drawcalls; i++)
				glDrawArraysIndirect(mode, (void*)(i * sizeof(drawcalls_t::cmd_t)));
		}
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}
	else
	{
		const bool prim_restart = glPrimitiveRestartIndex != nullptr;
		if (prim_restart) glPrimitiveRestartIndex(IDX_RESET_VAL);

		if (GLAD_GL_VERSION_3_1 || GLAD_GL_ES_VERSION_3_0)
			glEnable(prim_restart ? GL_PRIMITIVE_RESTART : GL_PRIMITIVE_RESTART_FIXED_INDEX);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawcalls->ebo);

		for (u32 i = 0; i <= drawcalls->idx_passes; i++)
		{
			const GLsizei count = i == drawcalls->idx_passes ? drawcalls->idx_count : IDX_MAX_VAL;
			glDrawElements(mode, count, IDX_GL_TYPE, (void*)(i * IDX_RESET_VAL * sizeof(IDX_TYPE)));
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		if (GLAD_GL_VERSION_3_1 || GLAD_GL_ES_VERSION_3_0)
			glDisable(prim_restart ? GL_PRIMITIVE_RESTART : GL_PRIMITIVE_RESTART_FIXED_INDEX);
	}
}

void batch_t::clear()
{
	cursor = 0;
	drawcount = 0;
	frame = (frame + 1) % buffer_count;
	drawcalls->idx_count = 0;
	drawcalls->idx_cursor = 0;
	drawcalls->idx_passes = 0;
}