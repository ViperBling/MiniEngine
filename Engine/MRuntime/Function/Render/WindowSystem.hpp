#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <functional>
#include <vector>

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
        ~WindowSystem();
        void Initialize(WindowCreateInfo& createInfo);
        void SetTitle(const char* title) { glfwSetWindowTitle(mWindow, title); }
        static void PollEvents() { glfwPollEvents(); }
        bool ShouldClose() { return glfwWindowShouldClose(mWindow); }

        GLFWwindow* GetWindow() const { return mWindow; };
        std::array<int, 2> GetWindowSize() const { return std::array<int, 2>({mWidth, mHeight}); }

        typedef std::function<void()>                   OnResetFunc;
        typedef std::function<void(int, int, int, int)> OnKeyFunc;
        typedef std::function<void(unsigned int)>       OnCharFunc;
        typedef std::function<void(int, unsigned int)>  OnCharModsFunc;
        typedef std::function<void(int, int, int)>      OnMouseButtonFunc;
        typedef std::function<void(double, double)>     OnCursorPosFunc;
        typedef std::function<void(int)>                OnCursorEnterFunc;
        typedef std::function<void(double, double)>     OnScrollFunc;
        typedef std::function<void(int, const char**)>  OnDropFunc;
        typedef std::function<void(int, int)>           OnWindowSizeFunc;
        typedef std::function<void()>                   OnWindowCloseFunc;

        void RegisterOnResetFunc(OnResetFunc func) { mOnResetFunc.push_back(func); }
        void RegisterOnKeyFunc(OnKeyFunc func) { mOnKeyFunc.push_back(func); }
        void RegisterOnCharFunc(OnCharFunc func) { mOnCharFunc.push_back(func); }
        void RegisterOnCharModsFunc(OnCharModsFunc func) { mOnCharModsFunc.push_back(func); }
        void RegisterOnMouseButtonFunc(OnMouseButtonFunc func) { mOnMouseButtonFunc.push_back(func); }
        void RegisterOnCursorPosFunc(OnCursorPosFunc func) { mOnCursorPosFunc.push_back(func); }
        void RegisterOnCursorEnterFunc(OnCursorEnterFunc func) { mOnCursorEnterFunc.push_back(func); }
        void RegisterOnScrollFunc(OnScrollFunc func) { mOnScrollFunc.push_back(func); }
        void RegisterOnDropFunc(OnDropFunc func) { mOnDropFunc.push_back(func); }
        void RegisterOnWindowSizeFunc(OnWindowSizeFunc func) { mOnWindowSizeFunc.push_back(func); }
        void RegisterOnWindowCloseFunc(OnWindowCloseFunc func) { mOnWindowCloseFunc.push_back(func); }

        bool IsMouseButtonDown(int button) const
        {
            if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST)
            {
                return false;
            }
            return glfwGetMouseButton(mWindow, button) == GLFW_PRESS;
        }
        bool GetFocusMode() const { return mbIsFocusMode; }
        void SetFocusMode(bool mode);

    protected:
        // window event callbacks
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
            if (app)
            {
                app->OnKey(key, scancode, action, mods);
            }
        }
        static void CharCallback(GLFWwindow* window, unsigned int codepoint)
        {
            WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
            if (app)
            {
                app->OnChar(codepoint);
            }
        }
        static void CharModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
        {
            WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
            if (app)
            {
                app->OnCharMods(codepoint, mods);
            }
        }
        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
        {
            WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
            if (app)
            {
                app->OnMouseButton(button, action, mods);
            }
        }
        static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
        {
            WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
            if (app)
            {
                app->OnCursorPos(xpos, ypos);
            }
        }
        static void CursorEnterCallback(GLFWwindow* window, int entered)
        {
            WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
            if (app)
            {
                app->OnCursorEnter(entered);
            }
        }
        static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
        {
            WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
            if (app)
            {
                app->OnScroll(xoffset, yoffset);
            }
        }
        static void DropCallback(GLFWwindow* window, int count, const char** paths)
        {
            WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
            if (app)
            {
                app->OnDrop(count, paths);
            }
        }
        static void WindowSizeCallback(GLFWwindow* window, int width, int height)
        {
            WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
            if (app)
            {
                app->mWidth  = width;
                app->mHeight = height;
            }
        }
        static void WindowCloseCallback(GLFWwindow* window) { glfwSetWindowShouldClose(window, true); }

        void OnReset()
        {
            for (auto& func : mOnResetFunc)
                func();
        }
        void OnKey(int key, int scancode, int action, int mods)
        {
            for (auto& func : mOnKeyFunc)
                func(key, scancode, action, mods);
        }
        void OnChar(unsigned int codepoint)
        {
            for (auto& func : mOnCharFunc)
                func(codepoint);
        }
        void OnCharMods(int codepoint, unsigned int mods)
        {
            for (auto& func : mOnCharModsFunc)
                func(codepoint, mods);
        }
        void OnMouseButton(int button, int action, int mods)
        {
            for (auto& func : mOnMouseButtonFunc)
                func(button, action, mods);
        }
        void OnCursorPos(double xpos, double ypos)
        {
            for (auto& func : mOnCursorPosFunc)
                func(xpos, ypos);
        }
        void OnCursorEnter(int entered)
        {
            for (auto& func : mOnCursorEnterFunc)
                func(entered);
        }
        void OnScroll(double xoffset, double yoffset)
        {
            for (auto& func : mOnScrollFunc)
                func(xoffset, yoffset);
        }
        void OnDrop(int count, const char** paths)
        {
            for (auto& func : mOnDropFunc)
                func(count, paths);
        }
        void OnWindowSize(int width, int height)
        {
            for (auto& func : mOnWindowSizeFunc)
                func(width, height);
        }

    private:
        GLFWwindow* mWindow = nullptr;
        int mWidth = 0;
        int mHeight = 0;

        bool mbIsFocusMode {false};

        std::vector<OnResetFunc>       mOnResetFunc;
        std::vector<OnKeyFunc>         mOnKeyFunc;
        std::vector<OnCharFunc>        mOnCharFunc;
        std::vector<OnCharModsFunc>    mOnCharModsFunc;
        std::vector<OnMouseButtonFunc> mOnMouseButtonFunc;
        std::vector<OnCursorPosFunc>   mOnCursorPosFunc;
        std::vector<OnCursorEnterFunc> mOnCursorEnterFunc;
        std::vector<OnScrollFunc>      mOnScrollFunc;
        std::vector<OnDropFunc>        mOnDropFunc;
        std::vector<OnWindowSizeFunc>  mOnWindowSizeFunc;
        std::vector<OnWindowCloseFunc> mOnWindowCloseFunc;
    };
}