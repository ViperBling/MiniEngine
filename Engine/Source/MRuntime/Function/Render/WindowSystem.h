#pragma once

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

    private:
        int mWidth = 0;
        int mHeight = 0;
    };
}