#pragma once
#include <glad/glad.h>
#include <string>

class Shader
{
public:
	Shader(GLuint shaderType, const char* sourcePath);
	~Shader();
	GLuint shaderObject;

private:
	bool checkShader(GLuint shader, std::string message);
};