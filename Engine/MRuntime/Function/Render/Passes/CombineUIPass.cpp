#include "CombineUIPass.hpp"

#include "MRuntime/Function/Render/Interface/Vulkan/VulkanRHI.hpp"
#include "MRuntime/Function/Render/Interface/Vulkan/VulkanUtil.hpp"

#include <CombineUI_frag.h>
#include <PostProcess_vert.h>

#include <stdexcept>

namespace MiniEngine
{
    void CombineUIPass::Initialize(const RenderPassInitInfo *initInfo)
    {
        
    }

    void CombineUIPass::Draw()
    {
    }

    void CombineUIPass::UpdateAfterFramebufferRecreate(RHIImageView *sceneInputAttachment, RHIImageView *uiInputAttachment)
    {
    }

    void CombineUIPass::setupDescriptorSetLayout()
    {
    }

    void CombineUIPass::setupPipelines()
    {
    }

    void CombineUIPass::setupDescriptorSet()
    {
    }

} // namespace MiniEngine