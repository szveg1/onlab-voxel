#pragma once
#include <glad/glad.h>

#include "GPUProgram.h"

class Geometry
{
protected:
	GLuint vertexBuffer, vertexNormalBuffer, vertexTexCoordBuffer;
	GLuint indexBuffer;
	GLuint vertexArray;

public:
	Geometry() {
		glGenBuffers(1, &vertexBuffer);
		glGenBuffers(1, &vertexNormalBuffer);
		glGenBuffers(1, &vertexTexCoordBuffer);
		glGenBuffers(1, &indexBuffer);
		glGenVertexArrays(1, &vertexArray);
	}

	virtual void draw() = 0;

	virtual ~Geometry() {
		glDeleteBuffers(1, &vertexBuffer);
		glDeleteBuffers(1, &vertexNormalBuffer);
		glDeleteBuffers(1, &vertexTexCoordBuffer);
		glDeleteBuffers(1, &indexBuffer);
		glDeleteVertexArrays(1, &vertexArray);
	}
};