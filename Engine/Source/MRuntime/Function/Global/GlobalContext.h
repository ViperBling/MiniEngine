#pragma once

#include <memory>
#include <string>

#include "Function/Render/WindowSystem.h"

namespace MiniEngine
{
    class RuntimeGlobalContext
    {
    public:
        void StartSystems(const std::string& configFilePath);
        void ShutdownSystems();

        std::shared_ptr<WindowSystem> mWindowsSystem;
    };

    extern RuntimeGlobalContext runtimeGlobalContext;
}