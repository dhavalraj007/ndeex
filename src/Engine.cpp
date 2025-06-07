#include "Engine.hpp"
#include "helpers_vulkan.hpp"
#include <cstring>
#include <span>

namespace Core
{

Engine::Engine() : window(Core::WindowCreateInfo{1024, 800, "ndeex"})
{
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
    graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);

    vk::Format requiredFormat = vk::Format::eR8G8B8A8Srgb;
    swapchain = Swapchain{physicalDevice, device, surface, requiredFormat};
    frameDatas = FrameDataContainer(device);
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
    // vertex buffer
    vertices = {Vertex{
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
    vertexBuffer = device.createBufferUnique(vk::BufferCreateInfo{
        .size = sizeof(Vertex) * vertices.size(),
        .usage = vk::BufferUsageFlagBits::eVertexBuffer,
        .sharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphicsQueueFamilyIndex,
    });

    auto [memIndex, allocSize] = helpers::vulkan::getMemoryIndexAndSizeForBuffer(
        physicalDevice, device, vertexBuffer.get(),
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    vertexBufferMemory = device.allocateMemoryUnique(vk::MemoryAllocateInfo{
        .allocationSize = allocSize,
        .memoryTypeIndex = memIndex,
    });

    device.bindBufferMemory(vertexBuffer.get(), vertexBufferMemory.get(), 0);
    {

        void *vertexMappedPtr = device.mapMemory(vertexBufferMemory.get(), 0, allocSize);

        std::span v = vertices;
        std::ranges::copy(std::as_writable_bytes(v), static_cast<std::byte *>(vertexMappedPtr));

        device.unmapMemory(vertexBufferMemory.get());
    }

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

void Engine::gameloop()
{
    while (not window.isCloseRequested())
    {
        processEvents();

        auto frameIndex = currentFrame % swapchain.size();
        auto &frameData = frameDatas[frameIndex];
        // wait for previous rendering on the same image be finished
        if (device.waitForFences(1, &frameData.fence_RenderFinished.get(), true, 1000) != vk::Result::eSuccess)
            throw Core::runtime_error("waitForFences failed");

        auto renderTargetIndexResult =
            swapchain.acquireNextRenderTarget(std::chrono::seconds{5}, frameData.sem_ImageAcquired.get(), {});
        if (!renderTargetIndexResult)
        {
            swapChainRecreate();
            continue;
        }

        auto renderTargetIndex = renderTargetIndexResult.value();
        // std::println("acquired Image index:{}", renderTargetIndex);
        auto &renderTarget = swapchain.getRenderTarget(renderTargetIndex);

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
        vk::ImageMemoryBarrier presentBarrier{
            .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
            .dstAccessMask = {},
            .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .newLayout = vk::ImageLayout::ePresentSrcKHR,
            .image = renderTarget.imageHandle,
            .subresourceRange = vk::ImageSubresourceRange{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                          .baseMipLevel = 0,
                                                          .levelCount = 1,
                                                          .baseArrayLayer = 0,
                                                          .layerCount = 1}};

        vk::RenderingAttachmentInfo colorAttachment{.imageView = renderTarget.imageView.get(),
                                                    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                    .loadOp = vk::AttachmentLoadOp::eClear,
                                                    .storeOp = vk::AttachmentStoreOp::eStore,
                                                    .clearValue =
                                                        vk::ClearValue{std::array<float, 4>{0.5f, 0.5f, 0.2f, 1.0f}}};

        vk::RenderingInfo renderingInfo{
            .renderArea = {{0, 0}, {window.getInfo().width, window.getInfo().height}},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
        };

        auto &commandBuffer = commandBuffers[frameIndex];
        commandBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
        commandBuffer->begin(vk::CommandBufferBeginInfo{});

        commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                       vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::DependencyFlagBits{},
                                       nullptr, nullptr, {imageBarrier});

        commandBuffer->beginRendering(renderingInfo);
        commandBuffer->bindVertexBuffers(0, vertexBuffer.get(), vk::DeviceSize(0));
        shaderObject.setState(commandBuffer.get());
        shaderObject.bind(commandBuffer.get());
        commandBuffer->draw(3, 1, 0, 0);

        commandBuffer->bindVertexBuffers(0, vertexBuffer.get(), vk::DeviceSize(0));
        shaderObject2.setState(commandBuffer.get());
        shaderObject2.bind(commandBuffer.get());
        commandBuffer->draw(3, 1, 0, 0);

        commandBuffer->endRendering();
        commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                       vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlagBits{}, nullptr,
                                       nullptr, {presentBarrier});
        commandBuffer->end();

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frameData.sem_ImageAcquired.get(),
            .pWaitDstStageMask = &waitStage,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer.get(),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &frameData.sem_RenderFinished.get(),
        };

        if (device.resetFences(1, &frameData.fence_RenderFinished.get()) != vk::Result::eSuccess)
            throw Core::runtime_error("failed to reset render fence");

        graphicsQueue.submit(submitInfo, frameData.fence_RenderFinished.get());
        swapchain.presentImage(graphicsQueue, renderTargetIndex, frameData.sem_RenderFinished.get());
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
    }
}

void Engine::swapChainRecreate()
{
    swapchain.recreate({window.getInfo().width, window.getInfo().height}, {graphicsQueueFamilyIndex});
    graphicsQueue.waitIdle();
    frameDatas.recreate(swapchain.size());
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

void Engine::FrameDataContainer::recreate(const size_t size)
{
    clear();
    for (uint32_t i = 0; i < size; ++i)
    {
        emplace_back(device.createSemaphoreUnique({}), device.createSemaphoreUnique({}),
                     device.createFenceUnique(vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}));
    }
}

Engine::FrameDataContainer::FrameDataContainer(const vk::Device device) : device(device)
{
}

} // namespace Core