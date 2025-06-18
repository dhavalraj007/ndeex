cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(ndeex VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)

include(cmake/CPM.cmake)
include(CMakeHelpers.cmake)


## vulkan
find_package(Vulkan REQUIRED)
## sdl
CPMAddPackage(
  NAME sdl
  GITHUB_REPOSITORY libsdl-org/SDL
  GIT_TAG release-3.2.10
        OPTIONS
        "SDL_STATIC ON"
)
## vma
CPMAddPackage(
  NAME vma
  GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
  GIT_TAG v3.3.0
  OPTIONS
    SYSTEM
)
## imgui
FetchContent_Declare(imgui_external
	URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.9.tar.gz
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui_external)
add_library(imgui
	${imgui_external_SOURCE_DIR}/imgui.cpp
  ${imgui_external_SOURCE_DIR}/imgui_demo.cpp
	${imgui_external_SOURCE_DIR}/imgui_draw.cpp
	${imgui_external_SOURCE_DIR}/imgui_tables.cpp
	${imgui_external_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  ${imgui_external_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_external_SOURCE_DIR} INTERFACE ${imgui_external_SOURCE_DIR}/backends)
target_link_libraries(imgui PRIVATE Vulkan::Vulkan SDL3::SDL3-static)

add_shaders(shaders src/triangle.vert src/triangle.frag)
add_executable(ndeex 
              src/main.cpp
              src/Vulkan.cpp 
              src/Engine.cpp 
              src/helpers_vulkan.cpp 
              src/ShaderObject.cpp 
              src/vma/Vma.cpp 
              src/vma/Buffer.cpp
              src/vma/Allocator.cpp
              src/Imgui.cpp)
add_dependencies(ndeex shaders)
target_include_directories(ndeex PRIVATE src)
target_link_libraries(ndeex PRIVATE Vulkan::Vulkan SDL3::SDL3-static GPUOpen::VulkanMemoryAllocator opengl32 imgui)




