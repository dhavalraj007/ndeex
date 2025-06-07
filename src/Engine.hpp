#pragma once

#include "ShaderObject.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"
#include "helpers.hpp"

namespace Core
{
class Engine
{
  public:
    Engine();
    ~Engine();
    void gameloop();

  private:
    void processEvents();
    void onWindowResize(uint32_t width, uint32_t height);

    void initCoreHandles();
    void initSwapchain();
    void initVertexBuffer();
    void initShaderObjects();

    void swapChainRecreate();

    struct FrameData
    {
        vk::UniqueSemaphore sem_ImageAcquired;
        vk::UniqueSemaphore sem_RenderFinished;
        vk::UniqueFence fence_RenderFinished;
    };

    struct FrameDataContainer : std::vector<FrameData>
    {
        vk::Device device;

        FrameDataContainer() = default;
        explicit FrameDataContainer(const vk::Device device);
        FrameDataContainer(const FrameDataContainer &) = delete;
        FrameDataContainer &operator=(const FrameDataContainer &) = delete;
        FrameDataContainer(FrameDataContainer &&other) = default;
        FrameDataContainer &operator=(FrameDataContainer &&other) = default;

        void recreate(const size_t size);
    };

    std::size_t currentFrame = 0;

    Core::Window window;
    vk::Instance vkInstance;
    VkSurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    uint32_t graphicsQueueFamilyIndex;
#if defined(DEBUG)
    vk::DebugUtilsMessengerEXT debug_utils_messenger;
#endif
    vk::Device device;
    vk::Queue graphicsQueue;
    Swapchain swapchain;
    FrameDataContainer frameDatas;
    // vk::RenderPass renderPass;
    vk::UniqueCommandPool commandPool;
    std::vector<vk::UniqueCommandBuffer> commandBuffers;
    ShaderObject shaderObject;
    ShaderObject shaderObject2;

    struct Vertex
    {
        std::array<float, 2> position;
        std::array<float, 3> color;
    };
    std::vector<Vertex> vertices;
    vk::UniqueBuffer vertexBuffer;
    vk::UniqueDeviceMemory vertexBufferMemory;
};
} // namespace Core
