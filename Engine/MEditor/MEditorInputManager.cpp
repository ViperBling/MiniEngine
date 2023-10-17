#include "MEditorInputManager.hpp"

#include "MEditor/MEditor.hpp"
#include "MEditor/MEditorGlobalContext.hpp"
#include "MEditor/MEditorSceneManager.hpp"

#include "MRuntime/MEngine.hpp"
#include "MRuntime/Function/Framework/Scene/Scene.hpp"
#include "MRuntime/Function/Framework/World/WorldManager.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Input/InputSystem.hpp"

#include "MRuntime/Function/Render/RenderCamera.hpp"
#include "MRuntime/Function/Render/RenderSystem.hpp"
#include "MRuntime/Function/Render/WindowSystem.hpp"

namespace MiniEngine
{
    void EditorInputManager::Initialize()
    {
        RegisterInput();
    }

    void EditorInputManager::Tick(float delta_time)
    {
        ProcessEditorCommand();
    }

    void EditorInputManager::RegisterInput()
    {
        gEditorGlobalContext.mWindowSystem->RegisterOnResetFunc(std::bind(&EditorInputManager::OnReset, this));
        gEditorGlobalContext.mWindowSystem->RegisterOnCursorPosFunc(
            std::bind(&EditorInputManager::OnCursorPos, this, std::placeholders::_1, std::placeholders::_2));
        gEditorGlobalContext.mWindowSystem->RegisterOnCursorEnterFunc(
            std::bind(&EditorInputManager::OnCursorEnter, this, std::placeholders::_1));
        gEditorGlobalContext.mWindowSystem->RegisterOnScrollFunc(
            std::bind(&EditorInputManager::OnScroll, this, std::placeholders::_1, std::placeholders::_2));
        gEditorGlobalContext.mWindowSystem->RegisterOnMouseButtonFunc(
            std::bind(&EditorInputManager::OnMouseButtonClicked, this, std::placeholders::_1, std::placeholders::_2));
        gEditorGlobalContext.mWindowSystem->RegisterOnWindowCloseFunc(
            std::bind(&EditorInputManager::OnWindowClosed, this));
        gEditorGlobalContext.mWindowSystem->RegisterOnKeyFunc(
            std::bind(&EditorInputManager::OnKey,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2,
                            std::placeholders::_3,
                            std::placeholders::_4));
    }

    void EditorInputManager::UpdateCursorOnAxis(Vector2 cursor_uv)
    {
        if (gEditorGlobalContext.mSceneManager->GetEditorCamera())
        {
            Vector2 window_size(mEngineWindowSize.x, mEngineWindowSize.y);
            mCursorOnAxis = gEditorGlobalContext.mSceneManager->UpdateCursorOnAxis(cursor_uv, window_size);
        }
    }

    void EditorInputManager::ProcessEditorCommand()
    {
        float           camera_speed  = mCameraSpeed;
        std::shared_ptr editor_camera = gEditorGlobalContext.mSceneManager->GetEditorCamera();
        Quaternion      camera_rotate = editor_camera->Rotation().Inverse();
        Vector3         camera_relative_pos(0, 0, 0);

        if ((unsigned int)EditorCommand::Camera_Forward & mEditorCommand)
        {
            camera_relative_pos += camera_rotate * Vector3 {0, camera_speed, 0};
        }
        if ((unsigned int)EditorCommand::Camera_Back & mEditorCommand)
        {
            camera_relative_pos += camera_rotate * Vector3 {0, -camera_speed, 0};
        }
        if ((unsigned int)EditorCommand::Camera_Left & mEditorCommand)
        {
            camera_relative_pos += camera_rotate * Vector3 {-camera_speed, 0, 0};
        }
        if ((unsigned int)EditorCommand::Camera_Right & mEditorCommand)
        {
            camera_relative_pos += camera_rotate * Vector3 {camera_speed, 0, 0};
        }
        if ((unsigned int)EditorCommand::Camera_Up & mEditorCommand)
        {
            camera_relative_pos += Vector3 {0, 0, camera_speed};
        }
        if ((unsigned int)EditorCommand::Camera_Down & mEditorCommand)
        {
            camera_relative_pos += Vector3 {0, 0, -camera_speed};
        }
        if ((unsigned int)EditorCommand::Delete_Object & mEditorCommand)
        {
            gEditorGlobalContext.mSceneManager->OnDeleteSelectedGObject();
        }

        editor_camera->Move(camera_relative_pos);
    }

