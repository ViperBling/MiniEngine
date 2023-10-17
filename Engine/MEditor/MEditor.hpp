#pragma once

#include "MRuntime/Core/Math/Vector2.hpp"

#include <memory>

namespace MiniEngine
{
    class MEditorUI;
    class MEngine;

    class MEditor
    {
        friend class MEditorUI;

    public:
        MEditor();
        ~MEditor();

        void Initialize(MEngine* engineRuntime);
        void Clear();
        void Run();

    protected:
        std::shared_ptr<MEditorUI> mEditorUI;
        MEngine* mEngineRuntime = nullptr;
    };
}