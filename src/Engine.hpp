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
        }

        void gameloop()
        {
            while (not window.closeRequested())
            {
                window.pollAndrocessEvents();
            }
        }

    private:
        Core::Window window;
        vk::Instance vkInstance;
#if defined(DEBUG)
        vk::DebugUtilsMessengerEXT debug_utils_messenger;
#endif
    };
}