#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <set>

class Camera
{
public:
	Camera();
	void setAspectRatio(float aspectRatio);
	void move(std::set<int> keysPressed, float dt);
	void mouseMove(double x, double y);
	const glm::mat4 ViewProjMatrix() { return viewProjMatrix; }
	const glm::mat4 RayDirMatrix() { return rayDirMatrix; }
	const glm::vec3 Position() { return position; }
private:
	glm::vec3 position = glm::vec3(0, 1, 1);
	float roll = 0, pitch = 0, yaw = 0;
	float fov = 1, aspect = 1;
	float nearPlane = 1, farPlane = 1000;
	glm::vec3 ahead = glm::vec3(0, 0, 1);
	glm::vec3 right = glm::vec3(1, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);
	const glm::vec3 worldUp = glm::vec3(0, 1, 0);
	glm::mat4 rotationMatrix = glm::mat4(1);
	glm::mat4 viewProjMatrix = glm::mat4(1);
	glm::mat4 rayDirMatrix = glm::mat4(1);
	bool firstMouse = true;
	float lastX, lastY;

	void update();

};