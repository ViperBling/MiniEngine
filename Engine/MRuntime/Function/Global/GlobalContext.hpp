#pragma once

#include <memory>
#include <string>

namespace MiniEngine
{
    class LogSystem;
    class InputSystem;
    class FileSystem;
    class WindowSystem;
    class RenderSystem;
    class DebugDrawManager;
    class AssetManager;
    class WorldManager;
    class ConfigManager;

    class RuntimeGlobalContext
    {
    public:
        void StartSystems(const std::string& configFilePath);
        void ShutdownSystems();

        std::shared_ptr<LogSystem>          mLoggerSystem;
        std::shared_ptr<InputSystem>        mInputSystem;
        std::shared_ptr<FileSystem>         mFileSystem;
        std::shared_ptr<WindowSystem>       mWindowSystem;
        std::shared_ptr<RenderSystem>       mRenderSystem;

        std::shared_ptr<DebugDrawManager>   mDebugDrawManager;
        std::shared_ptr<AssetManager>       mAssetManager;
        std::shared_ptr<WorldManager>       mWorldManager;
        std::shared_ptr<ConfigManager>      mConfigManager;
    };

    extern RuntimeGlobalContext gRuntimeGlobalContext;
}