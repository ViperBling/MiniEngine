#include "MEditorUI.hpp"

#include "MEditor/MEditorGlobalContext.hpp"
#include "MEditor/MEditorInputManager.hpp"
#include "MEditor/MEditorSceneManager.hpp"

#include "MRuntime/Core/Base/Marco.hpp"
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

#include "MRuntime/Platform/Path/Path.hpp"

#include "MRuntime/Resource/AssetManager/AssetManager.hpp"
#include "MRuntime/Resource/ConfigManager/ConfigManager.hpp"

#include "MRuntime/MEngine.hpp"

#include "MRuntime/Function/Framework/Component/MeshComponent/MeshComponent.hpp"
#include "MRuntime/Function/Framework/Component/TransformComponent/TransformComponent.hpp"
#include "MRuntime/Function/Framework/Scene/Scene.hpp"
#include "MRuntime/Function/Framework/World/WorldManager.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Input/InputSystem.hpp"
#include "MRuntime/Function/Render/RenderCamera.hpp"
#include "MRuntime/Function/Render/RenderSystem.hpp"
#include "MRuntime/Function/Render/WindowSystem.hpp"
#include "MRuntime/Function/Render/RenderDebugConfig.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <stb_image.h>

namespace MiniEngine
{
    std::vector<std::pair<std::string, bool>> gEditorNodeStateArray;
    int                                       gNodeDepth = -1;
    void DrawVecControl(
        const std::string& label,
        MiniEngine::Vector3&    values,
        float              resetValue  = 0.0f,
        float              columnWidth = 100.0f
    );
    void  DrawVecControl(
        const std::string& label,
        MiniEngine::Quaternion& values,
        float              resetValue  = 0.0f,
        float              columnWidth = 100.0f
    );

