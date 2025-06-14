#include "Buffer.hpp"
#include "helpers_vulkan.hpp"
#include <utility>

namespace vma
{

Buffer::Buffer(VmaAllocator allocator_, VkBufferCreateInfo const &bufferInfo, VmaAllocationCreateInfo const &allocInfo)
    : allocator(allocator_)
{
    if (bufferInfo.size != 0)
        VULKAN_CHECKTHROW(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr));
    size_ = bufferInfo.size;
}
Buffer::~Buffer()
{
    if (allocator)
        vmaDestroyBuffer(allocator, buffer, allocation);
}
Buffer::Buffer(Buffer &&other) noexcept
    : buffer(std::exchange(other.buffer, VK_NULL_HANDLE)), allocation(std::exchange(other.allocation, VK_NULL_HANDLE)),
      allocator(std::exchange(other.allocator, VK_NULL_HANDLE))
{
}
Buffer &Buffer::operator=(Buffer &&other) noexcept
{
    if (this != &other)
    {
        if (allocator)
            vmaDestroyBuffer(allocator, buffer, allocation);
        buffer = std::exchange(other.buffer, VK_NULL_HANDLE);
        allocation = std::exchange(other.allocation, VK_NULL_HANDLE);
        allocator = std::exchange(other.allocator, VK_NULL_HANDLE);
        size_ = std::exchange(other.size_, 0);
    }
    return *this;
}
} // namespace vma