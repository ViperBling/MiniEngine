#include "AssetManager.hpp"

#include "MRuntime/Resource/ConfigManager/ConfigManager.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"

#include <filesystem>

namespace MiniEngine
{
    std::filesystem::path AssetManager::GetFullPath(const std::string& relative_path) const
    {
        return std::filesystem::absolute(gRuntimeGlobalContext.mConfigManager->GetRootFolder() / relative_path);
    }
} // namespace MiniEngine
