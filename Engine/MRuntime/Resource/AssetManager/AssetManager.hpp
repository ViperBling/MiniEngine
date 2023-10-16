#pragma once

#include "MRuntime/Core/Base/Marco.hpp"
#include "MRuntime/Core/Meta/Serializer/Serializer.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

#include "Generated/Serializer/all_serializer.h"

namespace MiniEngine
{
    class AssetManager
    {
    public:
        template<typename AssetType>
        bool LoadAsset(const std::string& asset_url, AssetType& out_asset) const
        {
            // read json file to string
            std::filesystem::path asset_path = GetFullPath(asset_url);
            std::ifstream asset_json_file(asset_path);
            if (!asset_json_file)
            {
                LOG_ERROR("open file: {} failed!", asset_path.generic_string());
                return false;
            }

            std::stringstream buffer;
            buffer << asset_json_file.rdbuf();
            std::string asset_json_text(buffer.str());

            // parse to json object and read to runtime res object
            std::string error;
            auto&&      asset_json = Json::parse(asset_json_text, error);
            if (!error.empty())
            {
                LOG_ERROR("parse json file {} failed!", asset_url);
                return false;
            }

            Serializer::read(asset_json, out_asset);
            return true;
        }

        template<typename AssetType>
        bool SaveAsset(const AssetType& out_asset, const std::string& asset_url) const
        {
            std::ofstream asset_json_file(GetFullPath(asset_url));
            if (!asset_json_file)
            {
                LOG_ERROR("open file {} failed!", asset_url);
                return false;
            }

            // write to json object and dump to string
            auto&&        asset_json      = Serializer::write(out_asset);
            std::string&& asset_json_text = asset_json.dump();

            // write to file
            asset_json_file << asset_json_text;
            asset_json_file.flush();

            return true;
        }

        std::filesystem::path GetFullPath(const std::string& relative_path) const;
    };
} // namespace MiniEngine
