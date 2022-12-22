#include <cassert>
#include <memory>

#include "MEditor.h"
#include "Function/Global/GlobalContext.h"

using namespace MiniEngine;

MEditor::MEditor() {};

void MEditor::Initialize(MEngine* engineRuntime) {

    assert(engineRuntime);
    this->mEngineRuntime = engineRuntime;

    mEditorUI = std::make_shared<MEditorUI>();
    WindowUIInitInfo uiInitInfo = {runtimeGlobalContext.mWindowsSystem};
    mEditorUI->Initialize(uiInitInfo);
}

void MEditor::Run() {

    assert(mEngineRuntime);
    assert(mEditorUI);

    while (true) {

        float deltaTime = mEngineRuntime->CalculateDeltaTime();
        if (!mEngineRuntime->Tick(deltaTime)) return;
    }
}

void MEditor::Finalize() {}

