#pragma once

#include "glcore.h"
#include <glm/mat4x4.hpp>
#include <string>

class Shader {
public:
    Shader(const char* vertexSource, const char* fragmentSource);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    void setMat4(const char* name, const glm::mat4& value) const;
    GLuint id() const { return program_; }

private:
    static GLuint compile(GLenum type, const char* source);
    GLuint program_ = 0;
};
