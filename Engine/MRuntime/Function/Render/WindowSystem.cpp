#include "WindowSystem.hpp"
#include "MRuntime/Core/Base/Marco.hpp"
#include "MRuntime/Core/Log/LogSystem.hpp"

#include <chrono>
#include <thread>

namespace MiniEngine
{
    WindowSystem::~WindowSystem()
    {
        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }

    void WindowSystem::Initialize(WindowCreateInfo& create_info)
    {
        if (!glfwInit())
        {
            LOG_FATAL(__FUNCTION__, "failed to initialize GLFW");
            return;
        }

        mWidth  = create_info.Width;
        mHeight = create_info.Height;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        mWindow = glfwCreateWindow(create_info.Width, create_info.Height, create_info.Title, nullptr, nullptr);
        if (!mWindow)
        {
            LOG_FATAL(__FUNCTION__, "failed to create window");
            glfwTerminate();
            return;
        }

        // Setup input callbacks
        glfwSetWindowUserPointer(mWindow, this);
        glfwSetKeyCallback(mWindow, KeyCallback);
        glfwSetCharCallback(mWindow, CharCallback);
        glfwSetCharModsCallback(mWindow, CharModsCallback);
        glfwSetMouseButtonCallback(mWindow, MouseButtonCallback);
        glfwSetCursorPosCallback(mWindow, CursorPosCallback);
        glfwSetCursorEnterCallback(mWindow, CursorEnterCallback);
        glfwSetScrollCallback(mWindow, ScrollCallback);
        glfwSetDropCallback(mWindow, DropCallback);
        glfwSetWindowSizeCallback(mWindow, WindowSizeCallback);
        glfwSetWindowCloseCallback(mWindow, WindowCloseCallback);

        glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }

    void WindowSystem::SetFocusMode(bool mode)
    {
        mbIsFocusMode = mode;
        glfwSetInputMode(mWindow, GLFW_CURSOR, mbIsFocusMode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}