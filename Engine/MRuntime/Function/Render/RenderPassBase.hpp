#pragma once

#include "MRuntime/Function/Render/Interface/RHI.hpp"

namespace MiniEngine
{
    class RHI;
    class RenderResourceBase;
    class WindowUI;

    struct RenderPassInitInfo
    {};

    struct RenderPassCommonInfo
    {
        std::shared_ptr<RHI>                mRHI;
        std::shared_ptr<RenderResourceBase> mRenderResource;
    };

    class RenderPassBase
    {
    public:
        virtual void Initialize(const RenderPassInitInfo* init_info) = 0;
        virtual void PostInitialize();
        virtual void SetCommonInfo(RenderPassCommonInfo common_info);
        virtual void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource);
        virtual void InitializeUIRenderBackend(WindowUI* window_ui);

    protected:
        std::shared_ptr<RHI>                mRHI;
        std::shared_ptr<RenderResourceBase> mRenderResource;
    };
} // namespace MiniEngine
