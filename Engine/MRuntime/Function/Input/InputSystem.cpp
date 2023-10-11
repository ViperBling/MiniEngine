#include "InputSystem.hpp"

#include "MRuntime/Core/Base/Marco.hpp"

#include "MRuntime/MEngine.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Render/RenderCamera.hpp"
#include "MRuntime/Function/Render/RenderSystem.hpp"
#include "MRuntime/Function/Render/WindowSystem.hpp"

#include <GLFW/glfw3.h>
#include <iostream>

namespace MiniEngine
{
    unsigned int kComplementControlCommand = 0xFFFFFFFF;

    void InputSystem::OnKey(int key, int scancode, int action, int mods)
    {
        onKeyInEditMode(key, scancode, action, mods);
    }

    void InputSystem::onKeyInEditMode(int key, int scancode, int action, int mods)
    {
        mGameCommand &= (kComplementControlCommand ^ (unsigned int)GameCommand::Jump);

        if (action == GLFW_PRESS)
        {
            switch (key)
            {
                case GLFW_KEY_ESCAPE:
                    // close();
                    break;
                case GLFW_KEY_R:
                    break;
                case GLFW_KEY_A:
                    mGameCommand |= (unsigned int)GameCommand::Left;
                    break;
                case GLFW_KEY_S:
                    mGameCommand |= (unsigned int)GameCommand::Backward;
                    break;
                case GLFW_KEY_W:
                    mGameCommand |= (unsigned int)GameCommand::Forward;
                    break;
                case GLFW_KEY_D:
                    mGameCommand |= (unsigned int)GameCommand::Right;
                    break;
                case GLFW_KEY_SPACE:
                    mGameCommand |= (unsigned int)GameCommand::Jump;
                    break;
                case GLFW_KEY_LEFT_CONTROL:
                    mGameCommand |= (unsigned int)GameCommand::Squat;
                    break;
                case GLFW_KEY_LEFT_ALT: {
                    std::shared_ptr<WindowSystem> wndSystem = gRuntimeGlobalContext.mWindowsSystem;
                    // wndSystem->SetFocusMode(!wndSystem->GetFocusMode());
                }
                break;
                case GLFW_KEY_LEFT_SHIFT:
                    mGameCommand |= (unsigned int)GameCommand::Sprint;
                    break;
                case GLFW_KEY_F:
                    mGameCommand ^= (unsigned int)GameCommand::FreeCarema;
                    break;
                default:
                    break;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            switch (key)
            {
                case GLFW_KEY_ESCAPE:
                    // close();
                    break;
                case GLFW_KEY_R:
                    break;
                case GLFW_KEY_W:
                    mGameCommand &= (kComplementControlCommand ^ (unsigned int)GameCommand::Forward);
                    break;
                case GLFW_KEY_S:
                    mGameCommand &= (kComplementControlCommand ^ (unsigned int)GameCommand::Backward);
                    break;
                case GLFW_KEY_A:
                    mGameCommand &= (kComplementControlCommand ^ (unsigned int)GameCommand::Left);
                    break;
                case GLFW_KEY_D:
                    mGameCommand &= (kComplementControlCommand ^ (unsigned int)GameCommand::Right);
                    break;
                case GLFW_KEY_LEFT_CONTROL:
                    mGameCommand &= (kComplementControlCommand ^ (unsigned int)GameCommand::Squat);
                    break;
                case GLFW_KEY_LEFT_SHIFT:
                    mGameCommand &= (kComplementControlCommand ^ (unsigned int)GameCommand::Sprint);
                    break;
                default:
                    break;
            }
        }
    }

    void InputSystem::OnCursorPos(double current_cursor_x, double current_cursor_y)
    {
        if (gRuntimeGlobalContext.mWindowsSystem->GetFocusMode())
        {
            mCursorDeltaX = mLastCursorX - current_cursor_x;
            mCursorDeltaY = mLastCursorY - current_cursor_y;
        }
        mLastCursorX = current_cursor_x;
        mLastCursorY = current_cursor_y;
    }

    void InputSystem::Clear()
    {
        mCursorDeltaX = 0;
        mCursorDeltaY = 0;
    }

    void InputSystem::Initialize()
    {
        std::shared_ptr<WindowSystem> window_system = gRuntimeGlobalContext.mWindowsSystem;
        ASSERT(window_system);

        window_system->RegisterOnKeyFunc(std::bind(&InputSystem::OnKey,
                                                   this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3,
                                                   std::placeholders::_4));
        window_system->RegisterOnCursorPosFunc(
            std::bind(&InputSystem::OnCursorPos, this, std::placeholders::_1, std::placeholders::_2));
    }

    void InputSystem::Tick()
    {
        Clear();

        std::shared_ptr<WindowSystem> window_system = gRuntimeGlobalContext.mWindowsSystem;
        if (window_system->GetFocusMode())
        {
            mGameCommand &= (kComplementControlCommand ^ (unsigned int)GameCommand::Invalid);
        }
        else
        {
            mGameCommand |= (unsigned int)GameCommand::Invalid;
        }
    }
} // namespace MiniEngine
