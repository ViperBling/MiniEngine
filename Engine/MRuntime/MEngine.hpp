#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace MiniEngine
{
    extern bool                            gbIsEditorMode;
    extern std::unordered_set<std::string> gEditorTickComponentTypes;

    class MEngine
    {
        friend class MEditor;
        static const float msFPSAlpha;
    public:
        void StartEngine(const std::string& configFilePath);
        void ShutdownEngine();

        void Initialize();
        void Clear();

        bool IsQuit() const { return mbIsQuit; }
        void Run();
        bool TickOneFrame(float DeltaTime);

        int GetFPS() const { return mFPS; }

    protected:
        void LogicalTick(float DeltaTime);
        bool RendererTick(float DeltaTime);

        void CalculateFPS(float DeltaTime);
        float CalculateDeltaTime();

    protected:
        bool mbIsQuit {false};
        std::chrono::steady_clock::time_point mLastTickTimePoint = std::chrono::steady_clock::now();    // 上个时间点
        float mAvgDuration {0.0f};
        uint32_t mFrameCount {0};
        uint32_t mFPS {0};
    };
} // namespace MiniEngine