#include "RenderPass.hpp"
#include "MRuntime/Core/Base/Marco.hpp"
#include "MRuntime/Function/Render/RenderResource.hpp"
#include "MRuntime/Function/Render/Interface/Vulkan/VulkanRHI.hpp"

MiniEngine::VisiableNodes MiniEngine::RenderPass::mVisibleNodes;

namespace MiniEngine
{
    void RenderPass::Initialize(const RenderPassInitInfo* init_info)
    {
        mGlobalRenderResource =
            &(std::static_pointer_cast<RenderResource>(mRenderResource)->mGlobalRenderResource);
    }
    void RenderPass::Draw() {}

    void RenderPass::PostInitialize() {}

    RHIRenderPass* RenderPass::GetRenderPass() const { return mFrameBuffer.renderPass; }

    std::vector<RHIImageView*> RenderPass::GetFramebufferImageViews() const
    {
        std::vector<RHIImageView*> image_views;
        for (auto& attach : mFrameBuffer.attachments)
        {
            image_views.push_back(attach.view);
        }
        return image_views;
    }

    std::vector<RHIDescriptorSetLayout*> RenderPass::GetDescriptorSetLayouts() const
    {
        std::vector<RHIDescriptorSetLayout*> layouts;
        for (auto& desc : mDescInfos)
        {
            layouts.push_back(desc.layout);
        }
        return layouts;
    }
}