#pragma once

#include <Windows.h>
#undef min
#include "Vulkan.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "Exception.hpp"
#include "helpers.hpp"
#include <expected>

namespace Core {
struct WindowCreateInfo {
  uint32_t width;
  uint32_t height;
  std::string title;
};

class Window {
public:
  Window(WindowCreateInfo wci) : info(wci), closeRequested(false) {
    SDL_CHECKTHROW(SDL_Init(SDL_INIT_VIDEO));

    window = SDL_CHECKTHROW(SDL_CreateWindow(info.title.c_str(), info.width,
                                             info.height, SDL_WINDOW_VULKAN));
    SDL_CHECKTHROW(SDL_SetWindowResizable(window, true));
  }

  ~Window() {
    SDL_DestroyWindow(window);
    SDL_Quit();
  }

  bool isCloseRequested() { return closeRequested; }

  void clearScreen() {}

  SDL_Event *pollAndProcessEvent() {
    static SDL_Event event;
    if (SDL_PollEvent(&event)) {
      processEvent(&event);
      return &event;
    }
    return nullptr;
  }

  void processEvent(SDL_Event *event) {
    switch (event->type) {
    case SDL_EVENT_QUIT:
      closeRequested = true;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      info.width = event->window.data1;
      info.height = event->window.data2;
      break;
    }
  }

  VkSurfaceKHR createSurface(vk::Instance vkInstance) {
    VkSurfaceKHR surface;
    SDL_CHECKTHROW(
        SDL_Vulkan_CreateSurface(window, vkInstance, nullptr, &surface));
    return surface;
  }

  WindowCreateInfo const &getInfo() { return info; }

  void setExtent(uint32_t width, uint32_t height) {
    info.width = width;
    info.height = height;
  }

  std::span<char const *const> getRequiredInstanceExtensions() {
    uint32_t count = 0;
    char const *const *ptr = SDL_Vulkan_GetInstanceExtensions(&count);
    return std::span{ptr, count};
  }

private:
  WindowCreateInfo info;
  SDL_Window *window;
  bool closeRequested;
};
} // namespace Core