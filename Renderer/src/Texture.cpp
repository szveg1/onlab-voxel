#include "Texture.h"

Texture::Texture(GLuint width, GLuint height) : width(width), height(height)
{
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture::bind(int binding)
{
	glActiveTexture(GL_TEXTURE0 + binding);
	glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::bindCompute(int binding)
{
	glBindImageTexture(binding, id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

Texture::~Texture() 
{
	glDeleteTextures(1, &id);
}