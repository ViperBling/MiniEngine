#pragma once

#include <memory>

#include "Function/Render/Interface/RHI.h"
#include "Function/Render/WindowSystem.h"

namespace MiniEngine
{
    struct RenderSystemInitInfo
    {
        std::shared_ptr<WindowSystem> windowSystem;
    };

    class RenderSystem
    {
    public:
        void Initialize(RenderSystemInitInfo initInfo);

    private:
        std::shared_ptr<RHI> rhi;
    };
}