    MEditorUI::MEditorUI() 
    {
        const auto& asset_folder            = gRuntimeGlobalContext.mConfigManager->GetAssetFolder();
        mEditorUICreator["TreeNodePush"] = [this](const std::string& name, void* value_ptr) -> void {
            static ImGuiTableFlags flags      = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings;
            bool                   node_state = false;
            gNodeDepth++;
            if (gNodeDepth > 0)
            {
                if (gEditorNodeStateArray[gNodeDepth - 1].second)
                {
                    node_state = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
                }
                else
                {
                    gEditorNodeStateArray.emplace_back(std::pair(name.c_str(), node_state));
                    return;
                }
            }
            else
            {
                node_state = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
            }
            gEditorNodeStateArray.emplace_back(std::pair(name.c_str(), node_state));
        };
        mEditorUICreator["TreeNodePop"] = [this](const std::string& name, void* value_ptr) -> void {
            if (gEditorNodeStateArray[gNodeDepth].second)
            {
                ImGui::TreePop();
            }
            gEditorNodeStateArray.pop_back();
            gNodeDepth--;
        };
        mEditorUICreator["Transform"] = [this](const std::string& name, void* value_ptr) -> void {
            if (gEditorNodeStateArray[gNodeDepth].second)
            {
                Transform* trans_ptr = static_cast<Transform*>(value_ptr);

                Vector3 degrees_val;

                degrees_val.x = trans_ptr->m_rotation.GetPitch(false).ValueDegrees();
                degrees_val.y = trans_ptr->m_rotation.GetRoll(false).ValueDegrees();
                degrees_val.z = trans_ptr->m_rotation.GetYaw(false).ValueDegrees();

                DrawVecControl("Position", trans_ptr->m_position);
                DrawVecControl("Rotation", degrees_val);
                DrawVecControl("Scale", trans_ptr->m_scale);

                trans_ptr->m_rotation.w = Math::Cos(Math::DegreesToRadians(degrees_val.x / 2)) *
                                              Math::Cos(Math::DegreesToRadians(degrees_val.y / 2)) *
                                              Math::Cos(Math::DegreesToRadians(degrees_val.z / 2)) +
                                          Math::Sin(Math::DegreesToRadians(degrees_val.x / 2)) *
                                              Math::Sin(Math::DegreesToRadians(degrees_val.y / 2)) *
                                              Math::Sin(Math::DegreesToRadians(degrees_val.z / 2));
                trans_ptr->m_rotation.x = Math::Sin(Math::DegreesToRadians(degrees_val.x / 2)) *
                                              Math::Cos(Math::DegreesToRadians(degrees_val.y / 2)) *
                                              Math::Cos(Math::DegreesToRadians(degrees_val.z / 2)) -
                                          Math::Cos(Math::DegreesToRadians(degrees_val.x / 2)) *
                                              Math::Sin(Math::DegreesToRadians(degrees_val.y / 2)) *
                                              Math::Sin(Math::DegreesToRadians(degrees_val.z / 2));
                trans_ptr->m_rotation.y = Math::Cos(Math::DegreesToRadians(degrees_val.x / 2)) *
                                              Math::Sin(Math::DegreesToRadians(degrees_val.y / 2)) *
                                              Math::Cos(Math::DegreesToRadians(degrees_val.z / 2)) +
                                          Math::Sin(Math::DegreesToRadians(degrees_val.x / 2)) *
                                              Math::Cos(Math::DegreesToRadians(degrees_val.y / 2)) *
                                              Math::Sin(Math::DegreesToRadians(degrees_val.z / 2));
                trans_ptr->m_rotation.z = Math::Cos(Math::DegreesToRadians(degrees_val.x / 2)) *
                                              Math::Cos(Math::DegreesToRadians(degrees_val.y / 2)) *
                                              Math::Sin(Math::DegreesToRadians(degrees_val.z / 2)) -
                                          Math::Sin(Math::DegreesToRadians(degrees_val.x / 2)) *
                                              Math::Sin(Math::DegreesToRadians(degrees_val.y / 2)) *
                                              Math::Cos(Math::DegreesToRadians(degrees_val.z / 2));
                trans_ptr->m_rotation.Normalize();

                gEditorGlobalContext.mSceneManager->DrawSelectedEntityAxis();
            }
        };
        mEditorUICreator["bool"] = [this](const std::string& name, void* value_ptr)  -> void {
            if(gNodeDepth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::Checkbox(label.c_str(), static_cast<bool*>(value_ptr));
            }
            else
            {
                if(gEditorNodeStateArray[gNodeDepth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", name.c_str());
                    ImGui::Checkbox(full_label.c_str(), static_cast<bool*>(value_ptr));
                }
            }
        };
        mEditorUICreator["int"] = [this](const std::string& name, void* value_ptr) -> void {
            if (gNodeDepth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::InputInt(label.c_str(), static_cast<int*>(value_ptr));
            }
            else
            {
                if (gEditorNodeStateArray[gNodeDepth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    ImGui::InputInt(full_label.c_str(), static_cast<int*>(value_ptr));
                }
            }
        };
        mEditorUICreator["float"] = [this](const std::string& name, void* value_ptr) -> void {
            if (gNodeDepth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::InputFloat(label.c_str(), static_cast<float*>(value_ptr));
            }
            else
            {
                if (gEditorNodeStateArray[gNodeDepth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    ImGui::InputFloat(full_label.c_str(), static_cast<float*>(value_ptr));
                }
            }
        };
        mEditorUICreator["Vector3"] = [this](const std::string& name, void* value_ptr) -> void {
            Vector3* vec_ptr = static_cast<Vector3*>(value_ptr);
            float    val[3]  = {vec_ptr->x, vec_ptr->y, vec_ptr->z};
            if (gNodeDepth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::DragFloat3(label.c_str(), val);
            }
            else
            {
                if (gEditorNodeStateArray[gNodeDepth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    ImGui::DragFloat3(full_label.c_str(), val);
                }
            }
            vec_ptr->x = val[0];
            vec_ptr->y = val[1];
            vec_ptr->z = val[2];
        };
        mEditorUICreator["Quaternion"] = [this](const std::string& name, void* value_ptr) -> void {
            Quaternion* qua_ptr = static_cast<Quaternion*>(value_ptr);
            float       val[4]  = {qua_ptr->x, qua_ptr->y, qua_ptr->z, qua_ptr->w};
            if (gNodeDepth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::DragFloat4(label.c_str(), val);
            }
            else
            {
                if (gEditorNodeStateArray[gNodeDepth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    ImGui::DragFloat4(full_label.c_str(), val);
                }
            }
            qua_ptr->x = val[0];
            qua_ptr->y = val[1];
            qua_ptr->z = val[2];
            qua_ptr->w = val[3];
        };
        mEditorUICreator["std::string"] = [this, &asset_folder](const std::string& name, void* value_ptr) -> void {
            if (gNodeDepth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::Text("%s", (*static_cast<std::string*>(value_ptr)).c_str());
            }
            else
            {
                if (gEditorNodeStateArray[gNodeDepth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    std::string value_str = *static_cast<std::string*>(value_ptr);
                    if (value_str.find_first_of('/') != std::string::npos)
                    {
                        std::filesystem::path value_path(value_str);
                        if (value_path.is_absolute())
                        {
                            value_path = Path::GetRelativePath(asset_folder, value_path);
                        }
                        value_str = value_path.generic_string();
                        if (value_str.size() >= 2 && value_str[0] == '.' && value_str[1] == '.')
                        {
                            value_str.clear();
                        }
                    }
                    ImGui::Text("%s", value_str.c_str());
                }
            }
        };
    }

    inline void windowContentScaleUpdate(float scale)
    {
#if defined(__GNUC__) && defined(__MACH__)
        float font_scale               = fmaxf(1.0f, scale);
        ImGui::GetIO().FontGlobalScale = 1.0f / font_scale;
#endif
        // TOOD: Reload fonts if DPI scale is larger than previous font loading DPI scale
    }

    inline void windowContentScaleCallback(GLFWwindow* window, float x_scale, float y_scale)
    {
        windowContentScaleUpdate(fmaxf(x_scale, y_scale));
    }

    void MEditorUI::Initialize(WindowUIInitInfo init_info)
    {
        std::shared_ptr<ConfigManager> config_manager = gRuntimeGlobalContext.mConfigManager;
        ASSERT(config_manager);

        // create imgui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // set ui content scale
        float x_scale, y_scale;
        glfwGetWindowContentScale(init_info.mWindowSystem->GetWindow(), &x_scale, &y_scale);
        float content_scale = fmaxf(1.0f, fmaxf(x_scale, y_scale));
        windowContentScaleUpdate(content_scale);
        glfwSetWindowContentScaleCallback(init_info.mWindowSystem->GetWindow(), windowContentScaleCallback);

        // load font for imgui
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigDockingAlwaysTabBar         = true;
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.Fonts->AddFontFromFileTTF(
            config_manager->GetEditorFontPath().generic_string().data(), content_scale * 16, nullptr, nullptr);
        io.Fonts->Build();

        ImGuiStyle& style     = ImGui::GetStyle();
        style.WindowPadding   = ImVec2(1.0, 0);
        style.FramePadding    = ImVec2(14.0, 2.0f);
        style.ChildBorderSize = 0.0f;
        style.FrameRounding   = 5.0f;
        style.FrameBorderSize = 1.5f;

        // set imgui color style
        setUIColorStyle();

        // setup window icon
        GLFWimage   window_icon[2];
        std::string big_icon_path_string   = config_manager->GetEditorBigIconPath().generic_string();
        std::string small_icon_path_string = config_manager->GetEditorSmallIconPath().generic_string();
        window_icon[0].pixels =
            stbi_load(big_icon_path_string.data(), &window_icon[0].width, &window_icon[0].height, 0, 4);
        window_icon[1].pixels =
            stbi_load(small_icon_path_string.data(), &window_icon[1].width, &window_icon[1].height, 0, 4);
        glfwSetWindowIcon(init_info.mWindowSystem->GetWindow(), 2, window_icon);
        stbi_image_free(window_icon[0].pixels);
        stbi_image_free(window_icon[1].pixels);

        // initialize imgui vulkan render backend
        init_info.mRenderSystem->InitializeUIRenderBackend(this);
    }


    void MEditorUI::PreRender()
    {
        showEditorUI();
    }

    void MEditorUI::onFileContentItemClicked(EditorFileNode *node)
    {
        if (node->mFileType != "object")
            return;

        std::shared_ptr<Scene> level = gRuntimeGlobalContext.mWorldManager->GetCurrentActiveScene().lock();
        if (level == nullptr)
            return;

        const unsigned int new_object_index = ++mNewObjectIndexMap[node->mFileName];

        ObjectInstanceRes new_object_instance_res;
        new_object_instance_res.mName =
            "New_" + Path::GetFilePureName(node->mFileName) + "_" + std::to_string(new_object_index);
        new_object_instance_res.mDefinition =
            gRuntimeGlobalContext.mAssetManager->GetFullPath(node->mFilePath).generic_string();

        size_t new_gobject_id = level->CreateObject(new_object_instance_res);
        if (new_gobject_id != kInvalidGObjectID)
        {
            gEditorGlobalContext.mSceneManager->OnGObjectSelected(new_gobject_id);
        }
    }

    void MEditorUI::buildEditorFileAssetsUITree(EditorFileNode *node)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        const bool is_folder = (node->mChildNodes.size() > 0);
        if (is_folder)
        {
            bool open = ImGui::TreeNodeEx(node->mFileName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(100.0f);
            ImGui::TextUnformatted(node->mFileType.c_str());
            if (open)
            {
                for (int child_n = 0; child_n < node->mChildNodes.size(); child_n++)
                    buildEditorFileAssetsUITree(node->mChildNodes[child_n].get());
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::TreeNodeEx(node->mFileName.c_str(),
                              ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                  ImGuiTreeNodeFlags_SpanFullWidth);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                onFileContentItemClicked(node);
            }
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(100.0f);
            ImGui::TextUnformatted(node->mFileType.c_str());
        }
    }

    void MEditorUI::drawAxisToggleButton(const char *stringID, bool checkState, int axisMode)
    {
        if (checkState)
        {
            ImGui::PushID(stringID);
            ImVec4 check_button_color = ImVec4(93.0f / 255.0f, 10.0f / 255.0f, 66.0f / 255.0f, 1.00f);
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(check_button_color.x, check_button_color.y, check_button_color.z, 0.40f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, check_button_color);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, check_button_color);
            ImGui::Button(stringID);
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
        else
        {
            if (ImGui::Button(stringID))
            {
                checkState = true;
                gEditorGlobalContext.mSceneManager->SetEditorAxisMode((EditorAxisMode)axisMode);
                gEditorGlobalContext.mSceneManager->DrawSelectedEntityAxis();
            }
        }
    }

    void MEditorUI::createClassUI(Reflection::ReflectionInstance &instance)
    {
        Reflection::ReflectionInstance* reflection_instance;
        int count = instance.mMeta.GetBaseClassReflectionInstanceList(reflection_instance, instance.mInstance);
        for (int index = 0; index < count; index++)
        {
            createClassUI(reflection_instance[index]);
        }
        createLeafNodeUI(instance);

        if (count > 0)
            delete[] reflection_instance;
    }

    void MEditorUI::createLeafNodeUI(Reflection::ReflectionInstance &instance)
    {
        Reflection::FieldAccessor* fields;
        int                        fields_count = instance.mMeta.GetFieldsList(fields);

        for (size_t index = 0; index < fields_count; index++)
        {
            auto field = fields[index];
            if (field.IsArrayType())
            {
                Reflection::ArrayAccessor array_accessor;
                if (Reflection::TypeMeta::NewArrayAccessorFromName(field.GetFieldTypeName(), array_accessor))
                {
                    void* field_instance = field.Get(instance.mInstance);
                    int   array_count    = array_accessor.GetSize(field_instance);
                    mEditorUICreator["TreeNodePush"](
                        std::string(field.GetFieldName()) + "[" + std::to_string(array_count) + "]", nullptr);
                    auto item_type_meta_item =
                        Reflection::TypeMeta::NewMetaFromName(array_accessor.GetElementTypeName());
                    auto item_ui_creator_iterator = mEditorUICreator.find(item_type_meta_item.GetTypeName());
                    for (int index = 0; index < array_count; index++)
                    {
                        if (item_ui_creator_iterator == mEditorUICreator.end())
                        {
                            mEditorUICreator["TreeNodePush"]("[" + std::to_string(index) + "]", nullptr);
                            auto object_instance = Reflection::ReflectionInstance(
                                MiniEngine::Reflection::TypeMeta::NewMetaFromName(item_type_meta_item.GetTypeName().c_str()),
                                array_accessor.Get(index, field_instance));
                            createClassUI(object_instance);
                            mEditorUICreator["TreeNodePop"]("[" + std::to_string(index) + "]", nullptr);
                        }
                        else
                        {
                            if (item_ui_creator_iterator == mEditorUICreator.end())
                            {
                                continue;
                            }
                            mEditorUICreator[item_type_meta_item.GetTypeName()](
                                "[" + std::to_string(index) + "]", array_accessor.Get(index, field_instance));
                        }
                    }
                    mEditorUICreator["TreeNodePop"](field.GetFieldName(), nullptr);
                }
            }
            auto ui_creator_iterator = mEditorUICreator.find(field.GetFieldTypeName());
            if (ui_creator_iterator == mEditorUICreator.end())
            {
                Reflection::TypeMeta field_meta =
                    Reflection::TypeMeta::NewMetaFromName(field.GetFieldTypeName());
                if (field.GetTypeMeta(field_meta))
                {
                    auto child_instance =
                        Reflection::ReflectionInstance(field_meta, field.Get(instance.mInstance));
                    mEditorUICreator["TreeNodePush"](field_meta.GetTypeName(), nullptr);
                    createClassUI(child_instance);
                    mEditorUICreator["TreeNodePop"](field_meta.GetTypeName(), nullptr);
                }
                else
                {
                    if (ui_creator_iterator == mEditorUICreator.end())
                    {
                        continue;
                    }
                    mEditorUICreator[field.GetFieldTypeName()](field.GetFieldName(),
                                                                         field.Get(instance.mInstance));
                }
            }
            else
            {
                mEditorUICreator[field.GetFieldTypeName()](field.GetFieldName(),
                                                                     field.Get(instance.mInstance));
            }
        }
        delete[] fields;
    }

    std::string MEditorUI::getLeafUINodeParentLabel()
    {
        std::string parent_label;
        int         array_size = gEditorNodeStateArray.size();
        for (int index = 0; index < array_size; index++)
        {
            parent_label += gEditorNodeStateArray[index].first + "::";
        }
        return parent_label;
    }

    void MEditorUI::showEditorUI()
    {
        showEditorMenu(&mbEditorMenuWindowOpen);
        showEditorWorldObjectsWindow(&mbAssetWindowOpen);
        showEditorGameWindow(&mbGameEngineWindowOpen);
        showEditorFileContentWindow(&mbFileContentWindowOpen);
        showEditorDetailWindow(&mbDetailWindowOpen);
    }

    void MEditorUI::showEditorMenu(bool *pOpen)
    {
        ImGuiDockNodeFlags dock_flags   = ImGuiDockNodeFlags_DockSpace;
        ImGuiWindowFlags   window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
                                        ImGuiConfigFlags_NoMouseCursorChange | ImGuiWindowFlags_NoBringToFrontOnFocus;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(main_viewport->WorkPos, ImGuiCond_Always);
        std::array<int, 2> window_size = gEditorGlobalContext.mWindowSystem->GetWindowSize();
        ImGui::SetNextWindowSize(ImVec2((float)window_size[0], (float)window_size[1]), ImGuiCond_Always);

        ImGui::SetNextWindowViewport(main_viewport->ID);

        ImGui::Begin("Editor menu", pOpen, window_flags);

        ImGuiID main_docking_id = ImGui::GetID("Main Docking");
        if (ImGui::DockBuilderGetNode(main_docking_id) == nullptr)
        {
            ImGui::DockBuilderRemoveNode(main_docking_id);

            ImGui::DockBuilderAddNode(main_docking_id, dock_flags);
            ImGui::DockBuilderSetNodePos(main_docking_id,
                                         ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y + 18.0f));
            ImGui::DockBuilderSetNodeSize(main_docking_id,
                                          ImVec2((float)window_size[0], (float)window_size[1] - 18.0f));

            ImGuiID center = main_docking_id;
            ImGuiID left;
            ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &left);

            ImGuiID left_other;
            ImGuiID left_file_content = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.30f, nullptr, &left_other);

            ImGuiID left_game_engine;
            ImGuiID left_asset =
                ImGui::DockBuilderSplitNode(left_other, ImGuiDir_Left, 0.30f, nullptr, &left_game_engine);

            ImGui::DockBuilderDockWindow("World Objects", left_asset);
            ImGui::DockBuilderDockWindow("Components Details", right);
            ImGui::DockBuilderDockWindow("File Content", left_file_content);
            ImGui::DockBuilderDockWindow("Game Engine", left_game_engine);

            ImGui::DockBuilderFinish(main_docking_id);
        }

        ImGui::DockSpace(main_docking_id);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Menu"))
            {
                if (ImGui::MenuItem("Reload Current Level"))
                {
                    gRuntimeGlobalContext.mWorldManager->ReloadCurrentScene();
                    gRuntimeGlobalContext.mRenderSystem->ClearForLevelReloading();
                    gEditorGlobalContext.mSceneManager->OnGObjectSelected(kInvalidGObjectID);
                }
                if (ImGui::MenuItem("Save Current Level"))
                {
                    gRuntimeGlobalContext.mWorldManager->SaveCurrentScene();
                }
                if (ImGui::BeginMenu("Debug"))
                {
                    if (ImGui::BeginMenu("Animation"))
                    {
                        if (ImGui::MenuItem(gRuntimeGlobalContext.mRenderDebugConfig->animation.mbShowSkeleton ? "off skeleton" : "show skeleton"))
                        {
                            gRuntimeGlobalContext.mRenderDebugConfig->animation.mbShowSkeleton = !gRuntimeGlobalContext.mRenderDebugConfig->animation.mbShowSkeleton;
                        }
                        if (ImGui::MenuItem(gRuntimeGlobalContext.mRenderDebugConfig->animation.mbShowBoneName ? "off bone name" : "show bone name"))
                        {
                            gRuntimeGlobalContext.mRenderDebugConfig->animation.mbShowBoneName = !gRuntimeGlobalContext.mRenderDebugConfig->animation.mbShowBoneName;
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Camera"))
                    {
                        if (ImGui::MenuItem(gRuntimeGlobalContext.mRenderDebugConfig->camera.mbShowRuntimeInfo ? "off runtime info" : "show runtime info"))
                        {
                            gRuntimeGlobalContext.mRenderDebugConfig->camera.mbShowRuntimeInfo = !gRuntimeGlobalContext.mRenderDebugConfig->camera.mbShowRuntimeInfo;
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Game Object"))
                    {
                        if (ImGui::MenuItem(gRuntimeGlobalContext.mRenderDebugConfig->gameObject.mbShowBoundingBox ? "off bounding box" : "show bounding box"))
                        {
                            gRuntimeGlobalContext.mRenderDebugConfig->gameObject.mbShowBoundingBox = !gRuntimeGlobalContext.mRenderDebugConfig->gameObject.mbShowBoundingBox;
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Exit"))
                {
                    gEditorGlobalContext.mEngineRuntime->ShutdownEngine();
                    exit(0);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window"))
            {
                ImGui::MenuItem("World Objects", nullptr, &mbAssetWindowOpen);
                ImGui::MenuItem("Game", nullptr, &mbGameEngineWindowOpen);
                ImGui::MenuItem("File Content", nullptr, &mbFileContentWindowOpen);
                ImGui::MenuItem("Detail", nullptr, &mbDetailWindowOpen);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();
    }

    void MEditorUI::showEditorWorldObjectsWindow(bool *pOpen)
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (!*pOpen)
            return;

        if (!ImGui::Begin("World Objects", pOpen, window_flags))
        {
            ImGui::End();
            return;
        }

        std::shared_ptr<Scene> current_active_level =
            gRuntimeGlobalContext.mWorldManager->GetCurrentActiveScene().lock();
        if (current_active_level == nullptr)
            return;

        const SceneObjectsMap& all_gobjects = current_active_level->GetAllGObjects();
        for (auto& id_object_pair : all_gobjects)
        {
            const GObjectID          object_id = id_object_pair.first;
            std::shared_ptr<GObject> object    = id_object_pair.second;
            const std::string        name      = object->GetName();
            if (name.size() > 0)
            {
                if (ImGui::Selectable(name.c_str(),
                                      gEditorGlobalContext.mSceneManager->GetSelectedObjectID() == object_id))
                {
                    if (gEditorGlobalContext.mSceneManager->GetSelectedObjectID() != object_id)
                    {
                        gEditorGlobalContext.mSceneManager->OnGObjectSelected(object_id);
                    }
                    else
                    {
                        gEditorGlobalContext.mSceneManager->OnGObjectSelected(kInvalidGObjectID);
                    }
                    break;
                }
            }
        }
        ImGui::End();
    }

    void MEditorUI::showEditorFileContentWindow(bool *pOpen)
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (!*pOpen)
            return;

        if (!ImGui::Begin("File Content", pOpen, window_flags))
        {
            ImGui::End();
            return;
        }

        static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                                       ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("File Content", 2, flags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            auto current_time = std::chrono::steady_clock::now();
            if (current_time - mLastFileTreeUpdate > std::chrono::seconds(1))
            {
                mEditorFileService.BuildEngineFileTree();
                mLastFileTreeUpdate = current_time;
            }
            mLastFileTreeUpdate = current_time;

            EditorFileNode* editor_root_node = mEditorFileService.GetEditorRootNode();
            buildEditorFileAssetsUITree(editor_root_node);
            ImGui::EndTable();
        }

        // file image list

        ImGui::End();
    }

    void MEditorUI::showEditorGameWindow(bool *pOpen)
    {
        ImGuiIO&         io           = ImGui::GetIO();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (!*pOpen)
            return;

        if (!ImGui::Begin("Game Engine", pOpen, window_flags))
        {
            ImGui::End();
            return;
        }

        static bool trans_button_ckecked  = false;
        static bool rotate_button_ckecked = false;
        static bool scale_button_ckecked  = false;

        switch (gEditorGlobalContext.mSceneManager->GetEditorAxisMode())
        {
            case EditorAxisMode::TranslateMode:
                trans_button_ckecked  = true;
                rotate_button_ckecked = false;
                scale_button_ckecked  = false;
                break;
            case EditorAxisMode::RotateMode:
                trans_button_ckecked  = false;
                rotate_button_ckecked = true;
                scale_button_ckecked  = false;
                break;
            case EditorAxisMode::ScaleMode:
                trans_button_ckecked  = false;
                rotate_button_ckecked = false;
                scale_button_ckecked  = true;
                break;
            default:
                break;
        }

        if (ImGui::BeginMenuBar())
        {
            ImGui::Indent(10.f);
            drawAxisToggleButton("Trans", trans_button_ckecked, (int)EditorAxisMode::TranslateMode);
            ImGui::Unindent();

            ImGui::SameLine();

            drawAxisToggleButton("Rotate", rotate_button_ckecked, (int)EditorAxisMode::RotateMode);

            ImGui::SameLine();

            drawAxisToggleButton("Scale", scale_button_ckecked, (int)EditorAxisMode::ScaleMode);

            ImGui::SameLine();

            float indent_val = 0.0f;

#if defined(__GNUC__) && defined(__MACH__)
            float indent_scale = 1.0f;
#else // Not tested on Linux
            float x_scale, y_scale;
            glfwGetWindowContentScale(gEditorGlobalContext.mWindowSystem->GetWindow(), &x_scale, &y_scale);
            float indent_scale = fmaxf(1.0f, fmaxf(x_scale, y_scale));
#endif
            indent_val = gEditorGlobalContext.mInputManager->GetEngineWindowSize().x - 100.0f * indent_scale;

            ImGui::Indent(indent_val);
            if (gbIsEditorMode)
            {
                ImGui::PushID("Editor Mode");
                if (ImGui::Button("Editor Mode"))
                {
                    gbIsEditorMode = false;
                    gEditorGlobalContext.mSceneManager->DrawSelectedEntityAxis();
                    gEditorGlobalContext.mInputManager->ResetEditorCommand();
                    gEditorGlobalContext.mWindowSystem->SetFocusMode(true);
                }
                ImGui::PopID();
            }
            else
            {
                if (ImGui::Button("Game Mode"))
                {
                    gbIsEditorMode = true;
                    gEditorGlobalContext.mSceneManager->DrawSelectedEntityAxis();
                    gRuntimeGlobalContext.mInputSystem->ResetGameCommand();
                    gEditorGlobalContext.mRenderSystem->GetRenderCamera()->SetMainViewMatrix(
                        gEditorGlobalContext.mSceneManager->GetEditorCamera()->GetViewMatrix());
                }
            }

            ImGui::Unindent();
            ImGui::EndMenuBar();
        }

        if (!gbIsEditorMode)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Press Left Alt key to display the mouse cursor!");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                               "Current editor camera move speed: [%f]",
                               gEditorGlobalContext.mInputManager->GetCameraSpeed());
        }

        // GetWindowPos() ----->  X--------------------------------------------O
        //                        |                                            |
        //                        |                                            |
        // menu_bar_rect.Min -->  X--------------------------------------------O
        //                        |    It is the menu bar window.              |
        //                        |                                            |
        //                        O--------------------------------------------X  <-- menu_bar_rect.Max
        //                        |                                            |
        //                        |     It is the render target window.        |
        //                        |                                            |
        //                        O--------------------------------------------O

        Vector2 render_target_window_pos = { 0.0f, 0.0f };
        Vector2 render_target_window_size = { 0.0f, 0.0f };

        auto menu_bar_rect = ImGui::GetCurrentWindow()->MenuBarRect();

        render_target_window_pos.x = ImGui::GetWindowPos().x;
        render_target_window_pos.y = menu_bar_rect.Max.y;
        render_target_window_size.x = ImGui::GetWindowSize().x;
        render_target_window_size.y = (ImGui::GetWindowSize().y + ImGui::GetWindowPos().y) - menu_bar_rect.Max.y; // coord of right bottom point of full window minus coord of right bottom point of menu bar window.

        // if (new_window_pos != m_engine_window_pos || new_window_size != m_engine_window_size)
        {
#if defined(__MACH__)
            // The dpi_scale is not reactive to DPI changes or monitor switching, it might be a bug from ImGui.
            // Return value from ImGui::GetMainViewport()->DpiScal is always the same as first frame.
            // glfwGetMonitorContentScale and glfwSetWindowContentScaleCallback are more adaptive.
            float dpi_scale = main_viewport->DpiScale;
            gRuntimeGlobalContext.mRenderSystem->updateEngineContentViewport(render_target_window_pos.x * dpi_scale,
                render_target_window_pos.y * dpi_scale,
                render_target_window_size.x * dpi_scale,
                render_target_window_size.y * dpi_scale);
#else
            gRuntimeGlobalContext.mRenderSystem->UpdateEngineContentViewport(
                render_target_window_pos.x, render_target_window_pos.y, render_target_window_size.x, render_target_window_size.y);
#endif
            gEditorGlobalContext.mInputManager->SetEngineWindowPos(render_target_window_pos);
            gEditorGlobalContext.mInputManager->SetEngineWindowSize(render_target_window_size);
        }

        ImGui::End();
    }

    void MEditorUI::showEditorDetailWindow(bool *pOpen)
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (!*pOpen)
            return;

        if (!ImGui::Begin("Components Details", pOpen, window_flags))
        {
            ImGui::End();
            return;
        }

        std::shared_ptr<GObject> selected_object = gEditorGlobalContext.mSceneManager->GetSelectedGObject().lock();
        if (selected_object == nullptr)
        {
            ImGui::End();
            return;
        }

        const std::string& name = selected_object->GetName();
        static char        cname[128];
        memset(cname, 0, 128);
        memcpy(cname, name.c_str(), name.size());

        ImGui::Text("Name");
        ImGui::SameLine();
        ImGui::InputText("##Name", cname, IM_ARRAYSIZE(cname), ImGuiInputTextFlags_ReadOnly);

        static ImGuiTableFlags flags                      = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings;
        auto&&                 selected_object_components = selected_object->GetComponents();
        for (auto component_ptr : selected_object_components)
        {
            mEditorUICreator["TreeNodePush"](("<" + component_ptr.GetTypeName() + ">").c_str(), nullptr);
            auto object_instance = Reflection::ReflectionInstance(
                MiniEngine::Reflection::TypeMeta::NewMetaFromName(component_ptr.GetTypeName().c_str()),
                component_ptr.operator->());
            createClassUI(object_instance);
            mEditorUICreator["TreeNodePop"](("<" + component_ptr.GetTypeName() + ">").c_str(), nullptr);
        }
        ImGui::End();
    }

    void MEditorUI::setUIColorStyle()
    {
        ImGuiStyle* style  = &ImGui::GetStyle();
        ImVec4*     colors = style->Colors;

        colors[ImGuiCol_Text]                  = ImVec4(0.4745f, 0.4745f, 0.4745f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.0078f, 0.0078f, 0.0078f, 1.00f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border]                = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.047f, 0.047f, 0.047f, 0.5411f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.196f, 0.196f, 0.196f, 0.40f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.294f, 0.294f, 0.294f, 0.67f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.0039f, 0.0039f, 0.0039f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.0039f, 0.0039f, 0.0039f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]             = ImVec4(93.0f / 255.0f, 10.0f / 255.0f, 66.0f / 255.0f, 1.00f);
        colors[ImGuiCol_SliderGrab]            = colors[ImGuiCol_CheckMark];
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.3647f, 0.0392f, 0.2588f, 0.50f);
        colors[ImGuiCol_Button]                = ImVec4(0.0117f, 0.0117f, 0.0117f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.0235f, 0.0235f, 0.0235f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.0353f, 0.0196f, 0.0235f, 1.00f);
        colors[ImGuiCol_Header]                = ImVec4(0.1137f, 0.0235f, 0.0745f, 0.588f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(5.0f / 255.0f, 5.0f / 255.0f, 5.0f / 255.0f, 1.00f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
        colors[ImGuiCol_Separator]             = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);
        colors[ImGuiCol_SeparatorHovered]      = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 1.00f);
        colors[ImGuiCol_SeparatorActive]       = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 1.00f);
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab]                   = ImVec4(6.0f / 255.0f, 6.0f / 255.0f, 8.0f / 255.0f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 150.0f / 255.0f);
        colors[ImGuiCol_TabActive]             = ImVec4(47.0f / 255.0f, 6.0f / 255.0f, 29.0f / 255.0f, 1.0f);
        colors[ImGuiCol_TabUnfocused]          = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 25.0f / 255.0f);
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(6.0f / 255.0f, 6.0f / 255.0f, 8.0f / 255.0f, 200.0f / 255.0f);
        colors[ImGuiCol_DockingPreview]        = ImVec4(47.0f / 255.0f, 6.0f / 255.0f, 29.0f / 255.0f, 0.7f);
        colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.20f, 0.20f, 0.20f, 0.00f);
        colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]     = ImVec4(2.0f / 255.0f, 2.0f / 255.0f, 2.0f / 255.0f, 1.0f);
        colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }
    
    void DrawVecControl(const std::string &label, MiniEngine::Vector3 &values, float resetValue, float columnWidth)
    {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2 {0, 0});

        float  lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.8f, 0.1f, 0.15f, 1.0f});
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.2f, 0.45f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.3f, 0.55f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.2f, 0.45f, 0.2f, 1.0f});
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.1f, 0.25f, 0.8f, 1.0f});
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);
        ImGui::PopID();
    }

    void DrawVecControl(const std::string &label, MiniEngine::Quaternion &values, float resetValue, float columnWidth)
    {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2 {0, 0});

        float  lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.8f, 0.1f, 0.15f, 1.0f});
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.2f, 0.45f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.3f, 0.55f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.2f, 0.45f, 0.2f, 1.0f});
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.1f, 0.25f, 0.8f, 1.0f});
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.5f, 0.25f, 0.5f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.6f, 0.35f, 0.6f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.5f, 0.25f, 0.5f, 1.0f});
        if (ImGui::Button("W", buttonSize))
            values.w = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##W", &values.w, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);
        ImGui::PopID();
    }
}