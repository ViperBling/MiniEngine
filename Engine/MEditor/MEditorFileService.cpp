#include "MEditorFileService.hpp"

#include "MRuntime/Platform/FileSystem/FileSystem.hpp"
#include "MRuntime/Platform/Path/Path.hpp"

#include "MRuntime/Resource/AssetManager/AssetManager.hpp"
#include "MRuntime/Resource/ConfigManager/ConfigManager.hpp"

#include "MRuntime/Function/Global/GlobalContext.hpp"

namespace MiniEngine
{
    // helper function: split the input string with separator, and filter the substring
    std::vector<std::string> splitString(std::string input_string, const std::string& separator, const std::string& filter_string = "")
    {
        std::vector<std::string> output_string;
        int                      pos = input_string.find(separator);
        std::string              add_string;

        while (pos != std::string::npos)
        {
            add_string = input_string.substr(0, pos);
            if (!add_string.empty())
            {
                if (!filter_string.empty() && add_string == filter_string)
                {
                    // filter substring
                }
                else
                {
                    output_string.push_back(add_string);
                }
            }
            input_string.erase(0, pos + 1);
            pos = input_string.find(separator);
        }
        add_string = input_string;
        if (!add_string.empty())
        {
            output_string.push_back(add_string);
        }
        return output_string;
    }

    void EditorFileService::BuildEngineFileTree()
    {
        std::string                              asset_folder = gRuntimeGlobalContext.mConfigManager->GetAssetFolder().generic_string();
        const std::vector<std::filesystem::path> file_paths = gRuntimeGlobalContext.mFileSystem->GetFiles(asset_folder);
        std::vector<std::vector<std::string>>    all_file_segments;
        for (const auto& path : file_paths)
        {
            const std::filesystem::path& relative_path = Path::GetRelativePath(asset_folder, path);
            all_file_segments.emplace_back(Path::GetPathSegments(relative_path));
        }

        std::vector<std::shared_ptr<EditorFileNode>> node_array;

        mFileNodeArray.clear();
        auto root_node = std::make_shared<EditorFileNode>();
        *root_node     = mRootNode;
        mFileNodeArray.push_back(root_node);

        int all_file_segments_count = all_file_segments.size();
        for (int file_index = 0; file_index < all_file_segments_count; file_index++)
        {
            int depth = 0;
            node_array.clear();
            node_array.push_back(root_node);
            int file_segment_count = all_file_segments[file_index].size();
            for (int file_segment_index = 0; file_segment_index < file_segment_count; file_segment_index++)
            {
                auto file_node         = std::make_shared<EditorFileNode>();
                file_node->mFileName = all_file_segments[file_index][file_segment_index];
                if (depth < file_segment_count - 1)
                {
                    file_node->mFileType = "Folder";
                }
                else
                {
                    const auto& extensions = Path::GetFileExtensions(file_paths[file_index]);
                    file_node->mFileType = std::get<0>(extensions);
                    if (file_node->mFileType.size() == 0)
                        continue;

                    if (file_node->mFileType.compare(".json") == 0)
                    {
                        file_node->mFileType = std::get<1>(extensions);
                        if (file_node->mFileType.compare(".component") == 0)
                        {
                            file_node->mFileType = std::get<2>(extensions) + std::get<1>(extensions);
                        }
                    }
                    file_node->mFileType = file_node->mFileType.substr(1);
                    file_node->mFilePath = file_paths[file_index].generic_string();
                }
                file_node->mNodeDepth = depth;
                node_array.push_back(file_node);

                bool node_exists = checkFileArray(file_node.get());
                if (node_exists == false)
                {
                    mFileNodeArray.push_back(file_node);
                }
                EditorFileNode* parent_node_ptr = getParentNodePtr(node_array[depth].get());
                if (parent_node_ptr != nullptr && node_exists == false)
                {
                    parent_node_ptr->mChildNodes.push_back(file_node);
                }
                depth++;
            }
        }
    }

    EditorFileNode *EditorFileService::getParentNodePtr(EditorFileNode *fileNode)
    {
        int editor_node_count = mFileNodeArray.size();
        for (int file_node_index = 0; file_node_index < editor_node_count; file_node_index++)
        {
            if (mFileNodeArray[file_node_index]->mFileName == fileNode->mFileName &&
                mFileNodeArray[file_node_index]->mNodeDepth == fileNode->mNodeDepth)
            {
                return mFileNodeArray[file_node_index].get();
            }
        }
        return nullptr;
    }

    bool EditorFileService::checkFileArray(EditorFileNode *fileNode)
    {
        int editor_node_count = mFileNodeArray.size();
        for (int file_node_index = 0; file_node_index < editor_node_count; file_node_index++)
        {
            if (mFileNodeArray[file_node_index]->mFileName == fileNode->mFileName &&
                mFileNodeArray[file_node_index]->mNodeDepth == fileNode->mNodeDepth)
            {
                return true;
            }
        }
        return false;
    }
}