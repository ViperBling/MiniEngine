#include <memory>

#include "GlobalContext.hpp"

#include "MRuntime/Core/Base/Marco.hpp"
#include "MRuntime/Core/Log/LogSystem.hpp"
#include "MRuntime/Function/Input/InputSystem.hpp"
#include "MRuntime/Platform/FileSystem/FileSystem.hpp"
#include "MRuntime/Function/Render/WindowSystem.hpp"
#include "MRuntime/Function/Render/RenderSystem.hpp"
#include "MRuntime/Function/Render/RenderDebugConfig.hpp"
#include "MRuntime/Function/Render/DebugDraw/DebugDrawManager.hpp"
#include "MRuntime/Resource/AssetManager/AssetManager.hpp"
#include "MRuntime/Function/Framework/World/WorldManager.hpp"
#include "MRuntime/Resource/ConfigManager/ConfigManager.hpp"

namespace MiniEngine
{
    RuntimeGlobalContext gRuntimeGlobalContext;

    void RuntimeGlobalContext::StartSystems(const std::string &configFilePath) 
    {
        mConfigManager = std::make_shared<ConfigManager>();
        mConfigManager->Initialize(configFilePath);

        mFileSystem = std::make_shared<FileSystem>();

        mLoggerSystem = std::make_shared<LogSystem>();

        mAssetManager = std::make_shared<AssetManager>();

        mWorldManager = std::make_shared<WorldManager>();
        mWorldManager->Initialize();

        mWindowSystem = std::make_shared<WindowSystem>();
        WindowCreateInfo window_create_info;
        mWindowSystem->Initialize(window_create_info);

        mRenderSystem = std::make_shared<RenderSystem>();
        RenderSystemInitInfo render_init_info;
        render_init_info.mWindowSystem = mWindowSystem;
        mRenderSystem->Initialize(render_init_info);

        mDebugDrawManager = std::make_shared<DebugDrawManager>();
        mDebugDrawManager->Initialize();

        mInputSystem = std::make_shared<InputSystem>();
        mInputSystem->Initialize();

        mRenderDebugConfig = std::make_shared<RenderDebugConfig>();
    }

    void RuntimeGlobalContext::ShutdownSystems() 
    {
        mRenderDebugConfig.reset();
        
        mInputSystem->Clear();
        mInputSystem.reset();

        mRenderSystem->Clear();
        mRenderSystem.reset();

        mWindowSystem.reset();

        mLoggerSystem.reset();

        mFileSystem.reset();

        mAssetManager.reset();

        mWorldManager->Clear();
        mWorldManager.reset();

        mConfigManager.reset();

        mDebugDrawManager.reset();
    }
}