#pragma once

#include <memory>
#include <string>


namespace MiniEngine
{
    class LogSystem;
    class WindowSystem;
    class RenderSystem;
    class DebugDrawManager;
    class ConfigManager;

    class RuntimeGlobalContext
    {
    public:
        void StartSystems(const std::string& configFilePath);
        void ShutdownSystems();

        std::shared_ptr<LogSystem> mLoggerSystem;
        std::shared_ptr<WindowSystem> mWindowsSystem;
        std::shared_ptr<RenderSystem> mRenderSystem;
        std::shared_ptr<DebugDrawManager> mDebugDrawManager;
        std::shared_ptr<ConfigManager> mConfigManager;
    };

    extern RuntimeGlobalContext gRuntimeGlobalContext;
}