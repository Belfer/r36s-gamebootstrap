#include <debug.hpp>
#include <ringbuff.hpp>

static GLuint prg{ 0 };
static GLuint vao{ 0 };
static GLuint vbo{ 0 };
static GLint umvp{ 0 };

#define BATCH_SIZE (1024)
#define BATCH_COUNT (2)
#define BUFFER_SIZE (BATCH_SIZE + BATCH_SIZE*2 + BATCH_SIZE*3)
static u8 curr_batch{ 0 };
static bool begun{ false };

struct vertex_t { vec3 pos{}; u32 col{ 0 }; };
template <u32 Size> struct vertex_arr_t { vertex_t v[Size]{}; };

static ring_buff_t<vertex_arr_t<1>, BATCH_SIZE> points_rb{};
static ring_buff_t<vertex_arr_t<2>, BATCH_SIZE> lines_rb{};
static ring_buff_t<vertex_arr_t<3>, BATCH_SIZE> tris_rb{};

static const char* vsrc = R"(
#version 100
attribute vec3 aPos;
attribute vec4 aCol;
uniform mat4 uMVP;
varying vec4 vCol;
void main() {
	gl_Position = uMVP * vec4(aPos, 1.0);
	vCol = aCol;
})";

static const char* fsrc = R"(
#version 100
precision mediump float;
varying vec4 vCol;
void main() {
	gl_FragColor = vCol;
})";

bool debug_init()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	//glBufferData(GL_ARRAY_BUFFER, BATCH_COUNT * BUFFER_SIZE * sizeof(vertex_t), nullptr, GL_DYNAMIC_DRAW);
	glBufferStorage(GL_ARRAY_BUFFER, BATCH_COUNT * BUFFER_SIZE * sizeof(vertex_t), nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

	prg = create_program(vsrc, fsrc);
	const GLint apos = glGetAttribLocation(prg, "aPos");
	const GLint acol = glGetAttribLocation(prg, "aCol");
	umvp = glGetUniformLocation(prg, "uMVP");

	glEnableVertexAttribArray(apos);
	glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)0);
	glEnableVertexAttribArray(acol);
	glVertexAttribPointer(acol, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_t), (void*)(3 * sizeof(f32)));

	glBindVertexArray(0);

	return true;
}

void debug_shutdown()
{
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(prg);
}

void debug_begin()
{
	points_rb.consume(points_rb.count());
	lines_rb.consume(lines_rb.count());
	tris_rb.consume(tris_rb.count());
	begun = true;
	curr_batch = (curr_batch + 1) % BATCH_COUNT;
}

void debug_point(const vec3& a, u32 color)
{
	if (!begun) return;
	vertex_arr_t<1> p{};
	p.v[0].col = color;
	p.v[0].pos = a;
	points_rb.add(p);
}

void debug_line(const vec3& a, const vec3& b, u32 color)
{
	if (!begun) return;
	vertex_arr_t<2> l{};
	l.v[0].pos = a;
	l.v[0].col = color;
	l.v[1].pos = b;
	l.v[1].col = color;
	lines_rb.add(l);
}

void debug_tri(const vec3& a, const vec3& b, const vec3& c, u32 color)
{
	if (!begun) return;
	vertex_arr_t<3> t{};
	t.v[0].pos = a;
	t.v[0].col = color;
	t.v[1].pos = b;
	t.v[1].col = color;
	t.v[2].pos = c;
	t.v[2].col = color;
	tris_rb.add(t);
}

template <typename T, u32 S>
static void cpy_rb(const ring_buff_t<T, S>& rb, u8*& ptr)
{
	if (rb.count() != 0)
		memcpy(ptr, &rb.data[0], rb.size() * sizeof(T));
	ptr += rb.size() * sizeof(T);
}

void debug_end()
{
	begun = false;

	const GLsizeiptr size = BUFFER_SIZE * sizeof(vertex_t);
	const GLintptr offset = curr_batch * size;

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	u8* ptr = (u8*)glMapBufferRange(GL_ARRAY_BUFFER, offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	ASSERT(ptr != nullptr, "Failed to map buffer!");

	cpy_rb(tris_rb, ptr);
	cpy_rb(lines_rb, ptr);
	cpy_rb(points_rb, ptr);

	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template <typename T, u32 S>
static void draw_rb(const ring_buff_t<T, S>& rb, GLenum mode, GLint& start_offset)
{
	const u32 size = sizeof(T) / sizeof(vertex_t);
	if (rb.count() != 0)
	{
		if (rb.end > rb.start)
		{
			glDrawArrays(mode, start_offset + (size * rb.start), size * rb.count());
		}
		else
		{
			GLint first[2]{};
			GLsizei count[2]{};
			first[0] = start_offset + (size * rb.start);
			count[0] = size * (rb.size() - rb.start);
			first[1] = start_offset;
			count[1] = size * rb.end;
			glMultiDrawArrays(mode, first, count, count[1] > 0 ? 2 : 1);
		}
	}

	start_offset += size * rb.size();
}

void debug_draw(const mat4& mvp)
{
	glBindVertexArray(vao);
	glUseProgram(prg);
	glUniformMatrix4fv(umvp, 1, GL_FALSE, &mvp.data[0].x);

	GLint start_offset = curr_batch * BUFFER_SIZE;
	draw_rb(tris_rb, GL_TRIANGLES, start_offset);
	draw_rb(lines_rb, GL_LINES, start_offset);
	draw_rb(points_rb, GL_POINTS, start_offset);
	
	glUseProgram(0);
	glBindVertexArray(0);
}