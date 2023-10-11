#pragma once

#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

namespace MiniEngine
{
    class Path
    {
    public:
        static const std::filesystem::path GetRelativePath(
            const std::filesystem::path& directory,
            const std::filesystem::path& file_path) ;

        static const std::vector<std::string> GetPathSegments(const std::filesystem::path& file_path) ;

        static const std::tuple<std::string, std::string, std::string> GetFileExtensions(const std::filesystem::path& file_path);

        static const std::string GetFilePureName(const std::string);
    };
} // namespace MiniEngine