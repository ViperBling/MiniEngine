#pragma once

#include <iostream>
#include <stdexcept>
#include <thread>

#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Core/Log/LogSystem.hpp"

// #define LOG_INFO(...) std::cout << "file: " << __FILE__ << "\tline: " << __LINE__ << "\tinfo: " << __VA_ARGS__ << "\n"

// #define LOG_ERROR(...) \
//     std::cerr << "file: " << __FILE__ << "\tline: " << __LINE__ << "\terror: " << __VA_ARGS__ << "\n"; \
//     assert(false);

// #define LOG_FATAL(...) std::cerr << "file: " << __FILE__ << "\tline: " << __LINE__ << "\tfatal: " << __VA_ARGS__ << "\n"

#define LOG_HELPER(LOG_LEVEL, ...) \
    gRuntimeGlobalContext.mLoggerSystem->Log(LOG_LEVEL, "[" + std::string(__FUNCTION__) + "] " + __VA_ARGS__);

#define LOG_DEBUG(...) LOG_HELPER(LogSystem::LogLevel::Debug, __VA_ARGS__);

#define LOG_INFO(...) LOG_HELPER(LogSystem::LogLevel::Info, __VA_ARGS__);

#define LOG_WARN(...) LOG_HELPER(LogSystem::LogLevel::Warning, __VA_ARGS__);

#define LOG_ERROR(...) LOG_HELPER(LogSystem::LogLevel::Error, __VA_ARGS__);

#define LOG_FATAL(...) LOG_HELPER(LogSystem::LogLevel::Fatal, __VA_ARGS__);

#define PolitSleep(_ms) std::this_thread::sleep_for(std::chrono::milliseconds(_ms));

#define PolitNameOf(name) #name

#ifdef NDEBUG
#define ASSERT(statement)
#else
#define ASSERT(statement) assert(statement)
#endif