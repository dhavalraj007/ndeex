#pragma once
#include "Allocator.hpp"
#include "Buffer.hpp"
#include "Vma.hpp"

namespace vma
{

// allows to maintain cpu buffer and gpu buffer in sync
template <typename T> class VertexBuffer
{
  public:
    // main gpu buffer
    struct MBuffer : Buffer
    {
        MBuffer() = default;
        MBuffer(VmaAllocator allocator, vk::DeviceSize size);
        bool isStagingNeeded();

      private:
        static VkBufferCreateInfo getBufferCreateInfo(vk::DeviceSize allocSize)
        {
            return {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    .size = allocSize,
                    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};
        }
        static VmaAllocationCreateInfo getAllocationCreateInfo()
        {
            return {
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | // cpu must write it sequentially so
                                                                                  // memory chosen will be uncached and
                                                                                  // write combined flushed
                         VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | // this will try to find cpu
                                                                                        // mappable gpu access memory ,
                                                                                        // if not found we need staging
                         VMA_ALLOCATION_CREATE_MAPPED_BIT,                              // allow mapping if possible
                .usage = VMA_MEMORY_USAGE_AUTO,
            };
        }
    };

    // staging gpu buffer
    struct SBuffer : Buffer
    {
        SBuffer() = default;
        SBuffer(VmaAllocator allocator, vk::DeviceSize size);

      private:
        static VkBufferCreateInfo getBufferCreateInfo(vk::DeviceSize allocSize)
        {
            return {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    .size = allocSize,
                    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT};
        }
        static VmaAllocationCreateInfo getAllocationCreateInfo()
        {
            return {
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | // cpu must write it sequentially so
                                                                                  // memory chosen will be uncached and
                                                                                  // write combined flushed
                         VMA_ALLOCATION_CREATE_MAPPED_BIT,                        // allow mapping
                .usage = VMA_MEMORY_USAGE_AUTO,
            };
        }
    };

    VertexBuffer() = default;

    void commit(size_t firstDirtyVertexIndex, Allocator &allocator, vk::CommandBuffer cmd);

    std::vector<T> &vertices()
    {
        return cpuVertices;
    }

    vk::Buffer getBufferHandle()
    {
        return gpuBuffer.getBufferHandle();
    }

  private:
    void sendToGpu(size_t firstDirtyVertexIndex, Allocator &allocator, vk::CommandBuffer cmd);
    size_t getCpuBufferSize() const
    {
        return sizeof(T) * cpuVertices.size();
    }

    std::vector<T> cpuVertices;
    MBuffer gpuBuffer;
    std::optional<SBuffer> stagingBuffer;
};

} // namespace vma

#include "VertexBuffer.tpp"