#pragma once

#include "MRuntime/Function/Render/RenderPass.hpp"

namespace MiniEngine
{
    struct ColorGradientPassInitInfo : RenderPassInitInfo
    {
        RHIRenderPass* mRenderPass;
        RHIImageView* mInputAttachment;
    };

    class ColorGradientPass : public RenderPass
    {
    public:
        void Initialize(const RenderPassInitInfo* initInfo) override final;
        void Draw() override final;

        void UpdateAfterFramebufferRecreate(RHIImageView* inputAttachment);

    private:
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();
    };
} // namespace MiniEngine
