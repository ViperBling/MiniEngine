#pragma once

#include "Function/Render/RenderPipelineBase.h"

namespace MiniEngine
{
    class RenderPipeline : public RenderPipelineBase
    {
    public:
        void ForwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> renderResource) override;
        void PassUpdateAfterRecreateSwapChain();
    };
}