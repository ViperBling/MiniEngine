#pragma once

#include <memory>
#include <string>

#include "Function/Render/WindowSystem.h"
#include "Function/Render/RenderSystem.h"
#include "Function/Render/DebugDraw/DebugDrawManager.h"

namespace MiniEngine
{
    class RuntimeGlobalContext
    {
    public:
        void StartSystems(const std::string& configFilePath);
        void ShutdownSystems();

        std::shared_ptr<WindowSystem> mWindowsSystem;
        std::shared_ptr<RenderSystem> mRenderSystem;
        std::shared_ptr<DebugDrawManager> mDebugDrawManager;
    };

    extern RuntimeGlobalContext gRuntimeGlobalContext;
}