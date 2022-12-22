#include <memory>

#include "GlobalContext.h"

namespace MiniEngine
{
    RuntimeGlobalContext runtimeGlobalContext;

    void RuntimeGlobalContext::StartSystems(const std::string &configFilePath) {

        // 初始化
        mWindowsSystem = std::make_shared<WindowSystem>();
        WindowCreateInfo windowCreateInfo;
        mWindowsSystem->Initialize(windowCreateInfo);
    }

    void RuntimeGlobalContext::ShutdownSystems() {}
}