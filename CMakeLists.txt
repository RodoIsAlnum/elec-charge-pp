cmake_minimum_required(VERSION 3.10)
project(Vectores)

set(CMAKE_CXX_STANDARD 17)

find_path(GLM_INCLUDE_DIR glm/glm.hpp)
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h)
find_path(FTYPE_INCLUDE_DIR freetype2/freetype/freetype.h)

include_directories(external/glad/include)
include_directories(${GLFW_INCLUDE_DIR})
include_directories(${GLM_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${FTYPE_INCLUDE_DIR})

add_library(glad STATIC external/glad/src/glad.c)

#add_subdirectory(external/glfw)

add_executable(Vectores
  main.cpp
  Arrow.cpp
  ChargeRenderer.cpp
  TextRender.cpp
  Menu.cpp
)

target_link_libraries(Vectores glad glfw dl freetype)