#include <cassert>
#include <memory>

#include "MEditor.h"
#include "Function/Global/GlobalContext.h"

using namespace MiniEngine;

void MEditor::Initialize(MEngine* engineRuntime) {

    assert(engineRuntime);
    this->mEngineRuntime = engineRuntime;

    mEditorUI = std::make_shared<MEditorUI>();
    WindowUIInitInfo uiInitInfo = {gRuntimeGlobalContext.mWindowsSystem};
    mEditorUI->Initialize(uiInitInfo);
}

void MEditor::Run() {

    assert(mEngineRuntime);
    assert(mEditorUI);
    float DeltaTime = 0;

    do
    {
        DeltaTime = mEngineRuntime->CalculateDeltaTime();
    } while (mEngineRuntime->Tick(DeltaTime));
}

void MEditor::Finalize() {}

