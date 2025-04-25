#pragma once
#include <glad/glad.h>

class Arrow {
public:
  Arrow();
  ~Arrow();
  void draw();

private:
  GLuint VAO, VBO;
  void setupArrow();
};
