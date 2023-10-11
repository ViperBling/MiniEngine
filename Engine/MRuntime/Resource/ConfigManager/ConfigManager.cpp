#include "Resource/ConfigManager/ConfigManager.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include "ConfigManager.hpp"

namespace MiniEngine
{
    void MiniEngine::ConfigManager::Initialize(const std::filesystem::path &configFilePath)
    {
        std::ifstream configFile(configFilePath);

        std::string configLine;
        while (std::getline(configFile, configLine))
        {
            size_t seperatePos = configLine.find_first_of("=");
            if (seperatePos > 0 && seperatePos < (configLine.length() - 1))
            {
                std::string name = configLine.substr(0, seperatePos);
                std::string value = configLine.substr(seperatePos + 1, configLine.length() - seperatePos - 1);

                if (name == "BinaryRootFolder")
                    mRootFolder = configFilePath.parent_path() / value;
                else if (name == "AssetFolder")
                    mAssetFolder = mRootFolder / value;
                else if (name == "ShaderFolder")
                    mShaderFolder = mRootFolder / value;
                else if (name == "SchemaFolder")
                    mSchemaFolder = mRootFolder / value;
                else if (name == "DefaultWorld")
                    mDefaultWorldURL = value;
                else if (name == "BigIconFile")
                    mEditorBigIconPath = mRootFolder / value;
                else if (name == "SmallIconFile")
                    mEditorSmallIconPath = mRootFolder / value;
                else if (name == "FontFile")
                    mEditorFontPath = mRootFolder / value;
                else if (name == "GlobalRenderingRes")
                    mGlobalRenderingResURL = value;
            }
        }
    }
} // namespace MiniEngine

