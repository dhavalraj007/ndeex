#pragma once
#include "Vulkan.hpp"
#include "imgui.h"
#include <SDL3/SDL.h>

class DearImgui
{
  public:
    struct InitInfo
    {
        vk::Instance instance;
        vk::PhysicalDevice physicalDevice;
        vk::Device device;
        uint32_t graphicsQueueFamilyIndex;
        vk::Queue graphicsQueue;
        uint32_t imageCount;
        vk::SampleCountFlags sampleCount;
        uint32_t descriptorPoolSize;
        SDL_Window *window;
        vk::Format format;
    };

    DearImgui() = default;
    void init(InitInfo const &);

    DearImgui(DearImgui &) = delete;
    DearImgui(DearImgui &&) = delete;
    DearImgui &operator=(DearImgui const &) = delete;
    DearImgui &operator=(DearImgui &&) = delete;
    ~DearImgui();

    void newFrame();
    void render(vk::CommandBuffer cmd);

  private:
    ImGuiContext *context = nullptr;
};