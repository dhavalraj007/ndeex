#pragma once
#include "Vulkan.hpp"
#include <concepts>
#include <string_view>
#include <type_traits>
#include <unordered_map>

class ShaderObject
{
  public:
    ShaderObject() = default;
    ShaderObject(vk::Device device, std::string_view vertexShaderSpirvPath, std::string_view fragShaderSpirvPath);
    void bind(vk::CommandBuffer &commandBuffer);

    // Setters
    void setRasterizerDiscardEnable(vk::Bool32 enable);
    void setPolygonMode(vk::PolygonMode mode);
    void setCullMode(vk::CullModeFlags mode);
    void setFrontFace(vk::FrontFace front);
    void setDepthBiasEnable(vk::Bool32 enable);

    void setDepthTestEnable(vk::Bool32 enable);
    void setDepthWriteEnable(vk::Bool32 enable);
    void setDepthCompareOp(vk::CompareOp op);
    void setStencilTestEnable(vk::Bool32 enable);

    void setPrimitiveTopology(vk::PrimitiveTopology topology);
    void setPrimitiveRestartEnable(vk::Bool32 enable);
    void setColorBlendEnable(uint32_t attachment, vk::Bool32 enable);
    void setColorWriteMask(vk::ColorComponentFlags mask);

    void setRasterizationSamples(vk::SampleCountFlagBits samples);
    void setSampleMask(uint32_t mask);
    void setAlphaToCoverageEnable(vk::Bool32 enable);

    void setViewport(vk::Viewport viewport_);
    void setScissor(vk::Rect2D scissor_);

    std::vector<vk::VertexInputBindingDescription2EXT> &vertexBindings();
    std::vector<vk::VertexInputAttributeDescription2EXT> &attributeDescriptions();

    void setState(vk::CommandBuffer &commandBuffer);

  private:
    // Rasterizer
    vk::Bool32 rasterizerDiscardEnable = false;
    vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
    vk::CullModeFlags cullMode = vk::CullModeFlagBits::eNone;
    vk::FrontFace frontFace = vk::FrontFace::eClockwise;
    vk::Bool32 depthBiasEnable = false;

    // Depth/Stencil
    vk::Bool32 depthTestEnable = false;
    vk::Bool32 depthWriteEnable = false;
    vk::CompareOp depthCompareOp = vk::CompareOp::eAlways;
    vk::Bool32 stencilTestEnable = false;

    // Primitive
    vk::PrimitiveTopology primitiveTopology = vk::PrimitiveTopology::eTriangleList;
    vk::Bool32 primitiveRestartEnable = false;

    // Blend
    std::unordered_map<uint32_t, vk::Bool32> colorBlendEnables;
    vk::ColorComponentFlags colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                             vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    // MSAA
    vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1;
    uint32_t sampleMask = 0xFFFFFFFF;
    vk::Bool32 alphaToCoverageEnable = false;

    // Viewport
    vk::Viewport viewport;
    vk::Rect2D scissor;

    // vertex bindings
    std::vector<vk::VertexInputBindingDescription2EXT> vertexBindingDescriptions;
    std::vector<vk::VertexInputAttributeDescription2EXT> vertexAttributeDescriptions;

    std::vector<vk::UniqueShaderEXT> shaders;
    vk::Device device;
};