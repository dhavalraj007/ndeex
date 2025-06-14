#include "Allocator.hpp"
#include "helpers_vulkan.hpp"

namespace vma
{
Allocator::Allocator(VmaAllocatorCreateInfo const &allocatorCreateInfo)
{
    VULKAN_CHECKTHROW(vmaCreateAllocator(&allocatorCreateInfo, &allocator));
}
Allocator::Allocator(Allocator &&other) noexcept : allocator(std::exchange(other.allocator, VK_NULL_HANDLE))
{
}
Allocator &Allocator::operator=(Allocator &&other) noexcept
{
    if (this != &other)
    {
        vmaDestroyAllocator(this->allocator);
        this->allocator = std::exchange(other.allocator, VK_NULL_HANDLE);
    }
    return *this;
}
Allocator::~Allocator()
{
    vmaDestroyAllocator(allocator);
}
} // namespace vma