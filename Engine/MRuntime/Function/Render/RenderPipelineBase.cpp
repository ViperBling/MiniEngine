#include "RenderPipelineBase.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Render/DebugDraw/DebugDrawManager.hpp"
#include "MRuntime/Core/Base/Marco.hpp"

namespace MiniEngine
{
    void RenderPipelineBase::PreparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        mDirectionalLightPass->PreparePassData(render_resource);
        mMainCameraPass->PreparePassData(render_resource);
        mColorGradientPass->PreparePassData(render_resource);
        mToneMappingPass->PreparePassData(render_resource);
        mPickPass->PreparePassData(render_resource);
        mUIPass->PreparePassData(render_resource);
        mCombineUIPass->PreparePassData(render_resource);
        
        gRuntimeGlobalContext.mDebugDrawManager->PreparePassData(render_resource);
    }

    void RenderPipelineBase::ForwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> renderResource) 
    {

    }

    void RenderPipelineBase::InitializeUIRenderBackend(WindowUI* window_ui)
    {
        mUIPass->InitializeUIRenderBackend(window_ui);
    }
}