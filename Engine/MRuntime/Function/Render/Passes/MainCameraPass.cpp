#include "MainCameraPass.hpp"

namespace MiniEngine
{
    void MainCameraPass::Initialize(const RenderPassInitInfo *initInfo)
    {
    }

    void MainCameraPass::PreparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
    }

    void MainCameraPass::Draw(
        ColorGradientPass &colorGradientPass, 
        ToneMappingPass &toneMappingPass, 
        UIPass &uiPass, 
        CombineUIPass &combineUIPass, 
        uint32_t currentSwapChaingIndex)
    {
    }

    void MainCameraPass::DrawForward(
        ColorGradientPass &colorGradientPass, 
        ToneMappingPass &toneMappingPass, 
        UIPass &uiPass, 
        CombineUIPass &combineUIPass, 
        uint32_t currentSwapChaingIndex)
    {
    }

    void MainCameraPass::CopyNormalAndDepthImage()
    {
    }

    void MainCameraPass::UpdateAfterFramebufferRecreate()
    {
    }

    RHICommandBuffer *MainCameraPass::GetRenderCommandBuffer()
    {
        return nullptr;
    }

    void MainCameraPass::setupAttachments()
    {
    }

    void MainCameraPass::setupRenderPass()
    {
    }

    void MainCameraPass::setupDescriptorSetLayout()
    {
    }

    void MainCameraPass::setupPipelines()
    {
    }

    void MainCameraPass::setupDescriptorSet()
    {
    }

    void MainCameraPass::setupFrameBufferDescriptorSet()
    {
    }

    void MainCameraPass::setupSwapChainFrameBuffers()
    {
    }

    void MainCameraPass::setupModelGlobalDescriptorSet()
    {
    }

    void MainCameraPass::setupSkyboxDescriptorSet()
    {
    }

    void MainCameraPass::setupAxisDescriptorSet()
    {
    }

    void MainCameraPass::setupGBufferLightingDescriptorSet()
    {
    }

    void MainCameraPass::drawMeshGBuffer()
    {
    }

    void MainCameraPass::drawDeferredLighting()
    {
    }

    void MainCameraPass::drawMeshLighting()
    {
    }

    void MainCameraPass::drawSkybox()
    {
    }

    void MainCameraPass::drawAxis()
    {
    }

} // namespace MiniEngine