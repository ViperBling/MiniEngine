#include <string>

#include "MEngine.h"
#include "Function/Render/WindowSystem.h"
#include "Function/Global/GlobalContext.h"

namespace MiniEngine
{
    void MEngine::Initialize(const std::string& configFilePath) {

        gRuntimeGlobalContext.StartSystems(configFilePath);
    }

    void MEngine::Finalize() {}


    float MEngine::CalculateDeltaTime() {

        using namespace std::chrono;

        steady_clock::time_point tickTimePoint = steady_clock::now();

        auto timeSpan = duration_cast<duration<float>>(tickTimePoint - mTimePointLast);
        mTimePointLast = tickTimePoint;

        return timeSpan.count();
    }

    bool MEngine::Tick(float deltaTime) {

        gRuntimeGlobalContext.mWindowsSystem->SetTitle(
            std::string("MiniEngine - " /*+std::to_string(getFPS())+" FPS"*/).c_str()
            );

        gRuntimeGlobalContext.mWindowsSystem->PollEvents();

        return !gRuntimeGlobalContext.mWindowsSystem->ShouldClose();
    }
}






