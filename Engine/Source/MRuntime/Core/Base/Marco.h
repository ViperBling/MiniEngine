#pragma once

#include <iostream>
#include <stdexcept>

#define LOG_INFO(...) std::cout << __VA_ARGS__

#define LOG_ERROR(...) std::cerr << __VA_ARGS__

#define LOG_FATAL(...) std::cerr << __VA_ARGS__