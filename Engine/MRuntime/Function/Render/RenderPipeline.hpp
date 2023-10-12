#pragma once

#include "Function/Render/RenderPipelineBase.hpp"

namespace MiniEngine
{
    class RenderPipeline : public RenderPipelineBase
    {
    public:
        virtual void Initialize(RenderPipelineInitInfo init_info) override final;
        void ForwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> renderResource) override;
        void PassUpdateAfterRecreateSwapChain();

        virtual uint32_t GetGuidOfPickedMesh(const Vector2& picked_uv) override final;
        void SetAxisVisibleState(bool state);
        void SetSelectedAxis(size_t selected_axis);
    };
}