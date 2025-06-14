#pragma once
#include "Vma.hpp"

namespace vma
{
// RAII wrapper over VmaAllocator
class Allocator
{
  public:
    Allocator() = default;
    explicit Allocator(VmaAllocatorCreateInfo const &allocatorCreateInfo);
    Allocator(Allocator const &) = delete;
    Allocator(Allocator &&) noexcept;
    Allocator &operator=(Allocator const &) = delete;
    Allocator &operator=(Allocator &&) noexcept;

    ~Allocator();

    VmaAllocator getHandle()
    {
        return allocator;
    }

  private:
    VmaAllocator allocator{};
};

} // namespace vma