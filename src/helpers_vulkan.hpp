
#include <vector>
#include <ranges>
#include "Exception.hpp"
#include "Vulkan.hpp"
#define DEBUG

namespace helpers
{
    namespace vulkan
    {

#if defined(DEBUG)
        /// @brief A debug callback called from Vulkan validation layers.
        vk::Bool32 debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                  VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                  const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                  void *user_data)
        {
            // Log debug message
            if (message_severity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                std::println("WARNING: {} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            }
            else if (message_severity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                std::println("ERROR: {} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            }
            return VK_FALSE;
        }
#endif
        vk::Instance create_instance(const std::string_view appName, uint32_t appVersion, const std::string_view engineName, uint32_t engineVersion, std::vector<const char *> requiredInstanceExtensions, std::vector<const char *> requiredInstanceLayers)
        {
            static vk::DynamicLoader dl;
            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

#if defined(DEBUG)
            requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            requiredInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

            std::vector<vk::ExtensionProperties> availableExtensionProps = vk::enumerateInstanceExtensionProperties();
            for (const char *requiredExtension : requiredInstanceExtensions)
            {
                if (not std::ranges::contains(availableExtensionProps, std::string_view(requiredExtension), [](vk::ExtensionProperties const &extensionProp)
                                              { return std::string_view(extensionProp.extensionName); }))
                {
                    throw Core::runtime_error("Required Instance Extension:\"{}\" not found", std::string(requiredExtension));
                }
            }

            std::vector<vk::LayerProperties> availableLayerProps = vk::enumerateInstanceLayerProperties();
            for (const char *requiredLayer : requiredInstanceLayers)
            {
                if (not std::ranges::contains(availableLayerProps, std::string_view(requiredLayer), [](vk::LayerProperties const &layerProp)
                                              { return std::string_view(layerProp.layerName); }))
                {
                    throw Core::runtime_error("Required Instance Layer:\"{}\" not found", std::string(requiredLayer));
                }
            }

            vk::ApplicationInfo appInfo{
                .pApplicationName = appName.data(),
                .applicationVersion = appVersion,
                .pEngineName = engineName.data(),
                .engineVersion = engineVersion,
                .apiVersion = VK_API_VERSION_1_3};

            vk::InstanceCreateInfo instanceCreateInfo{
                .pApplicationInfo = &appInfo,
                .enabledLayerCount = (uint32_t)requiredInstanceLayers.size(),
                .ppEnabledLayerNames = requiredInstanceLayers.data(),
                .enabledExtensionCount = (uint32_t)requiredInstanceExtensions.size(),
                .ppEnabledExtensionNames = requiredInstanceExtensions.data()};
#if defined(DEBUG)
            vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessangerCreateInfo{
                .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                .pfnUserCallback = debug_utils_messenger_callback};

            instanceCreateInfo.pNext = &debugUtilsMessangerCreateInfo;
#endif

            vk::Instance vkInstance = vk::createInstance(instanceCreateInfo);
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkInstance);
            return vkInstance;
        }
    }
}