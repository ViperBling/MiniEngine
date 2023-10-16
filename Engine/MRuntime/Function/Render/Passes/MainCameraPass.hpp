#pragma once

#include "MRuntime/Function/Render/RenderPass.hpp"

#include "MRuntime/Function/Render/Passes/ColorGradientPass.hpp"
#include "MRuntime/Function/Render/Passes/CombineUIPass.hpp"
#include "MRuntime/Function/Render/Passes/ToneMappingPass.hpp"
#include "MRuntime/Function/Render/Passes/ToneMappingPass.hpp"

namespace MiniEngine
{
    class RenderSourceBase;

    struct MainCameraPassInitInfo : RenderPassInitInfo
    {
        bool mbEnableFXAA;
    };

    class MainCameraPass : public RenderPass
    {
    public:
        enum LayoutType : uint8_t
        {
            PerMesh = 0,
            MeshGlobal,
            MeshPerMaterial,
            Skybox,
            Axis,
            DeferredLighting,
            Count,
        };

        enum RenderPipelineType : uint8_t
        {
            MeshGBuffer = 0,
            DeferredLighting,
            MeshLighting,
            Skybox,
            Axis,
            Count,
        };

        void Initialize(const RenderPassInitInfo* initInfo) override final;
        void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;
        void Draw(
            ColorGradientPass& colorGradientPass,
            ToneMappingPass& toneMappingPass,
            UIPass& uiPass,
            CombineUIPass& combineUIPass,
            uint32_t currentSwapChaingIndex
        );
        void DrawForward(
            ColorGradientPass& colorGradientPass,
            ToneMappingPass& toneMappingPass,
            UIPass& uiPass,
            CombineUIPass& combineUIPass,
            uint32_t currentSwapChaingIndex
        );

        void CopyNormalAndDepthImage();
        void UpdateAfterFramebufferRecreate();
        RHICommandBuffer* GetRenderCommandBuffer();

    private:
        void setupAttachments();
        void setupRenderPass();
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();
        void setupFramebufferDescriptorSet();
        void setupSwapchainFramebuffers();

        void setupModelGlobalDescriptorSet();
        void setupSkyboxDescriptorSet();
        void setupAxisDescriptorSet();
        void setupGBufferLightingDescriptorSet();

        void drawMeshGBuffer();
        void drawDeferredLighting();
        void drawMeshLighting();
        void drawSkybox();
        void drawAxis();

    public:
        RHIImageView* mDirectionalLightShadowColorImageView;
        bool mbIsShowAxis{false};
        bool mbEnableFXAA{false};
        size_t mSelectedAxis{3};
        MeshPerframeStorageBufferObject mPerFrameStorageBufferObject;
        AxisStorageBufferObject mAxisStorageBufferObject;

    private:
        std::vector<RHIFrameBuffer*> mSwapChainFrameBuffers;
    };
}