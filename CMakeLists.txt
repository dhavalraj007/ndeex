cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(ndeex VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 26)

include(cmake/CPM.cmake)

find_package(Vulkan REQUIRED)

CPMAddPackage(
  NAME glfw
  GITHUB_REPOSITORY glfw/glfw
  GIT_TAG 3.4
  OPTIONS "GLFW_BUILD_EXAMPLES OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_DOCS OFF"
)

add_executable(ndeex src/main.cpp src/Vulkan.cpp)
target_include_directories(ndeex PRIVATE src)
target_link_libraries(ndeex PRIVATE Vulkan::Vulkan glfw opengl32 )

