#pragma once

#include "MEditor/Axis.hpp"
#include "MEditor/MEditorFileService.hpp"

#include "MRuntime/Core/Math/Vector2.hpp"
#include "MRuntime/Function/UI/WindowUI.hpp"
#include "MRuntime/Function/Framework/Object/Object.hpp"

#include <chrono>
#include <map>
#include <vector>

namespace MiniEngine
{
    class MEditor;
    class WindowSystem;
    class RenderSystem;

    class MEditorUI : public WindowUI
    {
    public:
        MEditorUI();
        virtual void Initialize(WindowUIInitInfo initInfo) override final;
        virtual void PreRender() override final;

    private:
        void        onFileContentItemClicked(EditorFileNode* node);
        void        buildEditorFileAssetsUITree(EditorFileNode* node);
        void        drawAxisToggleButton(const char* stringID, bool checkState, int axisMode);
        void        createClassUI(Reflection::ReflectionInstance& instance);
        void        createLeafNodeUI(Reflection::ReflectionInstance& instance);
        std::string getLeafUINodeParentLabel();

        void showEditorUI();
        void showEditorMenu(bool* pOpen);
        void showEditorWorldObjectsWindow(bool* pOpen);
        void showEditorFileContentWindow(bool* pOpen);
        void showEditorGameWindow(bool* pOpen);
        void showEditorDetailWindow(bool* pOpen);

        void setUIColorStyle();

    private:
        std::unordered_map<std::string, std::function<void(std::string, void*)>> mEditorUICreator;
        std::unordered_map<std::string, unsigned int>                            mNewObjectIndexMap;
        EditorFileService                                                        mEditorFileService;
        std::chrono::time_point<std::chrono::steady_clock>                       mLastFileTreeUpdate;

        bool mbEditorMenuWindowOpen      = true;
        bool mbAssetWindowOpen           = true;
        bool mbGameEngineWindowOpen      = true;
        bool mbFileContentWindowOpen     = true;
        bool mbDetailWindowOpen          = true;
        bool mbSceneLightsWindowOpen     = true;
        bool mbSceneLightsDataWindowOpen = true;
    };
}