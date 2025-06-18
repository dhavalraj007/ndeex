#include "Engine.hpp"
#include "Imgui.hpp"
#include "helpers.hpp"
#include "helpers_vulkan.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <optional>
#include <print>
#include <vk_mem_alloc.h>

namespace Core
{

Engine::Engine() : window(Core::WindowCreateInfo{1024, 800, "ndeex"})
{
    initCoreHandles();
    initVMA();
    initSwapchain();
    initImGui();
    initVertexBuffer();
    initShaderObjects();
    clearColor = vk::ClearValue{std::array<float, 4>{0.5f, 0.2f, 0.2f, 1.0f}};
}

void Engine::initCoreHandles()
{
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    std::vector<const char *> requiredExtensions;
    requiredExtensions.append_range(window.getRequiredInstanceExtensions());
    vkInstance = helpers::vulkan::create_instance("ndeex", 1, "ndeexEngine", 1, requiredExtensions, {});
#if defined(DEBUG)
    debug_utils_messenger = vkInstance.createDebugUtilsMessengerEXT(
        vk::DebugUtilsMessengerCreateInfoEXT{.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                                             .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                                             .pfnUserCallback = helpers::vulkan::debug_utils_messenger_callback});
#endif

    // surface creation
    surface = window.createSurface(vkInstance);

    auto [chosenDevice, chosenQueueFamiliyIndex] = helpers::vulkan::getFirstSupportedDeviceQueueSelection(
        vkInstance, vk::PhysicalDeviceType::eDiscreteGpu, vk::QueueFlagBits::eGraphics, surface);
    physicalDevice = chosenDevice;
    graphicsQueueFamilyIndex = chosenQueueFamiliyIndex;
    std::println("chosen physical device:{}", physicalDevice.getProperties().deviceName.data());
    std::println("found queue familyIndex:{}", graphicsQueueFamilyIndex);

    device = helpers::vulkan::create_device(
        {physicalDevice, graphicsQueueFamilyIndex},
        {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_EXT_SHADER_OBJECT_EXTENSION_NAME},
        {});
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);
}

void Engine::initVMA()
{
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_4;
    allocatorCreateInfo.physicalDevice = physicalDevice;
    allocatorCreateInfo.device = device;
    allocatorCreateInfo.instance = vkInstance;

    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    allocator = vma::Allocator{allocatorCreateInfo};
}

void Engine::initSwapchain()
{
    vk::Format requiredFormat = vk::Format::eR8G8B8A8Srgb;
    swapchain = Swapchain{physicalDevice, device, surface, requiredFormat};
    renderSyncs = RenderSyncContainer(device);
    swapChainRecreate();

    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueFamilyIndex,
    };
    commandPool = device.createCommandPoolUnique(poolInfo);
    commandBuffers = device.allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo{.commandPool = commandPool.get(),
                                      .level = vk::CommandBufferLevel::ePrimary,
                                      .commandBufferCount = static_cast<uint32_t>(swapchain.size())});
}

void Engine::initImGui()
{
    imgui.init(DearImgui::InitInfo{
        .instance = vkInstance,
        .physicalDevice = physicalDevice,
        .device = device,
        .graphicsQueueFamilyIndex = graphicsQueueFamilyIndex,
        .graphicsQueue = graphicsQueue,
        .imageCount = 2,
        .sampleCount = vk::SampleCountFlagBits::e1,
        .descriptorPoolSize = 2,
        .window = window.getHandle(),
        .format = swapchain.getFormat(),
    });
}

void Engine::initVertexBuffer()
{
    // vertex buffer
    vertexBuffer.vertices() = {Vertex{
                                   .position = {-0.5, -0.5},
                                   .color = {1.0, 0.0, 0.0},
                               },
                               Vertex{
                                   .position = {0.5, -0.5},
                                   .color = {0.0, 1.0, 0.0},
                               },
                               Vertex{
                                   .position = {0.0, 0.5},
                                   .color = {0.0, 0.0, 1.0},
                               }};
}

void Engine::initShaderObjects()
{
    // shader object setup
    shaderObject = ShaderObject(device, "triangle.vert.spv", "triangle.frag.spv");
    shaderObject2 = ShaderObject(device, "triangle.vert.spv", "triangle.frag.spv");
    shaderObject.setViewport({.x = 0,
                              .y = 0,
                              .width = static_cast<float>(window.getInfo().width),
                              .height = static_cast<float>(window.getInfo().height)});
    shaderObject.setScissor(vk::Rect2D{.offset{.x = 0, .y = 0},
                                       .extent = {.width = window.getInfo().width, .height = window.getInfo().height}});

    shaderObject2.setViewport({.x = 0,
                               .y = static_cast<float>(window.getInfo().height),
                               .width = static_cast<float>(window.getInfo().width),
                               .height = -static_cast<float>(window.getInfo().height)});
    shaderObject2.setScissor(vk::Rect2D{
        .offset{.x = 0, .y = 0}, .extent = {.width = window.getInfo().width, .height = window.getInfo().height}});

    shaderObject.setColorBlendEnable(0, false);
    shaderObject2.setColorBlendEnable(0, false);

    // shader vertex inputs
    shaderObject.vertexBindings().push_back(vk::VertexInputBindingDescription2EXT{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex,
        .divisor = 1,
    });
    shaderObject.attributeDescriptions().push_back(vk::VertexInputAttributeDescription2EXT{
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(Vertex, position),
    });
    shaderObject.attributeDescriptions().push_back(vk::VertexInputAttributeDescription2EXT{
        .location = 1,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, color),
    });

    shaderObject2.vertexBindings() = shaderObject.vertexBindings();
    shaderObject2.attributeDescriptions() = shaderObject.attributeDescriptions();
}

