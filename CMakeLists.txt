cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(ndeex VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)

include(cmake/CPM.cmake)

find_package(Vulkan REQUIRED)

CPMAddPackage(
  NAME sdl
  GITHUB_REPOSITORY libsdl-org/SDL
  GIT_TAG release-3.2.10
)

add_executable(ndeex src/main.cpp src/Vulkan.cpp)
target_include_directories(ndeex PRIVATE src)
target_link_libraries(ndeex PRIVATE Vulkan::Vulkan SDL3::SDL3 opengl32 )

