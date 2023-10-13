#pragma once

#include "MRuntime/Function/Render/RenderPass.hpp"

#include "MRuntime/Function/Render/Passes/ColorGradientPass.hpp"
#include "MRuntime/Function/Render/Passes/CombineUIPass.hpp"
#include "MRuntime/Function/Render/Passes/ToneMappingPass.hpp"
#include "MRuntime/Function/Render/Passes/ToneMappingPass.hpp"

namespace MiniEngine
{
    class RenderSourceBase;

    class MainCameraPass : public RenderPass
    {
    public:
        void Initialize(const RenderPassInitInfo* initInfo) override final;
        void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;
        // void Draw(ColorGradientPass&)

        void SetPerMeshLayout(RHIDescriptorSetLayout* layout) { mPerMeshLayout = layout; }

    private:
        void setupAttachments();
        void setupRenderPass();
        void setupFrameBuffer();
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();
        void drawModel();

    private:
        RHIDescriptorSetLayout* mPerMeshLayout;
        MeshDirectionalLightShadowPerFrameStorageBufferObject mMeshDirectLightShadowPerframeBufferObject;
    };
}