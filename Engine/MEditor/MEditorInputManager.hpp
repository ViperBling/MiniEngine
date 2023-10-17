#pragma once

#include "MRuntime/Core/Math/Vector2.hpp"
#include "MRuntime/Function/Render/RenderCamera.hpp"

#include <vector>

namespace MiniEngine
{
    class MEditor;

    enum class EditorCommand : unsigned int
    {
        Camera_Left      = 1 << 0,  // A
        Camera_Back      = 1 << 1,  // S
        Camera_Forward    = 1 << 2,  // W
        Camera_Right     = 1 << 3,  // D
        Camera_Up        = 1 << 4,  // Q
        Camera_Down      = 1 << 5,  // E
        Translation_Mode = 1 << 6,  // T
        Rotation_Mode    = 1 << 7,  // R
        Scale_Mode       = 1 << 8,  // C
        Exit             = 1 << 9,  // Esc
        Delete_Object    = 1 << 10, // Delete
    };

    class EditorInputManager
    {
    public:
        void Initialize();
        void Tick(float delta_time);

        void RegisterInput();
        void UpdateCursorOnAxis(Vector2 cursor_uv);
        void ProcessEditorCommand();
        void OnKeyInEditorMode(int key, int scancode, int action, int mods);

        void OnKey(int key, int scancode, int action, int mods);
        void OnReset();
        void OnCursorPos(double xpos, double ypos);
        void OnCursorEnter(int entered);
        void OnScroll(double xoffset, double yoffset);
        void OnMouseButtonClicked(int key, int action);
        void OnWindowClosed();

        bool IsCursorInRect(Vector2 pos, Vector2 size) const;

        Vector2 GetEngineWindowPos() const { return mEngineWindowPos; };
        Vector2 GetEngineWindowSize() const { return mEngineWindowSize; };
        float   GetCameraSpeed() const { return mCameraSpeed; };

        void SetEngineWindowPos(Vector2 new_window_pos) { mEngineWindowPos = new_window_pos; };
        void SetEngineWindowSize(Vector2 new_window_size) { mEngineWindowSize = new_window_size; };
        void ResetEditorCommand() { mEditorCommand = 0; }

    private:
        Vector2 mEngineWindowPos {0.0f, 0.0f};
        Vector2 mEngineWindowSize {1280.0f, 768.0f};
        float   mMouseX {0.0f};
        float   mMouseY {0.0f};
        float   mCameraSpeed {0.05f};

        size_t       mCursorOnAxis {3};
        unsigned int mEditorCommand {0};
    };
}