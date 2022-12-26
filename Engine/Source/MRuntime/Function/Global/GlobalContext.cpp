#include <memory>

#include "GlobalContext.h"

namespace MiniEngine
{
    RuntimeGlobalContext runtimeGlobalContext;

    void RuntimeGlobalContext::StartSystems(const std::string &configFilePath) {

        // 初始化视窗系统
        mWindowsSystem = std::make_shared<WindowSystem>();
        WindowCreateInfo windowCreateInfo;
        mWindowsSystem->Initialize(windowCreateInfo);

        // 初始化渲染系统
        mRenderSystem = std::make_shared<RenderSystem>();
        RenderSystemInitInfo renderSystemInitInfo;
        renderSystemInitInfo.windowSystem = mWindowsSystem;
        mRenderSystem->Initialize(renderSystemInitInfo);
    }

    void RuntimeGlobalContext::ShutdownSystems() {}
}