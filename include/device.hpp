#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

using uchar = unsigned char;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

#define INFO(fmt, ...) { printf("[INFO] " fmt "\n", ##__VA_ARGS__); }
#define WARN(fmt, ...) { printf("[WARN] " fmt "\n", ##__VA_ARGS__); }
#define ERROR(fmt, ...) { printf("[ERROR] " fmt "\n", ##__VA_ARGS__); exit(1); }
#define FAIL_CHECK(cond, fmt, ...) { if (cond) { printf("[FAIL] (" #cond ") " fmt "\n", ##__VA_ARGS__); exit(1); } }

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

struct config_t
{
	const char* display_title{ nullptr };
	i32 display_width{ 0 };
	i32 display_height{ 0 };
	bool display_vsync{ false };

	u32 audio_sample_rate{ 0 };
	i32 audio_channels{ 0 };
	typedef void (*audio_callback_t)(i16* samples, i32 frames, void* userdata);
	audio_callback_t audio_callback{ nullptr };
	void* audio_userdata{ nullptr };
};

bool device_init(const config_t& config);
void device_shutdown();
bool device_begin_frame();
void device_end_frame();
void device_close();

f64 device_time();
void device_size(i32* width, i32* height);

bool device_button(u8 btn);
f32 device_axis(u8 axis);