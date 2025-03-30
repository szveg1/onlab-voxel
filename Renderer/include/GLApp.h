#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

enum MouseButton { MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT };
enum SpecialKeys { KEY_RIGHT = 262, KEY_LEFT = 263, KEY_DOWN = 264, KEY_UP = 265 };

class GLApp
{
public:
	GLApp(const char* title);
	~GLApp();
	int versionMajor, versionMinor;
	int width, height;
	const char* title;
	void run();

	virtual void onInit() {}
	virtual void onDisplay() {}
	virtual void onKeyboard(int key) {}
	virtual void onKeyboardUp(int key) {}
	virtual void onMousePressed(MouseButton but, int pX, int pY) {}
	virtual void onMouseReleased(MouseButton but, int pX, int pY) {}
	virtual void onMouseScrolled(double xoffset, double yoffset) {}
	virtual void onMouseMotion(double x, double y) {}
	virtual void onTimeElapsed(float startTime, float endTime) {}
protected:
	virtual void window_size_callback(GLFWwindow* window, int width, int height);
private:
	static GLApp* instance;
	GLFWwindow* window;
	static void errorCallback(int error, const char* description);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void cursorPosCallback(GLFWwindow* window, double x, double y);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};