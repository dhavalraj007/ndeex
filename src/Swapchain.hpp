#include "Vulkan.hpp"
#include <helpers.hpp>
#include <vector>

class Swapchain
{
  public:
    Swapchain() = default;
    Swapchain(vk::PhysicalDevice physicalDevice_, vk::Device device_, vk::SurfaceKHR surface_,
              vk::Format requiredFormat)
        : physicalDevice(physicalDevice_), device(device_), surface(surface_)
    {
        if (auto surfaceFormatResult = helpers::vulkan::getSurfaceFormat(physicalDevice, surface, requiredFormat);
            surfaceFormatResult.has_value())
            surfaceFormat = surfaceFormatResult.value();
        else
            throw Core::runtime_error("surface format not suppported!");
    }
    Swapchain(Swapchain const &) = delete;
    Swapchain &operator=(Swapchain const &) = delete;
    Swapchain(Swapchain &&other)
        : physicalDevice(other.physicalDevice), device(other.device), surface(other.surface),
          surfaceFormat(other.surfaceFormat), swapchain(std::move(other.swapchain)),
          renderTargets(std::move(other.renderTargets))
    {
    }
    Swapchain &operator=(Swapchain &&other)
    {
        // Clean up any existing resources first
        cleanupRenderTargets();
        swapchain.reset();

        physicalDevice = other.physicalDevice;
        device = other.device;
        surface = other.surface;
        surfaceFormat = other.surfaceFormat;
        swapchain = std::move(other.swapchain);
        renderTargets = std::move(other.renderTargets);
        return *this;
    }

    ~Swapchain()
    {
        cleanupRenderTargets();
    }

    uint32_t getRequiredImageCount()
    {
        auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        // Make sure we don't exceed maxImageCount if it's not unlimited (value of
        // 0)
        uint32_t desiredCount = surfaceCaps.minImageCount + 1;
        if (surfaceCaps.maxImageCount > 0)
        {
            desiredCount = std::min(desiredCount, surfaceCaps.maxImageCount);
        }
        return desiredCount;
    }

    void recreate(vk::Extent2D newExtent, std::vector<uint32_t> queueFamilyIndices)
    {
        auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);

        // Keep a reference to the old swapchain
        auto oldSwapchain = swapchain.get();

        auto requiredImageCount = getRequiredImageCount();
        vk::SwapchainCreateInfoKHR swapchainCreateInfo{.surface = surface,
                                                       .minImageCount = requiredImageCount,
                                                       .imageFormat = surfaceFormat.format,
                                                       .imageColorSpace = surfaceFormat.colorSpace,
                                                       .imageExtent = newExtent,
                                                       .imageArrayLayers = 1,
                                                       .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
                                                       .imageSharingMode = queueFamilyIndices.size() == 1
                                                                               ? vk::SharingMode::eExclusive
                                                                               : vk::SharingMode::eConcurrent,
                                                       .queueFamilyIndexCount = (uint32_t)queueFamilyIndices.size(),
                                                       .pQueueFamilyIndices = queueFamilyIndices.data(),
                                                       .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
                                                       .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                       .presentMode = vk::PresentModeKHR::eFifo,
                                                       .clipped = false,
                                                       .oldSwapchain = oldSwapchain};

        // Create new swapchain
        auto newSwapchain = device.createSwapchainKHRUnique(swapchainCreateInfo);

        // Clear old render targets before creating new ones
        cleanupRenderTargets();

        // Assign the new swapchain
        swapchain = std::move(newSwapchain);

        // Get the new swapchain images
        auto swapchainImages = device.getSwapchainImagesKHR(swapchain.get());

        // Resize the renderTargets vector to match the number of swapchain images
        renderTargets.resize(swapchainImages.size());

        // Create image views and semaphores for each swapchain image
        for (size_t i = 0; i < swapchainImages.size(); i++)
        {
            vk::ImageViewCreateInfo imageViewCreateInfo{
                .image = swapchainImages[i],
                .viewType = vk::ImageViewType::e2D,
                .format = surfaceFormat.format,
                .components = {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB,
                               vk::ComponentSwizzle::eA},
                .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                                     .baseMipLevel = 0,
                                     .levelCount = 1,
                                     .baseArrayLayer = 0,
                                     .layerCount = 1},
            };

            // We don't own the raw swapchain images, so just store the handle
            renderTargets[i].imageHandle = swapchainImages[i];
            renderTargets[i].imageView = device.createImageViewUnique(imageViewCreateInfo);
        }

        std::println("swapchain re/created with {} images", renderTargets.size());
    }

    struct RenderTarget
    {
        vk::Image imageHandle;         // Handle to the swapchain image (not owned)
        vk::UniqueImageView imageView; // View into the image
    };

    RenderTarget &getRenderTarget(uint32_t renderTargetIndex)
    {
        if (renderTargetIndex >= renderTargets.size())
        {
            throw Core::runtime_error("Invalid render target index: {}", renderTargetIndex);
        }
        return renderTargets[renderTargetIndex];
    }

    vk::Format getFormat() const
    {
        return surfaceFormat.format;
    }

    // if nullopt recreate swapchain needed, throws if timeout
    std::optional<uint32_t> acquireNextRenderTarget(std::chrono::nanoseconds timeout, vk::Semaphore semaphoreToSignal,
                                                    vk::Fence fenceToSignal)
    {
        if (!swapchain)
        {
            return std::nullopt; // Swapchain not created yet
        }

        uint32_t imageIndex;
        auto result =
            device.acquireNextImageKHR(swapchain.get(), timeout.count(), semaphoreToSignal, fenceToSignal, &imageIndex);

        switch (result)
        {
        case vk::Result::eErrorOutOfDateKHR:
            return std::nullopt;
        case vk::Result::eSuccess:
        case vk::Result::eSuboptimalKHR:
            return imageIndex;
        default:
            throw Core::runtime_error("acquire next image failed {}!", vk::to_string(result));
        }
    }

    void presentImage(vk::Queue presentQueue, uint32_t imageIndex, vk::Semaphore waitSemaphore)
    {
        vk::PresentInfoKHR presentInfo = {.waitSemaphoreCount = 1,
                                          .pWaitSemaphores = &waitSemaphore,
                                          .swapchainCount = 1,
                                          .pSwapchains = &swapchain.get(),
                                          .pImageIndices = &imageIndex};

        if (presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess)
            throw Core::runtime_error("present queue presentKHR failed!");
    }

    bool isValid() const
    {
        return swapchain.get() != VK_NULL_HANDLE;
    }

    std::size_t size()
    {
        return renderTargets.size();
    }

  private:
    void cleanupRenderTargets()
    {
        for (auto &renderTarget : renderTargets)
        {
            renderTarget.imageView.reset();
        }
        renderTargets.clear();
    }

    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::SurfaceKHR surface;
    vk::SurfaceFormatKHR surfaceFormat;
    std::vector<RenderTarget> renderTargets;
    vk::UniqueSwapchainKHR swapchain;
};