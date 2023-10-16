#include <string>

#include "MEngine.hpp"
#include "MRuntime/Core/Base/Marco.hpp"
#include "MRuntime/Core/Meta/Reflection/ReflectionRegister.hpp"

#include "MRuntime/Function/Framework/World/WorldManager.hpp"
#include "MRuntime/Function/Input/InputSystem.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"

#include "MRuntime/Function/Render/WindowSystem.hpp"
#include "MRuntime/Function/Render/RenderSystem.hpp"
#include "MRuntime/Function/Render/DebugDraw/DebugDrawManager.hpp"


namespace MiniEngine
{
    bool                            gbIsEditorMode;
    std::unordered_set<std::string> gEditorTickComponentTypes;
    const float MiniEngine::MEngine::msFPSAlpha = 1.0f / 100;

    void MEngine::StartEngine(const std::string &configFilePath)
    {
        Reflection::TypeMetaRegister metaRegister;
        gRuntimeGlobalContext.StartSystems(configFilePath);
        LOG_INFO("Engine Start");
    }

    void MEngine::ShutdownEngine()
    {
        LOG_INFO("Engine Shutdown");
        gRuntimeGlobalContext.ShutdownSystems();
        Reflection::TypeMetaRegister::MetaUnregister();
    }

    void MEngine::Initialize() {}

    void MEngine::Clear() {}

    void MEngine::Run()
    {
        std::shared_ptr<WindowSystem> wndSystem = gRuntimeGlobalContext.mWindowSystem;
        ASSERT(wndSystem);
        while (!wndSystem->ShouldClose())
        {
            const float deltaTime = CalculateDeltaTime();
            TickOneFrame(deltaTime);
        }
    }

    bool MEngine::TickOneFrame(float DeltaTime)
    {
        LogicalTick(DeltaTime);
        CalculateFPS(DeltaTime);

        gRuntimeGlobalContext.mRenderSystem->SwapLogicRenderData();

        RendererTick(DeltaTime);

        gRuntimeGlobalContext.mWindowSystem->PollEvents();
        gRuntimeGlobalContext.mWindowSystem->SetTitle(std::string("MiniEngine - " + std::to_string(GetFPS()) + " FPS").c_str());
        const bool shouldWndClose = gRuntimeGlobalContext.mWindowSystem->ShouldClose();

        return !shouldWndClose;
    }

    void MEngine::LogicalTick(float DeltaTime)
    {
        gRuntimeGlobalContext.mWorldManager->Tick(DeltaTime);
        gRuntimeGlobalContext.mInputSystem->Tick();
    }

    bool MEngine::RendererTick(float DeltaTime)
    {
        gRuntimeGlobalContext.mRenderSystem->Tick(DeltaTime);
        return true;
    }
    
    void MEngine::CalculateFPS(float DeltaTime)
    {
        mFrameCount++;
        if (mFrameCount == 1)
        {
            mAvgDuration = DeltaTime;
        }
        else 
        {
            mAvgDuration = mAvgDuration * (1 - msFPSAlpha) + DeltaTime * msFPSAlpha;
        }
        mFPS = static_cast<int>(1.0f / mAvgDuration);
    }

    float MEngine::CalculateDeltaTime()
    {
        float deltaTime;
        {
            using namespace std::chrono;

            steady_clock::time_point tickTimePoint = steady_clock::now();
            auto timeSpan = duration_cast<duration<float>>(tickTimePoint - mLastTickTimePoint);
            deltaTime = timeSpan.count();
            mLastTickTimePoint = tickTimePoint;
        }

        return deltaTime;
    }
}






