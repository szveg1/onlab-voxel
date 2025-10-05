#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class GPUProgram
{
public:
	GPUProgram(Shader* vertexShader, Shader* fragmentShader, Shader* geometryShader = nullptr);
	GPUProgram(Shader* computeShader);
	~GPUProgram();
	GLuint ID;
	void use() const;
	void setUniform(int i, const std::string& name);
	void setUniform(float f, const std::string& name);
	void setUniform(const glm::vec2& v, const std::string& name);
	void setUniform(const glm::vec3& v, const std::string& name);
	void setUniform(const glm::vec4& v, const std::string& name);
	void setUniform(const glm::mat4& mat, const std::string& name);
	void setUniform(const GLuint u, const std::string& name);
	void setUniform(const bool b, const std::string& name);
	int getLocation(const std::string& name);
private:
	bool checkLinking(GLuint program);
};