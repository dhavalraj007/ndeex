#include "Imgui.hpp"
#include "helpers.hpp"
#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include "imgui_impl_vulkan.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>

void DearImgui::init(InitInfo const &initInfo)
{
    context = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    static auto const VkloadFunc = +[](char const *name, void *user_data) {
        return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(*static_cast<vk::Instance *>(user_data), name);
    };
    vk::Instance instance = initInfo.instance;
    CHECKTHROW(ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_4, VkloadFunc, static_cast<void *>(&instance)));
    CHECKTHROW(ImGui_ImplSDL3_InitForVulkan(initInfo.window));

    VkFormat format = static_cast<VkFormat>(initInfo.format);
    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format,
    };
    ImGui_ImplVulkan_InitInfo info{
        .ApiVersion = VK_API_VERSION_1_4,
        .Instance = instance,
        .PhysicalDevice = initInfo.physicalDevice,
        .Device = initInfo.device,
        .QueueFamily = initInfo.graphicsQueueFamilyIndex,
        .Queue = static_cast<VkQueue>(initInfo.graphicsQueue),
        .MinImageCount = 2,
        .ImageCount = initInfo.imageCount,
        .MSAASamples = static_cast<VkSampleCountFlagBits>(vk::SampleCountFlagBits::e1),
        .DescriptorPoolSize = 2,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
    };
    CHECKTHROW(ImGui_ImplVulkan_Init(&info));
    CHECKTHROW(ImGui_ImplVulkan_CreateFontsTexture());
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 1.f;

    // sRGB (ImGui style color) → [x*x] → linear → [ImGui renders as-is] → [Vulkan: sqrt(x)] → sRGB on screen
    for (auto &colour : ImGui::GetStyle().Colors)
    {
        colour = ImVec4{colour.x * colour.x, colour.y * colour.y, colour.z * colour.z, colour.w * colour.w};
    }
}

DearImgui::~DearImgui()
{
    if (context)
    {
        ImGui_ImplVulkan_DestroyFontsTexture();
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
}

void DearImgui::newFrame()
{
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void DearImgui::render(vk::CommandBuffer cmd)
{
    ImGui::Render();
    if (auto *data = ImGui::GetDrawData(); data)
        ImGui_ImplVulkan_RenderDrawData(data, cmd);
}