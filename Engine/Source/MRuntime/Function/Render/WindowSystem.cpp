#include "WindowSystem.h"

namespace MiniEngine
{
    void WindowSystem::Initialize(WindowCreateInfo &createInfo) {

        if (!glfwInit()) return;

        mWidth = createInfo.Width;
        mHeight = createInfo.Height;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        if (!(mWindow = glfwCreateWindow(mWidth, mHeight, createInfo.Title, nullptr, nullptr))) {

            glfwTerminate();
            return;
        }
    }
}