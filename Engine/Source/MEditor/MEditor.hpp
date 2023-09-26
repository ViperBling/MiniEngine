#pragma once

#include <memory>

#include "MEngine.hpp"
#include "MEditorUI.hpp"

namespace MiniEngine
{
    class MEditor
    {
    public:

        void Initialize(MEngine* engineRuntime);
        void Run();
        void Finalize();

    protected:
        std::shared_ptr<MEditorUI> mEditorUI;
        MEngine* mEngineRuntime = nullptr;
    };
}