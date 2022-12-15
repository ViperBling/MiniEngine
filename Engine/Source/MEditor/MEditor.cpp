#include <cassert>

#include "MEditor.h"

using namespace MiniEngine;

MEditor::MEditor() {};

void MEditor::Initialize(MEngine* engineRuntime) {

    assert(engineRuntime);
    this->mEngineRuntime = engineRuntime;
}

void MEditor::Run() {}

void MEditor::Finalize() {}

