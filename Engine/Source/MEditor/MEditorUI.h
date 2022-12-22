#pragma once

#include "Function/UI/WindowUI.h"

namespace MiniEngine
{
    class MEditorUI : public WindowUI
    {
    public:
        MEditorUI();
        virtual void Initialize(WindowUIInitInfo initInfo) override final;
    };
}