Engine::~Engine()
{
    device.waitIdle();
}

Swapchain::RenderTarget *Engine::acquireRenderTarget(std::chrono::milliseconds timeout)
{
    auto &renderSync = getFrameRenderSync();
    // wait for previous rendering on the same image be finished
    VULKAN_CHECKTHROW(device.waitForFences(1, &renderSync.fence_RenderFinished.get(), true, 1000));
    VULKAN_CHECKTHROW(device.resetFences(1, &renderSync.fence_RenderFinished.get()));

    std::optional<uint32_t> renderTargetIndexResult{};

    auto end = std::chrono::system_clock::now() + timeout;
    while (std::chrono::system_clock::now() <= end)
    {
        renderTargetIndexResult =
            swapchain.acquireNextRenderTarget(std::chrono::seconds{5}, renderSync.sem_ImageAcquired.get(), {});
        if (!renderTargetIndexResult)
        {
            swapChainRecreate();
            continue;
        }
        break;
    }

    if (renderTargetIndexResult)
        return &swapchain.getRenderTarget(renderTargetIndexResult.value());
    else
        return nullptr;
}

void Engine::beginRecording(vk::CommandBuffer cmd)
{
    cmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    cmd.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
}

void Engine::transitionToRender(vk::CommandBuffer cmd, Swapchain::RenderTarget &renderTarget)
{
    vk::ImageMemoryBarrier imageBarrier{.srcAccessMask = vk::AccessFlagBits::eNone,
                                        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
                                        .oldLayout = vk::ImageLayout::eUndefined,
                                        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                        .srcQueueFamilyIndex = graphicsQueueFamilyIndex,
                                        .dstQueueFamilyIndex = graphicsQueueFamilyIndex,
                                        .image = renderTarget.imageHandle,
                                        .subresourceRange =
                                            vk::ImageSubresourceRange{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                                      .baseMipLevel = 0,
                                                                      .levelCount = 1,
                                                                      .baseArrayLayer = 0,
                                                                      .layerCount = 1}};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::DependencyFlagBits{}, nullptr, nullptr, {imageBarrier});
}

void Engine::beginRendering(vk::CommandBuffer cmd, Swapchain::RenderTarget &renderTarget)
{
    vk::RenderingAttachmentInfo colorAttachment{.imageView = renderTarget.imageView.get(),
                                                .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                .loadOp = vk::AttachmentLoadOp::eClear,
                                                .storeOp = vk::AttachmentStoreOp::eStore,
                                                .clearValue = clearColor};

    vk::RenderingInfo renderingInfo{
        .renderArea = {{0, 0}, {window.getInfo().width, window.getInfo().height}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
    };

    cmd.beginRendering(renderingInfo);
}
void Engine::endRendering(vk::CommandBuffer cmd)
{
    cmd.endRendering();
}
void Engine::transitionToPresent(vk::CommandBuffer cmd, Swapchain::RenderTarget &renderTarget)
{
    vk::ImageMemoryBarrier presentBarrier{.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
                                          .dstAccessMask = {},
                                          .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                          .newLayout = vk::ImageLayout::ePresentSrcKHR,
                                          .image = renderTarget.imageHandle,
                                          .subresourceRange =
                                              vk::ImageSubresourceRange{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                                        .baseMipLevel = 0,
                                                                        .levelCount = 1,
                                                                        .baseArrayLayer = 0,
                                                                        .layerCount = 1}};

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
                        vk::DependencyFlagBits{}, nullptr, nullptr, {presentBarrier});
}
void Engine::stopRecording(vk::CommandBuffer cmd)
{
    cmd.end();
}
void Engine::submitToQueue(vk::CommandBuffer cmd)
{
    auto &renderSync = getFrameRenderSync();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderSync.sem_ImageAcquired.get(),
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderSync.sem_RenderFinished.get(),
    };

    graphicsQueue.submit(submitInfo, renderSync.fence_RenderFinished.get());
}

void Engine::present(Swapchain::RenderTarget &renderTarget)
{
    auto &renderSync = getFrameRenderSync();
    swapchain.presentImage(graphicsQueue, renderTarget.imageIndex, renderSync.sem_RenderFinished.get());
}

