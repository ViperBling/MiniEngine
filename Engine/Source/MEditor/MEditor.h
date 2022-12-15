#pragma once

#include "MEngine.h"

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
        MEngine* mEngineRuntime = nullptr;
    };
}