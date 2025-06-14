cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(ndeex VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)

include(cmake/CPM.cmake)
include(CMakeHelpers.cmake)

find_package(Vulkan REQUIRED)

CPMAddPackage(
  NAME sdl
  GITHUB_REPOSITORY libsdl-org/SDL
  GIT_TAG release-3.2.10
        OPTIONS
        "SDL_STATIC ON"
)

CPMAddPackage(
  NAME vma
  GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
  GIT_TAG v3.3.0
)

add_shaders(shaders src/triangle.vert src/triangle.frag)

add_executable(ndeex 
              src/main.cpp
              src/Vulkan.cpp 
              src/Engine.cpp 
              src/helpers_vulkan.cpp 
              src/ShaderObject.cpp 
              src/vma/Vma.cpp 
              src/vma/Buffer.cpp
              src/vma/Allocator.cpp)
add_dependencies(ndeex shaders)
target_include_directories(ndeex PRIVATE src)
target_link_libraries(ndeex PRIVATE Vulkan::Vulkan SDL3::SDL3-static GPUOpen::VulkanMemoryAllocator opengl32)