void Engine::gameloop()
{
    while (not window.isCloseRequested())
    {
        processEvents();

        auto &renderSync = getFrameRenderSync();
        // test change vertex data
        auto updateVertexPosition = [](Vertex &v, uint32_t frameCount, float radius = 0.5f) {
            float angle = frameCount * 0.05f; // Radians per frame
            v.position[0] = radius * std::cos(angle);
            v.position[1] = radius * std::sin(angle);
            v.position[2] = 0.0f; // Keep on XY plane
        };

        bool updateVertexBuffer = true;

        auto &renderTarget = *CHECKTHROW(acquireRenderTarget());
        auto cmd = getFrameCommandBuffer();

        imgui.newFrame();
        {
            beginRecording(cmd);
            transitionToRender(cmd, renderTarget);

            if (updateVertexBuffer)
            {
                VULKAN_CHECKTHROW(
                    device.waitForFences(getPrevFrameRenderSync().fence_RenderFinished.get(), true, UINT64_MAX));
                vertexBuffer.commit(0, allocator, cmd);
            }
            {
                beginRendering(cmd, renderTarget);

                cmd.bindVertexBuffers(0, vertexBuffer.getBufferHandle(), vk::DeviceSize(0));
                shaderObject.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleFan);
                shaderObject.setState(cmd);
                shaderObject.bind(cmd);
                cmd.draw(vertexBuffer.vertices().size(), 1, 0, 0);

                cmd.bindVertexBuffers(0, vertexBuffer.getBufferHandle(), vk::DeviceSize(0));
                shaderObject2.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleFan);
                shaderObject2.setState(cmd);
                shaderObject2.bind(cmd);
                cmd.draw(vertexBuffer.vertices().size(), 1, 0, 0);
                endRendering(cmd);
            }

            {

                vk::RenderingAttachmentInfo colorAttachment{.imageView = renderTarget.imageView.get(),
                                                            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                            .loadOp = vk::AttachmentLoadOp::eLoad,
                                                            .storeOp = vk::AttachmentStoreOp::eStore,
                                                            .clearValue = clearColor};

                vk::RenderingInfo renderingInfo{
                    .renderArea = {{0, 0}, {window.getInfo().width, window.getInfo().height}},
                    .layerCount = 1,
                    .colorAttachmentCount = 1,
                    .pColorAttachments = &colorAttachment,
                };

                ImGui::ShowDemoWindow();
                ImGui::Begin("control");
                for (std::string str = "pos 0"; auto &vertex : vertexBuffer.vertices())
                {
                    ImGui::SliderFloat2(str.c_str(), vertex.position.data(), -2.f, +2.f);
                    str.back()++;
                }
                ImGui::End();

                if (updateVertexBuffer)
                {
                    VULKAN_CHECKTHROW(
                        device.waitForFences(getPrevFrameRenderSync().fence_RenderFinished.get(), true, UINT64_MAX));
                    vertexBuffer.commit(0, allocator, cmd);
                }

                cmd.beginRendering(renderingInfo);
                imgui.render(cmd);
                cmd.endRendering();
            }
            transitionToPresent(cmd, renderTarget);
            stopRecording(cmd);
        }

        submitToQueue(cmd);
        present(renderTarget);
        currentFrame++;
    }
}

void Engine::processEvents()
{
    SDL_Event *event;
    while ((event = window.pollAndProcessEvent()))
    {
        switch (event->type)
        {
        case SDL_EVENT_WINDOW_RESIZED:
            onWindowResize((uint32_t)event->window.data1, (uint32_t)event->window.data2);
            break;
        default:
            break;
        }
        ImGui_ImplSDL3_ProcessEvent(event);
    }
}

void Engine::swapChainRecreate()
{
    swapchain.recreate({window.getInfo().width, window.getInfo().height}, {graphicsQueueFamilyIndex});
    graphicsQueue.waitIdle();
    renderSyncs.recreate(swapchain.size());
}

void Engine::onWindowResize(uint32_t width, uint32_t height)
{
    swapChainRecreate();
    shaderObject.setViewport(
        {.x = 0, .y = 0, .width = static_cast<float>(width), .height = static_cast<float>(height)});
    shaderObject.setScissor(vk::Rect2D{.offset{.x = 0, .y = 0}, .extent = {.width = width, .height = height}});
    shaderObject2.setViewport({.x = 0,
                               .y = static_cast<float>(height),
                               .width = static_cast<float>(width),
                               .height = -static_cast<float>(height)});
    shaderObject2.setScissor(vk::Rect2D{.offset{.x = 0, .y = 0}, .extent = {.width = width, .height = height}});
}

Engine::RenderSyncContainer::RenderSyncContainer(const vk::Device device) : device(device)
{
}

void Engine::RenderSyncContainer::recreate(const size_t size)
{
    clear();
    for (uint32_t i = 0; i < size; ++i)
    {
        emplace_back(device.createSemaphoreUnique({}), device.createSemaphoreUnique({}),
                     device.createFenceUnique(vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}));
    }
}

} // namespace Core
