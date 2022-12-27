#pragma once

#include <memory>

#include "Function/Render/WindowSystem.h"
#include "Function/Render/RenderSystem.h"

namespace MiniEngine
{
    struct WindowUIInitInfo
    {
        std::shared_ptr<WindowSystem> mWindowSystem;
        std::shared_ptr<RenderSystem> mRenderSystem;
    };

    class WindowUI
    {
    public:
        virtual void Initialize(WindowUIInitInfo initInfo) = 0;
    };
}