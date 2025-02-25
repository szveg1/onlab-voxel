#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

using namespace glm;
class GPUProgram
{
public:
	GPUProgram(Shader vertexShader, Shader fragmentShader);
	~GPUProgram();
	GLuint ID;
	void use() const;
	void setUniform(int i, const std::string& name);
	void setUniform(float f, const std::string& name);
	void setUniform(const vec2& v, const std::string& name);
	void setUniform(const vec3& v, const std::string& name);
	void setUniform(const vec4& v, const std::string& name);
	void setUniform(const mat4& mat, const std::string& name);
	int getLocation(const std::string& name);
private:
	bool checkLinking(GLuint program);
};