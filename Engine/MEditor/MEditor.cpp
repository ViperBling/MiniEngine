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
    void registerEdtorTickComponent(std::string component_type_name)
    {
        gEditorTickComponentTypes.insert(component_type_name);
    }

    MEditor::MEditor()
    {
        registerEdtorTickComponent("TransformComponent");
        registerEdtorTickComponent("MeshComponent");
    }

    MEditor::~MEditor()
    {
    }

    void MEditor::Initialize(MEngine *engineRuntime)
    {
        assert(engineRuntime);

        gbIsEditorMode = true;
        mEngineRuntime = engineRuntime;

        EditorGlobalContextInitInfo init_info = {gRuntimeGlobalContext.mWindowSystem.get(),
                                                 gRuntimeGlobalContext.mRenderSystem.get(),
                                                 engineRuntime};
        gEditorGlobalContext.Initialize(init_info);
        gEditorGlobalContext.mSceneManager->SetEditorCamera(
            gRuntimeGlobalContext.mRenderSystem->GetRenderCamera());
        gEditorGlobalContext.mSceneManager->UploadAxisResource();

        mEditorUI                   = std::make_shared<MEditorUI>();
        WindowUIInitInfo ui_init_info = {gRuntimeGlobalContext.mWindowSystem,
                                         gRuntimeGlobalContext.mRenderSystem};
        mEditorUI->Initialize(ui_init_info);
    }

    void MEditor::Clear()
    {
        gEditorGlobalContext.Clear();
    }

    void MEditor::Run() 
    {
        assert(mEngineRuntime);
        assert(mEditorUI);
        float delta_time;
        while (true)
        {
            delta_time = mEngineRuntime->CalculateDeltaTime();
            gEditorGlobalContext.mSceneManager->Tick(delta_time);
            gEditorGlobalContext.mInputManager->Tick(delta_time);
            if (!mEngineRuntime->TickOneFrame(delta_time))
                return;
        }
    }
}