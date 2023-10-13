#pragma once

#include "MRuntime/Function/Render/RenderPass.hpp"

namespace MiniEngine
{
    class RenderSourceBase;

    class DirectionalLightShadowPass : public RenderPass
    {
    public:
        void Initialize(const RenderPassInitInfo* initInfo) override final;
        void PostInitialize() override final;
        void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;
        void Draw() override final;

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