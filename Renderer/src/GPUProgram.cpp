#include <glm/glm.hpp>

#include "GPUProgram.h"
#include "Shader.h"


GPUProgram::GPUProgram(Shader* vertexShader, Shader* fragmentShader, Shader* geometryShader)
{
	ID = glCreateProgram();
	glAttachShader(ID, vertexShader->shaderObject);
	glAttachShader(ID, fragmentShader->shaderObject);
	if (geometryShader) glAttachShader(ID, geometryShader->shaderObject);
	glLinkProgram(ID);
	if (!checkLinking(ID)) return;
}

GPUProgram::GPUProgram(Shader* computeShader)
{
	ID = glCreateProgram();
	glAttachShader(ID, computeShader->shaderObject);
	glLinkProgram(ID);
	if (!checkLinking(ID)) return;
}

GPUProgram::~GPUProgram() { if (ID > 0) glDeleteProgram(ID); }

bool GPUProgram::checkLinking(GLuint program) 
{
	GLint infoLogLength = 0, result = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (!result) {
		std::string errorMessage(infoLogLength, '\0');
		glGetProgramInfoLog(program, infoLogLength, nullptr, (GLchar*)errorMessage.data());
		printf("Failed to link shader program! \n Log: \n%s\n", errorMessage.c_str());
		getchar();
		return false;
	}
	return true;
}

void GPUProgram::use() const
{
	glUseProgram(ID);
}

int GPUProgram::getLocation(const std::string& name) {
	int location = glGetUniformLocation(ID, name.c_str());
	if (location < 0) printf("uniform %s cannot be set\n", name.c_str());
	return location;
}

using namespace glm;

void GPUProgram::setUniform(int i, const std::string& name) {
	int location = getLocation(name);
	if (location >= 0) glUniform1i(location, i);
}

void GPUProgram::setUniform(float f, const std::string& name) {
	int location = getLocation(name);
	if (location >= 0) glUniform1f(location, f);
}

void GPUProgram::setUniform(const vec2& v, const std::string& name) {
	int location = getLocation(name);
	if (location >= 0) glUniform2fv(location, 1, &v.x);
}

void GPUProgram::setUniform(const vec3& v, const std::string& name) {
	int location = getLocation(name);
	if (location >= 0) glUniform3fv(location, 1, &v.x);
}

void GPUProgram::setUniform(const vec4& v, const std::string& name) {
	int location = getLocation(name);
	if (location >= 0) glUniform4fv(location, 1, &v.x);
}

void GPUProgram::setUniform(const mat4& mat, const std::string& name) {
	int location = getLocation(name);
	if (location >= 0) glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
}

void GPUProgram::setUniform(const GLuint u, const std::string& name)
{
	int location = getLocation(name);
	if (location >= 0) glUniform1ui(location, u);
}

void GPUProgram::setUniform(const bool b, const std::string& name)
{
	int location = getLocation(name);
	if (location >= 0) glUniform1i(location, b);
}
