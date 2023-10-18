#pragma once

#include "MRuntime/Function/Render/RenderPass.hpp"

#include "MRuntime/Function/Render/Passes/ColorGradientPass.hpp"
#include "MRuntime/Function/Render/Passes/CombineUIPass.hpp"
#include "MRuntime/Function/Render/Passes/ToneMappingPass.hpp"
#include "MRuntime/Function/Render/Passes/UIPass.hpp"

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
            LayoutType_PerMesh = 0,
            LayoutType_MeshGlobal,
            LayoutType_MeshPerMaterial,
            LayoutType_Skybox,
            LayoutType_Axis,
            LayoutType_DeferredLighting,
            LayoutType_Count,
        };

        enum RenderPipelineType : uint8_t
        {
            RenderPipelineType_MeshGBuffer = 0,
            RenderPipelineType_DeferredLighting,
            RenderPipelineType_MeshLighting,
            RenderPipelineType_Skybox,
            RenderPipelineType_Axis,
            RenderPipelineType_Count,
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
        void setupFrameBufferDescriptorSet();
        void setupSwapChainFrameBuffers();

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