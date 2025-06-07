#include "ShaderObject.hpp"
#include "Engine.hpp"
#include "helpers.hpp"

ShaderObject::ShaderObject(vk::Device device, std::string_view vertexShaderSpirvPath,
                           std::string_view fragShaderSpirvPath)
{
    shaders = helpers::vulkan::createShadersExtUnique(device, "triangle.vert.spv", "triangle.frag.spv");
}

void ShaderObject::bind(vk::CommandBuffer &commandBuffer)
{
    commandBuffer.bindShadersEXT({vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment},
                                 {shaders[0].get(), shaders[1].get()});
}
void ShaderObject::setState(vk::CommandBuffer &commandBuffer)
{
    commandBuffer.setRasterizerDiscardEnable(rasterizerDiscardEnable);
    commandBuffer.setPolygonModeEXT(polygonMode);
    commandBuffer.setCullMode(cullMode);
    commandBuffer.setFrontFace(frontFace);
    commandBuffer.setDepthBiasEnable(depthBiasEnable);

    commandBuffer.setDepthTestEnable(depthTestEnable);
    commandBuffer.setDepthWriteEnable(depthWriteEnable);
    commandBuffer.setDepthCompareOp(depthCompareOp);
    commandBuffer.setStencilTestEnable(stencilTestEnable);

    commandBuffer.setViewportWithCount(viewport);
    commandBuffer.setScissorWithCount(scissor);

    commandBuffer.setPrimitiveTopologyEXT(primitiveTopology);
    commandBuffer.setPrimitiveRestartEnableEXT(primitiveRestartEnable);

    commandBuffer.setVertexInputEXT(vertexBindingDescriptions.size(), vertexBindingDescriptions.data(),
                                    vertexAttributeDescriptions.size(), vertexAttributeDescriptions.data());

    for (const auto &[attachment, enable] : colorBlendEnables)
    {
        commandBuffer.setColorBlendEnableEXT(attachment, {enable});
    }
    commandBuffer.setColorWriteMaskEXT(0, colorBlendEnables.size(), &colorWriteMask);

    commandBuffer.setRasterizationSamplesEXT(rasterizationSamples);
    commandBuffer.setSampleMaskEXT(rasterizationSamples, &sampleMask);
    commandBuffer.setAlphaToCoverageEnableEXT(alphaToCoverageEnable);
}

std::vector<vk::VertexInputBindingDescription2EXT> &ShaderObject::vertexBindings()
{
    return vertexBindingDescriptions;
}

std::vector<vk::VertexInputAttributeDescription2EXT> &ShaderObject::attributeDescriptions()
{
    return vertexAttributeDescriptions;
}
// Setters
void ShaderObject::setRasterizerDiscardEnable(VkBool32 enable)
{
    rasterizerDiscardEnable = enable;
}
void ShaderObject::setPolygonMode(vk::PolygonMode mode)
{
    polygonMode = mode;
}
void ShaderObject::setCullMode(vk::CullModeFlags mode)
{
    cullMode = mode;
}
void ShaderObject::setFrontFace(vk::FrontFace front)
{
    frontFace = front;
}
void ShaderObject::setDepthBiasEnable(VkBool32 enable)
{
    depthBiasEnable = enable;
}

void ShaderObject::setDepthTestEnable(VkBool32 enable)
{
    depthTestEnable = enable;
}
void ShaderObject::setDepthWriteEnable(VkBool32 enable)
{
    depthWriteEnable = enable;
}
void ShaderObject::setDepthCompareOp(vk::CompareOp op)
{
    depthCompareOp = op;
}
void ShaderObject::setStencilTestEnable(VkBool32 enable)
{
    stencilTestEnable = enable;
}

void ShaderObject::setPrimitiveTopology(vk::PrimitiveTopology topology)
{
    primitiveTopology = topology;
}
void ShaderObject::setPrimitiveRestartEnable(VkBool32 enable)
{
    primitiveRestartEnable = enable;
}

void ShaderObject::setColorBlendEnable(uint32_t attachment, VkBool32 enable)
{
    colorBlendEnables[attachment] = enable;
}
void ShaderObject::setColorWriteMask(vk::ColorComponentFlags mask)
{
    colorWriteMask = mask;
}

void ShaderObject::setRasterizationSamples(vk::SampleCountFlagBits samples)
{
    rasterizationSamples = samples;
}
void ShaderObject::setSampleMask(uint32_t mask)
{
    sampleMask = mask;
}
void ShaderObject::setAlphaToCoverageEnable(VkBool32 enable)
{
    alphaToCoverageEnable = enable;
}
void ShaderObject::setViewport(vk::Viewport viewport_)
{
    viewport = viewport_;
}
void ShaderObject::setScissor(vk::Rect2D scissor_)
{
    scissor = scissor_;
}