#pragma once

namespace MiniEngine
{
    struct EditorGlobalContextInitInfo
    {
        class WindowSystem*  windowSystem;
        class RenderSystem*  renderSystem;
        class MEngine*       engineRuntime;
    };

    class EditorGlobalContext
    {
    public:
        void Initialize(const EditorGlobalContextInitInfo& initInfo);
        void Clear();

    public:
        class EditorSceneManager* mSceneManager {nullptr};
        class EditorInputManager* mInputManager {nullptr};
        class RenderSystem*       mRenderSystem {nullptr};
        class WindowSystem*       mWindowSystem {nullptr};
        class MEngine*            mEngineRuntime {nullptr};
    };

    extern EditorGlobalContext gEditorGlobalContext;
}