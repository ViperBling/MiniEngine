#pragma once

#include <string>
#include <chrono>
#include <cassert>
#include <memory>
#include <unordered_set>

namespace MiniEngine
{
    extern bool                            gbIsEditorMode;
    extern std::unordered_set<std::string> gEditorTickComponentTypes;

    class MEngine
    {
    public:
        void Initialize(const std::string& configFilePath);
        void Finalize();

        float CalculateDeltaTime();
        bool Tick(float deltaTime);

    protected:
        bool RenderTick(float DeltaTime);

    protected:
        std::chrono::steady_clock::time_point mTimePointLast = std::chrono::steady_clock::now();    // 上个时间点
    };
} // namespace MiniEngine