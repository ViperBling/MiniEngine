#pragma once

#include <memory>
#include <string>

#include "Function/Render/WindowSystem.hpp"
#include "Function/Render/RenderSystem.hpp"
#include "Function/Render/DebugDraw/DebugDrawManager.hpp"
#include "Resource/ConfigManager/ConfigManager.hpp"

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
        std::shared_ptr<ConfigManager> mConfigManager;
    };

    extern RuntimeGlobalContext gRuntimeGlobalContext;
}