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

        RenderTick(deltaTime);

        gRuntimeGlobalContext.mWindowsSystem->SetTitle(
            std::string("MiniEngine - " /*+std::to_string(getFPS())+" FPS"*/).c_str()
            );
        // 检查有没有触发什么事件（比如键盘输入、鼠标移动等）、更新窗口状态，并调用对应的回调函数
        gRuntimeGlobalContext.mWindowsSystem->PollEvents();

        return !gRuntimeGlobalContext.mWindowsSystem->ShouldClose();
    }

    bool MEngine::RenderTick(float DeltaTime) {

        gRuntimeGlobalContext.mRenderSystem->Tick(DeltaTime);
        return true;
    }
}






