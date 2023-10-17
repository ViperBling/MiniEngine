#pragma once

#include <memory>
#include <string>
#include <vector>

namespace MiniEngine
{
    class EditorFileNode;
    using EditorFileNodeArray = std::vector<std::shared_ptr<EditorFileNode>>;

    struct EditorFileNode
    {
        std::string         mFileName;
        std::string         mFileType;
        std::string         mFilePath;
        int                 mNodeDepth;
        EditorFileNodeArray mChildNodes;

        EditorFileNode() = default;
        EditorFileNode(const std::string& name, const std::string& type, const std::string& path, int depth) :
            mFileName(name), mFileType(type), mFilePath(path), mNodeDepth(depth)
        {}
    };

    class EditorFileService
    {
    public:
        EditorFileNode* GetEditorRootNode() { return mFileNodeArray.empty() ? nullptr : mFileNodeArray[0].get(); }
        void BuildEngineFileTree();
    
    private:
        EditorFileNode* getParentNodePtr(EditorFileNode* fileNode);
        bool            checkFileArray(EditorFileNode* fileNode);

    private:
        EditorFileNodeArray mFileNodeArray;
        EditorFileNode      mRootNode { "Asset", "Folder", "Asset", -1 };
    };
}