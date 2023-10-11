#include "RenderPassBase.hpp"

#include "MRuntime/Core/Base/Marco.hpp"

namespace MiniEngine
{
    void RenderPassBase::PostInitialize() {}

    void RenderPassBase::SetCommonInfo(RenderPassCommonInfo common_info)
    {
        mRHI             = common_info.mRHI;
        mRenderResource = common_info.mRenderResource;
    }

    void RenderPassBase::PreparePassData(std::shared_ptr<RenderResourceBase> render_resource) {}
    void RenderPassBase::InitializeUIRenderBackend(WindowUI* window_ui) {}
} // namespace MiniEngine
