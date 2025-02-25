#include "Camera.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


Camera::Camera()
{
    update();
}

void Camera::update()
{
    rotationMatrix = mat4(1);
    rotationMatrix = rotate(rotationMatrix, roll, vec3(0, 0, 1));
    rotationMatrix = rotate(rotationMatrix, pitch, vec3(1, 0, 0));
    rotationMatrix = rotate(rotationMatrix, yaw, vec3(0, 1, 0));

    viewProjMatrix = rotationMatrix;
    viewProjMatrix = translate(viewProjMatrix, position);
    viewProjMatrix = inverse(viewProjMatrix);

    float yScale = 1.0f / tan(fov * 0.5f);
    float xScale = yScale / aspect;
    float f = farPlane, n = nearPlane;
    
    viewProjMatrix *= mat4(
        xScale, 0, 0, 0,
        0, yScale, 0, 0,
        0, 0, -(f + n) / (n - f), -1,
        0, 0, -2 * f * n / (n - f), 0
    );

    rayDirMatrix = mat4(1);
    rayDirMatrix *= viewProjMatrix;
    rayDirMatrix = inverse(rayDirMatrix);
}


void Camera::setAspectRatio(float aspectRatio)
{
	aspect = aspectRatio;
	update();
}

void Camera::move(std::set<int> keysPressed, float dt)
{
    float speed = 5.0f * dt;

    if (keysPressed.count('w')) {
        position += speed * ahead;
    }
    if (keysPressed.count('s')) {
        position -= speed * ahead;
    }
    if (keysPressed.count('a')) {
        position += speed * right;
    }
    if (keysPressed.count('d')) {
        position -= speed * right;
    }
    if (keysPressed.count('q')) {
        position += speed * up;
    }
    if (keysPressed.count('e')) {
        position -= speed * up;
    }

    update();
}

void Camera::mouseMove(double x, double y)
{
    if (firstMouse)
    {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    float xoffset = x - lastX;
    float yoffset = lastY - y;
    lastX = x;
    lastY = y;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;


    yaw -= radians(xoffset);
    pitch -= radians(yoffset);

    if (pitch > radians(89.0f))
        pitch = radians(89.0f);
    if (pitch < radians(-89.0f))
        pitch = radians(-89.0f);

    update();
    ahead = vec4(0, 0, 1, 0) * rotationMatrix;
    right = vec4(1, 0, 0, 0) * rotationMatrix;
    up = vec4(0, 1, 0, 0) * rotationMatrix;
}

