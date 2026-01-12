#include <device.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static GLFWwindow* window;

static int glad_init();

bool device_init(const config_t& config)
{
	INFO("Initializing GLFW...");
	if (!glfwInit())
		ERROR("Failed to initialize GLFW");
	INFO("[OK] GLFW initialized.");

	INFO("Creating window...");
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(config.display_width, config.display_height, config.display_title, NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		ERROR("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(window);
	INFO("[OK] GLFW window created.");

	INFO("Initializing GLAD...");
	if (!glad_init())
		ERROR("Failed to initialize GLAD");
	INFO("[OK] GLAD initialized.");

	INFO("[DONE] Device initialize.");
	return true;
}

void device_shutdown()
{
	glfwDestroyWindow(window);
	glfwTerminate();

	INFO("[DONE] Device shutdown.");
}

bool device_begin_frame()
{
	return !glfwWindowShouldClose(window);
}

void device_end_frame()
{
	glfwPollEvents();
	glfwSwapBuffers(window);
}

void device_close()
{
	glfwSetWindowShouldClose(window, true);
}

f64 device_time()
{
	return glfwGetTime();
}

void device_size(i32* width, i32* height)
{
	glfwGetFramebufferSize(window, width, height);
}

bool device_button(u8 btn)
{
	if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
		return false;

	GLFWgamepadstate state;
	if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
		return false;

	switch (btn)
	{
	case GP_BTN_A:      return state.buttons[GLFW_GAMEPAD_BUTTON_A];
	case GP_BTN_B:      return state.buttons[GLFW_GAMEPAD_BUTTON_B];
	case GP_BTN_X:      return state.buttons[GLFW_GAMEPAD_BUTTON_X];
	case GP_BTN_Y:      return state.buttons[GLFW_GAMEPAD_BUTTON_Y];
	case GP_BTN_L1:     return state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
	case GP_BTN_L2:     return state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
	case GP_BTN_L3:     return state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];
	case GP_BTN_R1:     return state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
	case GP_BTN_R2:     return state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
	case GP_BTN_R3:     return state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB];
	case GP_BTN_SELECT: return state.buttons[GLFW_GAMEPAD_BUTTON_BACK];
	case GP_BTN_START:  return state.buttons[GLFW_GAMEPAD_BUTTON_START];
	case GP_BTN_UP:     return state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
	case GP_BTN_DOWN:   return state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
	case GP_BTN_LEFT:   return state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
	case GP_BTN_RIGHT:  return state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
	default: return false;
	}
}

f32 device_axis(u8 axis)
{
	if (!glfwJoystickPresent(GLFW_JOYSTICK_1))
		return 0.f;

	i32 count = 0;
	const f32* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);
	if (!axes || axis >= count)
		return 0.f;
	return axes[axis];
}

#include <glad/glad.h>
static int glad_init()
{
	return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}