#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <set>

using namespace glm;
class Camera
{
public:
	Camera();
	void setAspectRatio(float aspectRatio);
	void move(std::set<int> keysPressed, float dt);
	void mouseMove(double x, double y);
	const mat4 ViewProjMatrix() { return viewProjMatrix; }
	const mat4 RayDirMatrix() { return rayDirMatrix; }
	const vec3 Position() { return position; }
private:
	vec3 position = vec3(0, 1, 1);
	float roll = 0, pitch = 0, yaw = 0;
	float fov = radians(90.0f), aspect = 1;
	float nearPlane = 1, farPlane = 1000;
	vec3 ahead = vec3(0, 0, 1);
	vec3 right = vec3(1, 0, 0);
	vec3 up = vec3(0, 1, 0);
	const vec3 worldUp = vec3(0, 1, 0);
	mat4 rotationMatrix = mat4(1);
	mat4 viewProjMatrix = mat4(1);
	mat4 rayDirMatrix = mat4(1);
	bool firstMouse = true;
	float lastX, lastY;

	void update();

};