#pragma once

#include <GLFW/glfw3.h>

namespace MiniEngine
{
    struct WindowCreateInfo
    {
        int         Width         = 1280;
        int         Height        = 720;
        const char* Title         = "MiniEngine";
        bool        IsFullscreen  = false;
    };

    class WindowSystem
    {
    public:
        WindowSystem() = default;
        void Initialize(WindowCreateInfo& createInfo);
        void SetTitle(const char* title) { glfwSetWindowTitle(mWindow, title); }
        static void PollEvents() { glfwPollEvents(); }
        bool ShouldClose() { return glfwWindowShouldClose(mWindow); }

    private:
        GLFWwindow* mWindow = nullptr;
        int mWidth = 0;
        int mHeight = 0;
    };
}