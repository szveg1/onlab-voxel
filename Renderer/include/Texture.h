#pragma once

#include <glad/glad.h>

class Texture
{
public:
	Texture(GLuint width, GLuint height);
	void bind(int binding);
	void bindCompute(int binding);
	~Texture();
private:
	GLuint id;
	GLuint width, height;
};