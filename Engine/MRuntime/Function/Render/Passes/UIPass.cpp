#include "UIPass.hpp"

#include "MRuntime/Function/Render/Interface/Vulkan/VulkanRHI.hpp"
#include "MRuntime/Function/Render/Interface/Vulkan/VulkanUtil.hpp"
#include "MRuntime/Resource/ConfigManager/ConfigManager.hpp"

#include "MRuntime/Function/UI/WindowUI.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace MiniEngine
{
    void UIPass::Initialize(const RenderPassInitInfo *initInfo)
    {
        RenderPass::Initialize(initInfo);

        mFrameBuffer.renderPass = static_cast<const UIPassInitInfo*>(initInfo)->mRenderPass;
    }

    void UIPass::InitializeUIRenderBackend(WindowUI *wndUI)
    {
        mWndUI = wndUI;

        ImGui_ImplGlfw_InitForVulkan(std::static_pointer_cast<VulkanRHI>(mRHI)->mWindow, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = std::static_pointer_cast<VulkanRHI>(mRHI)->mInstance;
        init_info.PhysicalDevice            = std::static_pointer_cast<VulkanRHI>(mRHI)->mPhysicalDevice;
        init_info.Device                    = std::static_pointer_cast<VulkanRHI>(mRHI)->mDevice;
        init_info.QueueFamily               = mRHI->GetQueueFamilyIndices().graphicsFamily.value();
        init_info.Queue                     = ((VulkanQueue*)mRHI->GetGraphicsQueue())->GetResource();
        init_info.DescriptorPool            = std::static_pointer_cast<VulkanRHI>(mRHI)->mVkDescPool;
        init_info.Subpass                   = MAIN_CAMERA_SUBPASS_UI;
        
        // may be different from the real swapchain image count
        // see ImGui_ImplVulkanH_GetMinImageCountFromPresentMode
        init_info.MinImageCount = 3;
        init_info.ImageCount    = 3;
        ImGui_ImplVulkan_Init(&init_info, ((VulkanRenderPass*)mFrameBuffer.renderPass)->GetResource());

        uploadFonts();
    }

    void UIPass::Draw()
    {
        if (mWndUI)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            mWndUI->PreRender();

            float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            mRHI->PushEvent(mRHI->GetCurrentCommandBuffer(), "ImGUI", color);

            ImGui::Render();

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), std::static_pointer_cast<VulkanRHI>(mRHI)->mVkCurrentCommandBuffer);

            mRHI->PopEvent(mRHI->GetCurrentCommandBuffer());
        }
    }

    void UIPass::uploadFonts()
    {
        RHICommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType                       = RHI_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level                       = RHI_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool                 = mRHI->GetCommandPool();
        allocInfo.commandBufferCount          = 1;

        RHICommandBuffer* commandBuffer = new VulkanCommandBuffer();
        if (RHI_SUCCESS != mRHI->AllocateCommandBuffers(&allocInfo, commandBuffer))
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        RHICommandBufferBeginInfo beginInfo = {};
        beginInfo.sType                    = RHI_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags                    = RHI_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (RHI_SUCCESS != mRHI->BeginCommandBufferPFN(commandBuffer, &beginInfo))
        {
            throw std::runtime_error("Could not create one-time command buffer!");
        }

        ImGui_ImplVulkan_CreateFontsTexture(((VulkanCommandBuffer*)commandBuffer)->GetResource());

        if (RHI_SUCCESS != mRHI->EndCommandBufferPFN(commandBuffer))
        {
            throw std::runtime_error("failed to record command buffer!");
        }

        RHISubmitInfo submitInfo {};
        submitInfo.sType              = RHI_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;

        mRHI->QueueSubmit(mRHI->GetGraphicsQueue(), 1, &submitInfo, RHI_NULL_HANDLE);
        mRHI->QueueWaitIdle(mRHI->GetGraphicsQueue());

        mRHI->FreeCommandBuffers(mRHI->GetCommandPool(), 1, commandBuffer);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

} // namespace MiniEngine