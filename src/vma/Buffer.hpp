#pragma once
#include "Vma.hpp"

namespace vma
{
// RAII buffer with allocation
class Buffer
{
  public:
    Buffer() = default;
    Buffer(VmaAllocator allocator, VkBufferCreateInfo const &buffer, VmaAllocationCreateInfo const &allocInfo);
    Buffer(Buffer const &) = delete;
    Buffer(Buffer &&) noexcept;
    Buffer &operator=(Buffer const &) = delete;
    Buffer &operator=(Buffer &&) noexcept;
    ~Buffer();

    VkBuffer getBufferHandle()
    {
        return buffer;
    }
    VmaAllocation getAllocationHandle()
    {
        return allocation;
    }
    VmaAllocator getAllocatorHandle()
    {
        return allocator;
    }
    vk::DeviceSize size()
    {
        return size_;
    }

  protected:
    VkBuffer buffer{};
    VmaAllocation allocation{};
    VmaAllocator allocator{};
    vk::DeviceSize size_{};
};
} // namespace vma