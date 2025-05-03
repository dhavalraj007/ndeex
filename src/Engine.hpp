#pragma once

#include "Window.hpp"
#include <vulkan/vulkan_win32.h>
#include "helpers.hpp"

namespace Core
{
    class Engine
    {
    public:
        Engine() : window(Core::WindowCreateInfo{1024, 800, "ndeex"})
        {
            vkInstance = helpers::vulkan::create_instance("ndeex", 1, "ndeexEngine", 1, {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME}, {});
#if defined(DEBUG)
            debug_utils_messenger = vkInstance.createDebugUtilsMessengerEXT(vk::DebugUtilsMessengerCreateInfoEXT{
                .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                .pfnUserCallback = helpers::vulkan::debug_utils_messenger_callback});
#endif

            // surface creation
            surface = window.createSurface(vkInstance);

            auto [chosenDevice, chosenQueueFamiliyIndex] = helpers::vulkan::getFirstSupportedDeviceQueueSelection(vkInstance, vk::PhysicalDeviceType::eDiscreteGpu, vk::QueueFlagBits::eGraphics, surface);
            physicalDevice = chosenDevice;
            graphicsQueueFamilyIndex = chosenQueueFamiliyIndex;
            std::println("chosen physical device:{}", physicalDevice.getProperties().deviceName.data());
            std::println("found queue familyIndex:{}", graphicsQueueFamilyIndex);

            device = helpers::vulkan::create_device({physicalDevice, graphicsQueueFamilyIndex}, {VK_KHR_SWAPCHAIN_EXTENSION_NAME}, {});
            graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);

            vk::Format requiredFormat = vk::Format::eR8G8B8A8Srgb;
            if (auto surfaceFormatResult = helpers::vulkan::getSurfaceFormat(physicalDevice, surface, requiredFormat); surfaceFormatResult.has_value())
                surfaceFormat = surfaceFormatResult.value();
            else
                throw Core::runtime_error("surface format not suppported!");

            initSwapchain();
        }

        void gameloop()
        {
            while (not window.isCloseRequested())
            {
                SDL_Event *event = window.pollAndProcessEvent();
                if (!event)
                    continue;
                switch (event->type)
                {
                case SDL_EVENT_WINDOW_RESIZED:
                    windowResize((uint32_t)event->window.data1, (uint32_t)event->window.data2);
                    break;
                }
            }
        }

        void windowResize(uint32_t width, uint32_t height)
        {
            initSwapchain();
        }

    private:
        void initSwapchain()
        {
            auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);

            vk::SwapchainKHR oldSwapchain = swapchain;
            vk::SwapchainCreateInfoKHR swapchainCreateInfo{
                .surface = surface,
                .minImageCount = surfaceCaps.minImageCount + 1,
                .imageFormat = surfaceFormat.format,
                .imageColorSpace = surfaceFormat.colorSpace,
                .imageExtent = {window.getInfo().width, window.getInfo().height},
                .imageArrayLayers = 1,
                .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
                .imageSharingMode = vk::SharingMode::eExclusive,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &graphicsQueueFamilyIndex,
                .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
                .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                .presentMode = vk::PresentModeKHR::eFifo,
                .clipped = false,
                .oldSwapchain = oldSwapchain};
            swapchain = device.createSwapchainKHR(swapchainCreateInfo);

            if (oldSwapchain)
            {
                for (auto imageView : imageViews)
                {
                    device.destroyImageView(imageView);
                }
                imageViews.clear();
                device.destroySwapchainKHR(oldSwapchain);
            }

            auto swapchainImages = device.getSwapchainImagesKHR(swapchain);
            for (auto image : swapchainImages)
            {
                vk::ImageViewCreateInfo imageViewCreateInfo{
                    .image = image,
                    .viewType = vk::ImageViewType::e2D,
                    .format = surfaceFormat.format,
                    .components = {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
                    .subresourceRange = {
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1},
                };
                imageViews.push_back(device.createImageView(imageViewCreateInfo));
            }

            std::println("swapchain re/created");
        }

    private:
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
        vk::SurfaceFormatKHR surfaceFormat;
        vk::SwapchainKHR swapchain;
        std::vector<vk::ImageView> imageViews;
    };
}