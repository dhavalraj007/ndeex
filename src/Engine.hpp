#pragma once

#include "Imgui.hpp"
#include "ShaderObject.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"
#include "vma/VertexBuffer.hpp"

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
    void initVMA();
    void initSwapchain();
    void initImGui();
    void initVertexBuffer();
    void initShaderObjects();

    void swapChainRecreate();

    Swapchain::RenderTarget *acquireRenderTarget(std::chrono::milliseconds timeout = std::chrono::seconds{1});
    void beginRecording(vk::CommandBuffer cmd);
    void transitionToRender(vk::CommandBuffer cmd, Swapchain::RenderTarget &renderTarget);
    void beginRendering(vk::CommandBuffer cmd, Swapchain::RenderTarget &renderTarget);
    void endRendering(vk::CommandBuffer cmd);
    void transitionToPresent(vk::CommandBuffer cmd, Swapchain::RenderTarget &renderTarget);
    void stopRecording(vk::CommandBuffer cmd);
    void submitToQueue(vk::CommandBuffer cmd);
    void present(Swapchain::RenderTarget &renderTarget);

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

    RenderSync &getFrameRenderSync()
    {
        return renderSyncs.at(currentFrame % swapchain.size());
    }
    RenderSync &getPrevFrameRenderSync()
    {
        return renderSyncs.at((currentFrame + swapchain.size() - 1) % swapchain.size());
    }
    vk::CommandBuffer getFrameCommandBuffer()
    {
        return commandBuffers.at(currentFrame % swapchain.size()).get();
    }
    vk::CommandBuffer getPrevFrameCommandBuffer()
    {
        return commandBuffers.at((currentFrame + swapchain.size() - 1) % swapchain.size()).get();
    }

    std::size_t currentFrame = 0;

    vk::ClearValue clearColor{};
    vk::detail::DynamicLoader dl;
    vma::Allocator allocator;
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
    DearImgui imgui;
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
    vma::VertexBuffer<Vertex> vertexBuffer;
    vk::UniqueDeviceMemory vertexBufferMemory;
};
} // namespace Core
