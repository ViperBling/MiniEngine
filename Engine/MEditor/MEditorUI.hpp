#pragma once

#include "Function/UI/WindowUI.hpp"

namespace MiniEngine
{
    class MEditorUI : public WindowUI
    {
    public:
        MEditorUI();
        virtual void Initialize(WindowUIInitInfo initInfo) override final;
    };
}