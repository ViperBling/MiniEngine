#pragma once

#include <filesystem>

namespace MiniEngine
{
    class ConfigManager
    {
    public:
        void Initialize(const std::filesystem::path& configFilePath);

        const std::filesystem::path& GetRootFolder() const { return mRootFolder; }
        const std::filesystem::path& GetAssetFolder() const { return mAssetFolder; }
        const std::filesystem::path& GetShaderFolder() const { return mShaderFolder; }
        const std::filesystem::path& GetSchemaFolder() const { return mSchemaFolder; }
        const std::filesystem::path& GetEditorBigIconPath() const { return mEditorBigIconPath; }
        const std::filesystem::path& GetEditorSmallIconPath() const { return mEditorSmallIconPath; }
        const std::filesystem::path& GetEditorFontPath() const { return mEditorFontPath; }

        const std::string& GetDefaultWorldURL() const { return mDefaultWorldURL; }
        const std::string& GetGlobalRenderingResURL() const { return mGlobalRenderingResURL; }

    private:
        std::filesystem::path mRootFolder;
        std::filesystem::path mAssetFolder;
        std::filesystem::path mShaderFolder;
        std::filesystem::path mSchemaFolder;
        std::filesystem::path mEditorBigIconPath;
        std::filesystem::path mEditorSmallIconPath;
        std::filesystem::path mEditorFontPath;

        std::string mDefaultWorldURL;
        std::string mGlobalRenderingResURL;
    };
}