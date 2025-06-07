#include "helpers_vulkan.hpp"
#include "Exception.hpp"
#include <cstdint>
#include <filesystem>
#include <utility>

vk::Instance helpers::vulkan::create_instance(const std::string_view appName, uint32_t appVersion,
                                              const std::string_view engineName, uint32_t engineVersion,
                                              std::vector<const char *> requiredInstanceExtensions,
                                              std::vector<const char *> requiredInstanceLayers)
{
    static vk::detail::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

#if defined(DEBUG)
    requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    requiredInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    std::vector<vk::ExtensionProperties> availableExtensionProps = vk::enumerateInstanceExtensionProperties();
    for (const char *requiredExtension : requiredInstanceExtensions)
    {
        if (not std::ranges::contains(availableExtensionProps, std::string_view(requiredExtension),
                                      [](vk::ExtensionProperties const &extensionProp) {
                                          return std::string_view(extensionProp.extensionName);
                                      }))
        {
            throw Core::runtime_error("Required Instance Extension:\"{}\" not found", requiredExtension);
        }
    }

    std::vector<vk::LayerProperties> availableLayerProps = vk::enumerateInstanceLayerProperties();
    for (const char *requiredLayer : requiredInstanceLayers)
    {
        if (not std::ranges::contains(
                availableLayerProps, std::string_view(requiredLayer),
                [](vk::LayerProperties const &layerProp) { return std::string_view(layerProp.layerName); }))
        {
            throw Core::runtime_error("Required Instance Layer:\"{}\" not found", requiredLayer);
        }
    }

    vk::ApplicationInfo appInfo{.pApplicationName = appName.data(),
                                .applicationVersion = appVersion,
                                .pEngineName = engineName.data(),
                                .engineVersion = engineVersion,
                                .apiVersion = VK_API_VERSION_1_3};

    vk::InstanceCreateInfo instanceCreateInfo{.pApplicationInfo = &appInfo,
                                              .enabledLayerCount = (uint32_t)requiredInstanceLayers.size(),
                                              .ppEnabledLayerNames = requiredInstanceLayers.data(),
                                              .enabledExtensionCount = (uint32_t)requiredInstanceExtensions.size(),
                                              .ppEnabledExtensionNames = requiredInstanceExtensions.data()};
#if defined(DEBUG)
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessangerCreateInfo{
        .messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
        .messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = debug_utils_messenger_callback};

    instanceCreateInfo.pNext = &debugUtilsMessangerCreateInfo;
#endif

    vk::Instance vkInstance = vk::createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkInstance);
    return vkInstance;
}
helpers::vulkan::DeviceQueueSelection helpers::vulkan::getFirstSupportedDeviceQueueSelection(
    vk::Instance instance, vk::PhysicalDeviceType physicalDeviceType, vk::QueueFlags requiredFlags,
    std::optional<vk::SurfaceKHR> surface)
{
    auto physicalDevices = instance.enumeratePhysicalDevices();
    for (auto physicalDevice : instance.enumeratePhysicalDevices())
    {
        if (physicalDevice.getProperties().deviceType != physicalDeviceType)
            continue;

        auto queueProperties = physicalDevice.getQueueFamilyProperties();
        for (uint32_t queueFamilyIndex = 0; auto queueProp : queueProperties)
        {
            if (queueProp.queueFlags & requiredFlags)
            {
                bool surfacePresentSupport =
                    surface
                        .transform([&](vk::SurfaceKHR surface) {
                            return physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, surface);
                        })
                        .value_or(true);
                if (surfacePresentSupport)
                    return DeviceQueueSelection{physicalDevice, queueFamilyIndex};
            }
            queueFamilyIndex++;
        }
    }

    throw Core::runtime_error("no physical device and queue combination found!");
}
vk::Device helpers::vulkan::create_device(DeviceQueueSelection deviceQueue,
                                          std::vector<const char *> requiredDeviceExtensions,
                                          std::vector<const char *> requiredDeviceLayers)
{
    std::vector<vk::ExtensionProperties> availableExtensionProps =
        deviceQueue.physicalDevice.enumerateDeviceExtensionProperties();
    for (const char *requiredExtension : requiredDeviceExtensions)
    {
        if (not std::ranges::contains(availableExtensionProps, std::string_view(requiredExtension),
                                      [](vk::ExtensionProperties const &extensionProp) {
                                          return std::string_view(extensionProp.extensionName);
                                      }))
        {
            throw Core::runtime_error("Required Device Extension:\"{}\" not found", requiredExtension);
        }
    }

    std::vector<vk::LayerProperties> availableLayersProps = deviceQueue.physicalDevice.enumerateDeviceLayerProperties();
    for (const char *requiredLayer : requiredDeviceLayers)
    {
        if (not std::ranges::contains(
                availableExtensionProps, std::string_view(requiredLayer),
                [](vk::ExtensionProperties const &layerProp) { return std::string_view(layerProp.extensionName); }))
        {
            throw Core::runtime_error("Required Device Layer:\"{}\" not found", requiredLayer);
        }
    }

    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = deviceQueue.queueFamilyIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
    vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{.dynamicRendering = true};
    vk::PhysicalDeviceShaderObjectFeaturesEXT shaderObjFeatures{.pNext = &dynamicRenderingFeatures,
                                                                .shaderObject = true};
    vk::DeviceCreateInfo deviceCreateInfo{.pNext = &shaderObjFeatures,
                                          .queueCreateInfoCount = 1,
                                          .pQueueCreateInfos = &queueCreateInfo,
                                          .enabledLayerCount = (uint32_t)requiredDeviceLayers.size(),
                                          .ppEnabledLayerNames = requiredDeviceLayers.data(),
                                          .enabledExtensionCount = (uint32_t)requiredDeviceExtensions.size(),
                                          .ppEnabledExtensionNames = requiredDeviceExtensions.data()};
    vk::Device device = deviceQueue.physicalDevice.createDevice(deviceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
    return device;
}
std::optional<vk::SurfaceFormatKHR> helpers::vulkan::getSurfaceFormat(vk::PhysicalDevice physicalDevice,
                                                                      vk::SurfaceKHR surface, vk::Format format)
{
    auto supportedSurfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    auto it = std::ranges::find_if(supportedSurfaceFormats,
                                   [&](vk::SurfaceFormatKHR &surfaceFormat) { return surfaceFormat.format == format; });
    if (it == supportedSurfaceFormats.end())
    {
        return std::nullopt;
    }
    return *it;
}

std::vector<uint32_t> getSpirvShaderCode(std::filesystem::path path)
{
    std::ifstream file{path, std::ios::binary};
    auto pathStr = path.string();
    auto str = std::filesystem::current_path().string();
    if (!file)
    {
        throw Core::runtime_error("error with file:{} ", pathStr);
    }

    auto codeSize = std::filesystem::file_size(path);
    assert(codeSize % 4 == 0);
    std::vector<uint32_t> code;
    code.resize(codeSize / sizeof(uint32_t));
    file.read(reinterpret_cast<char *>(code.data()), codeSize);
    return code;
}

vk::ShaderModule helpers::vulkan::createShaderModule(vk::Device device, std::filesystem::path path)
{
    auto code = getSpirvShaderCode(path);
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo{.codeSize = code.size() * sizeof(uint32_t), .pCode = code.data()};

    return device.createShaderModule(shaderModuleCreateInfo);
};

vk::ShaderEXT helpers::vulkan::createShaderExt(vk::Device device, std::filesystem::path path)
{
    auto code = getSpirvShaderCode(path);
    auto pathStr = path.string();
    vk::ShaderCreateInfoEXT shaderCreateInfoExt{.stage = vk::ShaderStageFlagBits::eVertex,
                                                .nextStage = vk::ShaderStageFlagBits::eFragment,
                                                .codeType = vk::ShaderCodeTypeEXT::eSpirv,
                                                .codeSize = code.size() * sizeof(uint32_t),
                                                .pCode = code.data(),
                                                .pName = "main"};

    auto res = device.createShadersEXT(shaderCreateInfoExt);
    if (res.result != vk::Result::eSuccess)
    {
        throw Core::runtime_error("createShadersEXT failed for shader:{} with:{}", pathStr.c_str(),
                                  vk::to_string(res.result));
    }
    return res.value.at(0);
};

std::vector<vk::ShaderEXT> helpers::vulkan::createShadersExt(vk::Device device, std::filesystem::path vertexShaderPath,
                                                             std::filesystem::path fragShaderPath)
{
    auto vertexCode = getSpirvShaderCode(vertexShaderPath);
    auto vertexPathStr = vertexShaderPath.string();
    auto fragCode = getSpirvShaderCode(fragShaderPath);
    auto fragPathStr = fragShaderPath.string();

    vk::ShaderCreateInfoEXT vertexShaderCreateInfoExt{.stage = vk::ShaderStageFlagBits::eVertex,
                                                      .nextStage = vk::ShaderStageFlagBits::eFragment,
                                                      .codeType = vk::ShaderCodeTypeEXT::eSpirv,
                                                      .codeSize = vertexCode.size() * sizeof(uint32_t),
                                                      .pCode = vertexCode.data(),
                                                      .pName = "main"};
    vk::ShaderCreateInfoEXT fragShaderCreateInfoExt{.stage = vk::ShaderStageFlagBits::eFragment,
                                                    .nextStage = {},
                                                    .codeType = vk::ShaderCodeTypeEXT::eSpirv,
                                                    .codeSize = fragCode.size() * sizeof(uint32_t),
                                                    .pCode = fragCode.data(),
                                                    .pName = "main"};

    auto res = device.createShadersEXT({vertexShaderCreateInfoExt, fragShaderCreateInfoExt});
    if (res.result != vk::Result::eSuccess)
    {
        throw Core::runtime_error("createShadersEXT failed for:{},{} with:{}", vertexPathStr, fragPathStr,
                                  vk::to_string(res.result));
    }
    return res.value;
};

std::vector<vk::UniqueShaderEXT> helpers::vulkan::createShadersExtUnique(vk::Device device,
                                                                         std::filesystem::path vertexShaderPath,
                                                                         std::filesystem::path fragShaderPath)
{
    auto vertexCode = getSpirvShaderCode(vertexShaderPath);
    auto vertexPathStr = vertexShaderPath.string();
    auto fragCode = getSpirvShaderCode(fragShaderPath);
    auto fragPathStr = fragShaderPath.string();

    vk::ShaderCreateInfoEXT vertexShaderCreateInfoExt{.stage = vk::ShaderStageFlagBits::eVertex,
                                                      .nextStage = vk::ShaderStageFlagBits::eFragment,
                                                      .codeType = vk::ShaderCodeTypeEXT::eSpirv,
                                                      .codeSize = vertexCode.size() * sizeof(uint32_t),
                                                      .pCode = vertexCode.data(),
                                                      .pName = "main"};
    vk::ShaderCreateInfoEXT fragShaderCreateInfoExt{.stage = vk::ShaderStageFlagBits::eFragment,
                                                    .nextStage = {},
                                                    .codeType = vk::ShaderCodeTypeEXT::eSpirv,
                                                    .codeSize = fragCode.size() * sizeof(uint32_t),
                                                    .pCode = fragCode.data(),
                                                    .pName = "main"};

    auto res = device.createShadersEXTUnique({vertexShaderCreateInfoExt, fragShaderCreateInfoExt});
    if (res.result != vk::Result::eSuccess)
    {
        throw Core::runtime_error("createShadersEXT failed for:{},{} with:{}", vertexPathStr, fragPathStr,
                                  vk::to_string(res.result));
    }
    return std::move(res.value);
};

std::pair<uint32_t, vk::DeviceSize> helpers::vulkan::getMemoryIndexAndSizeForBuffer(
    vk::PhysicalDevice physicalDevice, vk::Device device, vk::Buffer buffer, vk::MemoryPropertyFlags requiredMemFlags)
{
    auto bufferMemRequirement = device.getBufferMemoryRequirements(buffer);
    auto memProps = physicalDevice.getMemoryProperties();
    for (int i = 0; i < memProps.memoryTypeCount; i++)
    {
        if (bufferMemRequirement.memoryTypeBits & (1 << i) /* buffer likes the ith mem type */ and
            ((memProps.memoryTypes[i].propertyFlags & requiredMemFlags) ==
             requiredMemFlags) /*ith mem type supports required flags*/)
        {
            return {i, bufferMemRequirement.size};
        }
    }

    throw Core::runtime_error(
        "couldn't find the required memory for buffer, requiredMemFlags:{} , bufferMemRequirement.memoryTypeBits:{}",
        vk::to_string(requiredMemFlags), bufferMemRequirement.memoryTypeBits);
}
