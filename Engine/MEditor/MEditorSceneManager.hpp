#pragma once

#include "MEditor/Axis.hpp"

#include "MRuntime/Function/Framework/Object/Object.hpp"
#include "MRuntime/Function/Render/RenderObject.hpp"

#include <memory>

namespace MiniEngine
{
    class MEditor;
    class RenderCamera;
    class RenderEntity;

    enum class EditorAxisMode : int
    {
        TranslateMode = 0,
        RotateMode = 1,
        ScaleMode = 2,
        Default = 3
    };

    class EditorSceneManager
    {
    public:
        void Initialize();
        void Tick(float delta_time);

        size_t UpdateCursorOnAxis(Vector2 cursor_uv, Vector2 game_engine_window_size);
        void DrawSelectedEntityAxis();
        std::weak_ptr<GObject> GetSelectedGObject() const;
        RenderEntity* GetAxisMeshByType(EditorAxisMode axis_mode);
        void OnGObjectSelected(GObjectID selected_gobject_id);
        void OnDeleteSelectedGObject();
        void MoveEntity(
            float     new_mouse_pos_x,
            float     new_mouse_pos_y,
            float     last_mouse_pos_x,
            float     last_mouse_pos_y,
            Vector2   engine_window_pos,
            Vector2   engine_window_size,
            size_t    cursor_on_axis,
            Matrix4x4 model_matrix);
        
        void UploadAxisResource();
        size_t GetGuidOfPickedMesh(const Vector2& picked_uv) const;
        
        void SetEditorCamera(std::shared_ptr<RenderCamera> camera) { mCamera = camera; }
        std::shared_ptr<RenderCamera> GetEditorCamera() { return mCamera; };
        GObjectID GetSelectedObjectID() { return mSelectedGOjectID; };
        Matrix4x4 GetSelectedObjectMatrix() { return mSelectedObjectMatrix; }
        EditorAxisMode GetEditorAxisMode() { return mAxisMode; }

        void SetSelectedObjectID(GObjectID selected_gobject_id) { mSelectedGOjectID = selected_gobject_id; };
        void SetSelectedObjectMatrix(Matrix4x4 new_object_matrix) { mSelectedObjectMatrix = new_object_matrix; }
        void SetEditorAxisMode(EditorAxisMode new_axis_mode) { mAxisMode = new_axis_mode; }

    private:
        EditorTranslationAxis mTranslationAxis;
        EditorRotationAxis    mRotationAxis;
        EditorScaleAxis       mScaleAixs;

        GObjectID mSelectedGOjectID { kInvalidGObjectID };
        Matrix4x4 mSelectedObjectMatrix { Matrix4x4::IDENTITY };

        EditorAxisMode mAxisMode{ EditorAxisMode::TranslateMode };
        std::shared_ptr<RenderCamera> mCamera;

        size_t mSelectedAxis { 3 };

        bool   mbIsShowAxis = true;
    };
    
}