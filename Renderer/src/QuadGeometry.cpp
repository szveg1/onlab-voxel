#include <vector>

#include "QuadGeometry.h"

QuadGeometry::QuadGeometry() : Geometry()
{
    glBindVertexArray(vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    std::vector<float> vertices = {
        -1.0f, -1.0f, 0.5f,
        -1.0f,  1.0f, 0.5f,
         1.0f, -1.0f, 0.5f,
         1.0f,  1.0f, 0.5f
    };
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vertexNormalBuffer);
    std::vector<float> normals = {
        0.0f,  0.0f, 1.0f,
        0.0f,  0.0f, 1.0f,
        0.0f,  0.0f, 1.0f,
        0.0f,  0.0f, 1.0f
    };
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vertexTexCoordBuffer);
    std::vector<float> texCoords = {
        0.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  1.0f,
        1.0f,  0.0f
    };
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), texCoords.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    std::vector<GLushort> indices = {
        0, 1, 2,
        1, 2, 3
    };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);


    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, vertexNormalBuffer);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, vertexTexCoordBuffer);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
}

QuadGeometry::~QuadGeometry()
{
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &vertexNormalBuffer);
    glDeleteBuffers(1, &vertexTexCoordBuffer);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}

void QuadGeometry::draw() {
	glBindVertexArray(vertexArray);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}