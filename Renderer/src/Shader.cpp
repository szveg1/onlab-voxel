#include "Shader.h"

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


Shader::Shader(GLuint shaderType, const char* sourcePath)
{
	std::string sourceCode;
	std::ifstream shaderFile;

	shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try 
	{
		shaderFile.open(sourcePath);
		std::stringstream shaderStream;

		shaderStream << shaderFile.rdbuf();

		shaderFile.close();

		sourceCode = shaderStream.str();
	}

	catch (std::ifstream::failure f) 
	{
		std::cerr << "Could not read shader file: " << sourcePath << std::endl;
	}

	const char* shaderCode = sourceCode.c_str();

	shaderObject = glCreateShader(shaderType);
	glShaderSource(shaderObject, 1, &shaderCode, NULL);
	glCompileShader(shaderObject);

	if (!checkShader(shaderObject, "Error in shader: " + std::string(sourcePath))) return;
}

Shader::~Shader()
{
}

bool Shader::checkShader(GLuint shader, std::string message)
{
	GLint infoLogLength = 0, result = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (!result) {
		std::string errorMessage(infoLogLength, '\0');
		glGetShaderInfoLog(shader, infoLogLength, NULL, (GLchar*)errorMessage.data());
		printf("%s! \n Log: \n%s\n", message.c_str(), errorMessage.c_str());
		getchar();
		return false;
	}
	return true;
}
