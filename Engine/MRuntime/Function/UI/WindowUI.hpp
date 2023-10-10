#pragma once

#include <memory>

namespace MiniEngine
{
    class WindowSystem;
    class RenderSystem;
    
    struct WindowUIInitInfo
    {
        std::shared_ptr<WindowSystem> mWindowSystem;
        std::shared_ptr<RenderSystem> mRenderSystem;
    };

    class WindowUI
    {
    public:
        virtual void Initialize(WindowUIInitInfo initInfo) = 0;
        virtual void PreRender() = 0;
    };
}