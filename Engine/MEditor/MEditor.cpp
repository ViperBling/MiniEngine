#include "MEditor.hpp"

#include "MRuntime/MEngine.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Render/RenderCamera.hpp"
#include "MRuntime/Function/Render/RenderSystem.hpp"

#include "MEditor/MEditorGlobalContext.hpp"
#include "MEditor/MEditorInputManager.hpp"
#include "MEditor/MEditorSceneManager.hpp"
#include "MEditor/MEditorUI.hpp"

using namespace MiniEngine;

namespace MiniEngine
{
    void MEditor::Initialize(MEngine* engineRuntime) 
    {
        assert(engineRuntime);
        this->mEngineRuntime = engineRuntime;

        mEditorUI = std::make_shared<MEditorUI>();
        WindowUIInitInfo uiInitInfo = {gRuntimeGlobalContext.mWindowSystem};
        mEditorUI->Initialize(uiInitInfo);
    }

    void MEditor::Run() 
    {
        assert(mEngineRuntime);
        assert(mEditorUI);
        float DeltaTime = 0;

        do
        {
            DeltaTime = mEngineRuntime->CalculateDeltaTime();
        } while (mEngineRuntime->TickOneFrame(DeltaTime));
    }
}