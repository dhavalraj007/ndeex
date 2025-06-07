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
        vk::CommandBuffer commandBuffer;
        Swapchain::RenderTarget &renderTarget;
    };
    std::optional<FrameData> beginFrame();
    void endFrame(FrameData &frameData);

    struct RenderSync
    {
        vk::UniqueSemaphore sem_ImageAcquired;
        vk::UniqueSemaphore sem_RenderFinished;
        vk::UniqueFence fence_RenderFinished;
    };
    struct RenderSyncContainer : std::vector<RenderSync>
    {
        vk::Device device;

        RenderSyncContainer() = default;
        explicit RenderSyncContainer(const vk::Device device);
        RenderSyncContainer(const RenderSyncContainer &) = delete;
        RenderSyncContainer &operator=(const RenderSyncContainer &) = delete;
        RenderSyncContainer(RenderSyncContainer &&other) = default;
        RenderSyncContainer &operator=(RenderSyncContainer &&other) = default;

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
    RenderSyncContainer renderSyncs;
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
