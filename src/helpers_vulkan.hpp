
#pragma once
#include "Exception.hpp"
#include <filesystem>
#include <fstream>
#include <ranges>
#include <vector>

#include "Vulkan.hpp"

namespace helpers
{
namespace vulkan
{

#if defined(DEBUG)
/// @brief A debug callback called from Vulkan validation layers.
inline vk::Bool32 debug_utils_messenger_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                 vk::DebugUtilsMessageTypeFlagsEXT message_type,
                                                 const vk::DebugUtilsMessengerCallbackDataEXT *callback_data,
                                                 void *user_data)
{
    // Log debug message
    if (message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
    {
        std::println("WARNING: {} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName,
                     callback_data->pMessage);
    }
    else if (message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
    {
        std::println("ERROR: {} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName,
                     callback_data->pMessage);
    }
    return false;
}
#endif
vk::Instance create_instance(const std::string_view appName, uint32_t appVersion, const std::string_view engineName,
                             uint32_t engineVersion, std::vector<const char *> requiredInstanceExtensions,
                             std::vector<const char *> requiredInstanceLayers);

struct DeviceQueueSelection
{
    vk::PhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
};

DeviceQueueSelection getFirstSupportedDeviceQueueSelection(vk::Instance instance,
                                                           vk::PhysicalDeviceType physicalDeviceType,
                                                           vk::QueueFlags requiredFlags,
                                                           std::optional<vk::SurfaceKHR> surface = std::nullopt);

vk::Device create_device(DeviceQueueSelection deviceQueue, std::vector<const char *> requiredDeviceExtensions,
                         std::vector<const char *> requiredDeviceLayers);

std::optional<vk::SurfaceFormatKHR> getSurfaceFormat(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface,
                                                     vk::Format format);

vk::ShaderModule createShaderModule(vk::Device device, std::filesystem::path path);

vk::ShaderEXT createShaderExt(vk::Device device, std::filesystem::path path);

std::vector<vk::ShaderEXT> createShadersExt(vk::Device device, std::filesystem::path vertexShaderPath,
                                            std::filesystem::path fragShaderPath);

std::vector<vk::UniqueShaderEXT> createShadersExtUnique(vk::Device device, std::filesystem::path vertexShaderPath,
                                                        std::filesystem::path fragShaderPath);
std::pair<uint32_t, vk::DeviceSize> getMemoryIndexAndSizeForBuffer(vk::PhysicalDevice physicalDevice, vk::Device device,
                                                                   vk::Buffer buffer,
                                                                   vk::MemoryPropertyFlags requiredMemFlags);
} // namespace vulkan
} // namespace helpers