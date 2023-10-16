#pragma once

#include <spdlog/spdlog.h>

#include <cstdint>
#include <stdexcept>

namespace MiniEngine
{
    class LogSystem final
    {
    public:
        enum class LogLevel : uint8_t
        {
            Debug,
            Info,
            Warning,
            Error,
            Fatal
        };
    
    public:
        LogSystem();
        ~LogSystem();

        template<typename... TARGS>
        void Log(LogLevel level, TARGS&&... args)
        {
            switch (level)
            {
                case LogLevel::debug:
                    mLogger->debug(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::info:
                    mLogger->info(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::warn:
                    mLogger->warn(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::error:
                    mLogger->error(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::fatal:
                    mLogger->critical(std::forward<TARGS>(args)...);
                    fatalCallback(std::forward<TARGS>(args)...);
                    break;
                default:
                    break;
            }
        }

        template<typename... TARGS>
        void FatalCallback(TARGS&&... args)
        {
            const std::string format_str = fmt::format(std::forward<TARGS>(args)...);
            throw std::runtime_error(format_str);
        }

        private:
            std::shared_ptr<spdlog::logger> mLogger;
    };
}