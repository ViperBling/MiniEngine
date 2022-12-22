#pragma once

#include <string>
#include <chrono>
#include <cassert>
#include <memory>

namespace MiniEngine
{
    class MEngine
    {
    public:
        void Initialize(const std::string& configFilePath);
        void Finalize();

        float CalculateDeltaTime();
        bool Tick(float deltaTime);

    protected:
        std::chrono::steady_clock::time_point mTimePointLast = std::chrono::steady_clock::now();    // 上个时间点
    };
} // namespace MiniEngine