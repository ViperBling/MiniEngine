#pragma once

#include "MRuntime/Function/Render/RenderPass.hpp"

namespace MiniEngine
{
    struct CombineUIPassInitInfo : RenderPassInitInfo
    {
        RHIRenderPass* mRenderPass;
        RHIImageView*  mSceneInputAttachment;
        RHIImageView*  mUIInputAttachment;
    };

    class CombineUIPass : public RenderPass
    {
    public:
        void Initialize(const RenderPassInitInfo* initInfo) override final;
        void Draw() override final;

        void UpdateAfterFramebufferRecreate(RHIImageView* sceneInputAttachment, RHIImageView* uiInputAttachment);

    private:
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();
    };
}