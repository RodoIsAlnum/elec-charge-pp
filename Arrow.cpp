#include "Arrow.hpp"

Arrow::Arrow() {
  setupArrow();
}

Arrow::~Arrow() {
  glDeleteVertexArrays(1,&VAO);
  glDeleteBuffers(1,&VBO);
}

void Arrow::setupArrow() {
  float vertices[] = {
    // Base Rectangle
    -0.4f, -0.05f, 0.0f,  // left bottom
     0.4f, -0.05f, 0.0f,  // right bottom
     0.4f,  0.05f, 0.0f,  // right top
    -0.4f, -0.05f, 0.0f,  // left bottom
     0.4f,  0.05f, 0.0f,  // right top
    -0.4f,  0.05f, 0.0f,  // left top
    
    // Tip
     0.4f, -0.10f, 0.0f,  // bottom of tip
     0.5f,  0.00f, 0.0f,  // point of tip
     0.4f,  0.10f, 0.0f,  // top of tip
  };
  
  glGenVertexArrays(1,&VAO);
  glGenBuffers(1,&VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
  glEnableVertexAttribArray(0);
}

void Arrow::draw() {
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES,0,9);
}
