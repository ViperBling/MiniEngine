#pragma once

#include <string>

namespace MiniEngine
{
    class MEngine
    {
    public:
        MEngine() = delete;
        explicit MEngine(const std::string & configFilePath);
        void Initialize();
        void Run();
        void Finalize();
    };
} // namespace MiniEngine