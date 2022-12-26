#include "WindowSystem.h"
#include "Core/Base/Marco.h"

namespace MiniEngine
{
    void WindowSystem::Initialize(WindowCreateInfo &createInfo) {

        if (!glfwInit()) {

            LOG_FATAL("Failed to Initialize GLFW!");
            return;
        }

        mWidth = createInfo.Width;
        mHeight = createInfo.Height;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // 不产生OpenGL上下文

        if (!(mWindow = glfwCreateWindow(mWidth, mHeight, createInfo.Title, nullptr, nullptr))) {

            LOG_FATAL("Failed To Create Window!");
            glfwTerminate();
            return;
        }
    }
}