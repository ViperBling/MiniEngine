#pragma once

#include <memory>

#include "Function/Render/Interface/RHI.hpp"
#include "Function/Render/WindowSystem.hpp"
#include "Function/Render/RenderPipelineBase.hpp"

namespace MiniEngine
{
    struct RenderSystemInitInfo
    {
        std::shared_ptr<WindowSystem> mWindowSystem;
    };

    class RenderSystem
    {
    public:
        void Initialize(RenderSystemInitInfo initInfo);
        void Tick(float DeltaTime);

        std::shared_ptr<RHI> GetRHI() const { return mRHI; }

    private:
        RENDER_PIPELINE_TYPE mRenderPipelineType{RENDER_PIPELINE_TYPE::FORWARD_PIPELINE};
        std::shared_ptr<RHI> mRHI;
        std::shared_ptr<RenderResourceBase> mRenderResource;
        std::shared_ptr<RenderPipelineBase> mRenderPipeline;
    };
}