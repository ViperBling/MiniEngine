#pragma once

#include <filesystem>
#include <vector>

namespace MiniEngine
{
    class FileSystem 
    {
    public:
        static std::vector<std::filesystem::path> GetFiles(const std::filesystem::path& directory);
    };
} // namespace MiniEngine
