#include <memory>

#include "GlobalContext.hpp"
#include "Core/Base/Marco.hpp"

namespace MiniEngine
{
    RuntimeGlobalContext gRuntimeGlobalContext;

    void RuntimeGlobalContext::StartSystems(const std::string &configFilePath) 
    {
        mConfigManager = std::make_shared<ConfigManager>();
        mConfigManager->Initialize(configFilePath);

        LOG_INFO(mConfigManager->GetGlobalRenderingResURL());

        // 初始化视窗系统
        mWindowsSystem = std::make_shared<WindowSystem>();
        WindowCreateInfo windowCreateInfo;
        mWindowsSystem->Initialize(windowCreateInfo);

        // 初始化渲染系统
        mRenderSystem = std::make_shared<RenderSystem>();
        RenderSystemInitInfo renderSystemInitInfo;
        renderSystemInitInfo.mWindowSystem = mWindowsSystem;
        mRenderSystem->Initialize(renderSystemInitInfo);

        // 初始化调试渲染管理器
        mDebugDrawManager = std::make_shared<DebugDrawManager>();
        mDebugDrawManager->Initialize();
    }

    void RuntimeGlobalContext::ShutdownSystems() {}
}