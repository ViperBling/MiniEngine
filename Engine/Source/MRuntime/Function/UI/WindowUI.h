#pragma once

#include <memory>

#include "Function/Render/WindowSystem.h"

namespace MiniEngine
{
    struct WindowUIInitInfo
    {
        std::shared_ptr<WindowSystem> mWindowSystem;
    };

    class WindowUI
    {
    public:
        virtual void Initialize(WindowUIInitInfo initInfo) = 0;
    };
}