    void EditorInputManager::OnKeyInEditorMode(int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            switch (key)
            {
                case GLFW_KEY_A:
                    mEditorCommand |= (unsigned int)EditorCommand::Camera_Left;
                    break;
                case GLFW_KEY_S:
                    mEditorCommand |= (unsigned int)EditorCommand::Camera_Back;
                    break;
                case GLFW_KEY_W:
                    mEditorCommand |= (unsigned int)EditorCommand::Camera_Forward;
                    break;
                case GLFW_KEY_D:
                    mEditorCommand |= (unsigned int)EditorCommand::Camera_Right;
                    break;
                case GLFW_KEY_Q:
                    mEditorCommand |= (unsigned int)EditorCommand::Camera_Up;
                    break;
                case GLFW_KEY_E:
                    mEditorCommand |= (unsigned int)EditorCommand::Camera_Down;
                    break;
                case GLFW_KEY_T:
                    mEditorCommand |= (unsigned int)EditorCommand::Translation_Mode;
                    break;
                case GLFW_KEY_R:
                    mEditorCommand |= (unsigned int)EditorCommand::Rotation_Mode;
                    break;
                case GLFW_KEY_C:
                    mEditorCommand |= (unsigned int)EditorCommand::Scale_Mode;
                    break;
                case GLFW_KEY_DELETE:
                    mEditorCommand |= (unsigned int)EditorCommand::Delete_Object;
                    break;
                default:
                    break;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            switch (key)
            {
                case GLFW_KEY_ESCAPE:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Exit);
                    break;
                case GLFW_KEY_A:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Camera_Left);
                    break;
                case GLFW_KEY_S:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Camera_Back);
                    break;
                case GLFW_KEY_W:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Camera_Forward);
                    break;
                case GLFW_KEY_D:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Camera_Right);
                    break;
                case GLFW_KEY_Q:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Camera_Up);
                    break;
                case GLFW_KEY_E:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Camera_Down);
                    break;
                case GLFW_KEY_T:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Translation_Mode);
                    break;
                case GLFW_KEY_R:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Rotation_Mode);
                    break;
                case GLFW_KEY_C:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Scale_Mode);
                    break;
                case GLFW_KEY_DELETE:
                    mEditorCommand &= (kComplementControlCommand ^ (unsigned int)EditorCommand::Delete_Object);
                    break;
                default:
                    break;
            }
        }
    }

    void EditorInputManager::OnKey(int key, int scancode, int action, int mods)
    {
        if (gbIsEditorMode)
        {
            OnKeyInEditorMode(key, scancode, action, mods);
        }
    }

    void EditorInputManager::OnReset()
    {
    }

    void EditorInputManager::OnCursorPos(double xpos, double ypos)
    {
        if (!gbIsEditorMode)
            return;

        float angularVelocity = 180.0f / Math::Max(mEngineWindowSize.x, mEngineWindowSize.y); // 180 degrees while moving full screen
        if (mMouseX >= 0.0f && mMouseY >= 0.0f)
        {
            if (gEditorGlobalContext.mWindowSystem->IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
            {
                glfwSetInputMode(
                    gEditorGlobalContext.mWindowSystem->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                gEditorGlobalContext.mSceneManager->GetEditorCamera()->Rotate(
                    Vector2(ypos - mMouseY, xpos - mMouseX) * angularVelocity);
            }
            else if (gEditorGlobalContext.mWindowSystem->IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
            {
                gEditorGlobalContext.mSceneManager->MoveEntity(
                    xpos,
                    ypos,
                    mMouseX,
                    mMouseY,
                    mEngineWindowPos,
                    mEngineWindowSize,
                    mCursorOnAxis,
                    gEditorGlobalContext.mSceneManager->GetSelectedObjectMatrix());
                glfwSetInputMode(gEditorGlobalContext.mWindowSystem->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else
            {
                glfwSetInputMode(gEditorGlobalContext.mWindowSystem->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

                if (IsCursorInRect(mEngineWindowPos, mEngineWindowSize))
                {
                    Vector2 cursor_uv = Vector2((mMouseX - mEngineWindowPos.x) / mEngineWindowSize.x,
                                                (mMouseY - mEngineWindowPos.y) / mEngineWindowSize.y);
                    UpdateCursorOnAxis(cursor_uv);
                }
            }
        }
        mMouseX = xpos;
        mMouseY = ypos;
    }

    void EditorInputManager::OnCursorEnter(int entered)
    {
        if (!entered) // lost focus
        {
            mMouseX = mMouseY = -1.0f;
        }
    }

    void EditorInputManager::OnScroll(double xoffset, double yoffset)
    {
        if (!gbIsEditorMode)
        {
            return;
        }

        if (IsCursorInRect(mEngineWindowPos, mEngineWindowSize))
        {
            if (gEditorGlobalContext.mWindowSystem->IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
            {
                if (yoffset > 0)
                {
                    mCameraSpeed *= 1.2f;
                }
                else
                {
                    mCameraSpeed *= 0.8f;
                }
            }
            else
            {
                gEditorGlobalContext.mSceneManager->GetEditorCamera()->Zoom(
                    (float)yoffset * 2.0f); // wheel scrolled up = zoom in by 2 extra degrees
            }
        }
    }

    void EditorInputManager::OnMouseButtonClicked(int key, int action)
    {
        if (!gbIsEditorMode)
            return;
        if (mCursorOnAxis != 3)
            return;

        std::shared_ptr<Scene> current_active_level = gRuntimeGlobalContext.mWorldManager->GetCurrentActiveScene().lock();
        if (current_active_level == nullptr)
            return;

        if (IsCursorInRect(mEngineWindowPos, mEngineWindowSize))
        {
            if (key == GLFW_MOUSE_BUTTON_LEFT)
            {
                Vector2 picked_uv((mMouseX - mEngineWindowPos.x) / mEngineWindowSize.x,
                                  (mMouseY - mEngineWindowPos.y) / mEngineWindowSize.y);
                size_t  select_mesh_id = gEditorGlobalContext.mSceneManager->GetGuidOfPickedMesh(picked_uv);

                size_t gobject_id = gEditorGlobalContext.mRenderSystem->GetGObjectIDByMeshID(select_mesh_id);
                gEditorGlobalContext.mSceneManager->OnGObjectSelected(gobject_id);
            }
        }
    }

    void EditorInputManager::OnWindowClosed()
    {
        gEditorGlobalContext.mEngineRuntime->ShutdownEngine();
    }

    bool EditorInputManager::IsCursorInRect(Vector2 pos, Vector2 size) const
    {
        return pos.x <= mMouseX && mMouseX <= pos.x + size.x && pos.y <= mMouseY && mMouseY <= pos.y + size.y;
    }
}