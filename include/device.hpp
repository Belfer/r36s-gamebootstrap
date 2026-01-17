#pragma once

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <types.hpp>
#include <math.hpp>
#include <glad/glad.h>

#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define ASSERT(cond, fmt, ...) \
    { if (!(cond)) { printf("[FAIL] (" #cond ") " fmt "\n", ##__VA_ARGS__); assert(cond); } }

#define GP_BTN_A		0x00
#define GP_BTN_B		0x01
#define GP_BTN_X		0x02
#define GP_BTN_Y		0x03
#define GP_BTN_L1		0x04
#define GP_BTN_L2		0x05
#define GP_BTN_L3		0x06
#define GP_BTN_R1		0x07
#define GP_BTN_R2		0x08
#define GP_BTN_R3		0x09
#define GP_BTN_SELECT	0x0A
#define GP_BTN_START	0x0B
#define GP_BTN_MODE		0x0C
#define GP_BTN_UP		0x0D
#define GP_BTN_DOWN		0x0E
#define GP_BTN_LEFT		0x0F
#define GP_BTN_RIGHT	0x10
#define GP_BTN_COUNT	0x11

#define GP_AXIS_LX		0x00
#define GP_AXIS_LY		0x01
#define GP_AXIS_RX		0x02
#define GP_AXIS_RY		0x03
#define GP_AXIS_COUNT	0x04

#define glGenVertexArraysX (glGenVertexArrays ? glGenVertexArrays : glGenVertexArraysOES ? glGenVertexArraysOES : nullptr)
#define glBindVertexArrayX (glBindVertexArray ? glBindVertexArray : glBindVertexArrayOES ? glBindVertexArrayOES : nullptr)
#define glDeleteVertexArraysX (glDeleteVertexArrays ? glDeleteVertexArrays : glDeleteVertexArraysOES ? glDeleteVertexArraysOES : nullptr)

struct config_t
{
	const char* display_title{ "Title" };
	i32 display_width{ 800 };
	i32 display_height{ 600 };
	bool display_vsync{ true };

	u32 audio_sample_rate{ 44100 };
	i32 audio_channels{ 2 };
	i32 audio_frame_count{ 256 };
	typedef void (*audio_callback_t)(i16* samples, i32 frames, void* userdata);
	audio_callback_t audio_callback{ nullptr };
	void* audio_userdata{ nullptr };
};

// Device / system management
bool init(const config_t& config);
void shutdown();
bool begin_frame();
void end_frame();
void close();
void screen_size(i32* width, i32* height);

// Input / timing
f64 get_time();
bool is_button_pressed(u8 btn);
f32 get_axis_value(u8 axis);

// OpenGL utilities
GLuint create_program(const char* vsrc, const char* fsrc);
GLuint create_buffer(GLenum type, GLenum usage, GLsizei size, void* data);
