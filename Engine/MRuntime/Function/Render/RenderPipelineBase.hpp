#pragma once

#include <memory>
#include <vector>

#include "MRuntime/Core/Math/Vector2.hpp"
#include "MRuntime/Function/Render/Interface/RHI.hpp"
#include "MRuntime/Function/Render/RenderResourceBase.hpp"
#include "MRuntime/Function/Render/RenderPassBase.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"

namespace MiniEngine
{
    class RHI;
    class RenderResourceBase;
    class WindowUI;

    struct RenderPipelineInitInfo
    {
        bool mbEnableFXAA = false;
        std::shared_ptr<RenderResourceBase> mRenderResource;
    };
    class RenderPipelineBase
    {
        friend class RenderSystem;

    public:
        virtual ~RenderPipelineBase() {}
        virtual void Clear() {}
        virtual void Initialize(RenderPipelineInitInfo initInfo) = 0;

        virtual void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource);
        void InitializeUIRenderBackend(WindowUI* window_ui);
        virtual uint32_t GetGuidOfPickedMesh(const Vector2& picked_uv) = 0;

        virtual void ForwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> renderResource);

    protected:
        std::shared_ptr<RHI> mRHI;

        std::shared_ptr<RenderPassBase> mDirectionalLightPass;
        std::shared_ptr<RenderPassBase> mMainCameraPass;
        std::shared_ptr<RenderPassBase> mColorGradientPass;
        std::shared_ptr<RenderPassBase> mTonMappingPass;
        std::shared_ptr<RenderPassBase> mPickPass;
        std::shared_ptr<RenderPassBase> mUIPass;
        std::shared_ptr<RenderPassBase> mCombineUIPass;
    };
}

