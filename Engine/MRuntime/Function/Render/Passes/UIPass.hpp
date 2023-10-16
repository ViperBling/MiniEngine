#pragma once

#include "MRuntime/Function/Render/RenderPass.hpp"

namespace MiniEngine
{
    class WindowUI;
    struct UIPassInitInfo : RenderPassInitInfo
    {
        RHIRenderPass* mRenderPass;
    };

    class UIPass : public RenderPass
    {
    public:
        void Initialize(const RenderPassInitInfo* initInfo) override final;
        void InitializeUIRenderBackend(WindowUI* wndUI) override final;
        void Draw() override final;

    private:
        void uploadFonts();

    private:
        WindowUI* mWndUI;
    };
}