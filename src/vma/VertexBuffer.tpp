#pragma once
#include "VertexBuffer.hpp"
#include <print>

namespace vma
{

template <typename T>
VertexBuffer<T>::MBuffer::MBuffer(VmaAllocator allocator, vk::DeviceSize size)
    : Buffer(allocator, getBufferCreateInfo(size), getAllocationCreateInfo())
{
}
template <typename T> bool VertexBuffer<T>::MBuffer::isStagingNeeded()
{
    VkMemoryPropertyFlags memPropFlags;
    vmaGetAllocationMemoryProperties(allocator, allocation, &memPropFlags);
    return !(memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
}

template <typename T>
VertexBuffer<T>::SBuffer::SBuffer(VmaAllocator allocator, vk::DeviceSize size)
    : Buffer(allocator, getBufferCreateInfo(size), getAllocationCreateInfo())
{
}

template <typename T>
void VertexBuffer<T>::commit(size_t firstDirtyVertexIndex, Allocator &allocator, vk::CommandBuffer cmd)
{
    if (firstDirtyVertexIndex >= cpuVertices.size())
        return;

    if (getCpuBufferSize() > gpuBuffer.size()) // cpu buffer bigger than gpu buffer -> needs reallocation
    {
        std::println("reallocating gpu buffer from {} to {}", gpuBuffer.size(), getCpuBufferSize());
        gpuBuffer = MBuffer(allocator.getHandle(), getCpuBufferSize()); // reallocate gpu buffer
        sendToGpu(firstDirtyVertexIndex, allocator, cmd);
    }
    else
    {
        sendToGpu(firstDirtyVertexIndex, allocator, cmd);
    }
}
template <typename T>
void VertexBuffer<T>::sendToGpu(size_t firstDirtyVertexIndex, Allocator &allocator, vk::CommandBuffer cmd)
{
    // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
    if (gpuBuffer.isStagingNeeded())
    {
        std::println("using staging buffer");
        if (!stagingBuffer or getCpuBufferSize() > stagingBuffer.value().size()) // reallocate staging buffer if needed
        {
            stagingBuffer = SBuffer(allocator.getHandle(), gpuBuffer.size());
        }

        void *cpuDataPtr = &cpuVertices[firstDirtyVertexIndex];
        VkDeviceSize gpuOffsetBytes = firstDirtyVertexIndex * sizeof(T);
        VkDeviceSize copySizeBytes = getCpuBufferSize() - gpuOffsetBytes;

        VULKAN_CHECKTHROW(vmaCopyMemoryToAllocation(allocator.getHandle(), cpuDataPtr,
                                                    stagingBuffer.value().getAllocationHandle(), gpuOffsetBytes,
                                                    copySizeBytes));

        // Calling vmaCopyMemoryToAllocation() does vmaMapMemory(), memcpy(), vmaUnmapMemory(), and
        // vmaFlushAllocation()
        vk::BufferMemoryBarrier cpuWToStageBufRBarrier{
            .srcAccessMask = vk::AccessFlagBits::eHostWrite,
            .dstAccessMask = vk::AccessFlagBits::eTransferRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = stagingBuffer.value().getBufferHandle(),
            .offset = gpuOffsetBytes,
            .size = copySizeBytes,
        };
        // Insert a buffer memory barrier to make sure cpu writing to the staging buffer has finished.
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, {},
                            cpuWToStageBufRBarrier, {});

        cmd.copyBuffer(stagingBuffer.value().getBufferHandle(), gpuBuffer.getBufferHandle(),
                       vk::BufferCopy{.srcOffset = gpuOffsetBytes, .dstOffset = gpuOffsetBytes, .size = copySizeBytes});

        vk::BufferMemoryBarrier stageBufWToGpuBufRBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = gpuBuffer.getBufferHandle(),
            .offset = gpuOffsetBytes,
            .size = copySizeBytes,
        };

        // Make sure copying from staging buffer to the actual buffer has finished by inserting a buffer memory
        // barrier.
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput, {}, {},
                            stageBufWToGpuBufRBarrier, {});
    }
    else
    {
        // The Allocation ended up in a mappable memory.

        // Calling vmaCopyMemoryToAllocation() does vmaMapMemory(), memcpy(), vmaUnmapMemory(), and
        // vmaFlushAllocation()
        void *cpuDataPtr = &cpuVertices[firstDirtyVertexIndex];
        VkDeviceSize gpuOffsetBytes = firstDirtyVertexIndex * sizeof(T);
        VkDeviceSize copySizeBytes = getCpuBufferSize() - gpuOffsetBytes;

        VULKAN_CHECKTHROW(vmaCopyMemoryToAllocation(allocator.getHandle(), cpuDataPtr, gpuBuffer.getAllocationHandle(),
                                                    gpuOffsetBytes, copySizeBytes));

        vk::BufferMemoryBarrier cpuWToGpuBufRBarrier{
            .srcAccessMask = vk::AccessFlagBits::eHostWrite,
            .dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead, // We created a uniform buffer
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = gpuBuffer.getBufferHandle(),
            .offset = gpuOffsetBytes,
            .size = copySizeBytes,
        };

        // It's important to insert a buffer memory barrier here to ensure writing to the buffer has finished.
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eVertexInput, {}, {},
                            cpuWToGpuBufRBarrier, {});
    }
}

} // namespace vma