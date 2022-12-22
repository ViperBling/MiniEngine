#pragma once

#include <memory>

#include "MEngine.h"
#include "MEditorUI.h"

namespace MiniEngine
{
    class MEditor
    {
    public:
        MEditor();
        void Initialize(MEngine* engineRuntime);
        void Run();
        void Finalize();

    protected:
        std::shared_ptr<MEditorUI> mEditorUI;
        MEngine* mEngineRuntime = nullptr;
    };
}