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
        const auto& asset_folder            = g_runtime_global_context.m_config_manager->getAssetFolder();
        m_editor_ui_creator["TreeNodePush"] = [this](const std::string& name, void* value_ptr) -> void {
            static ImGuiTableFlags flags      = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings;
            bool                   node_state = false;
            g_node_depth++;
            if (g_node_depth > 0)
            {
                if (g_editor_node_state_array[g_node_depth - 1].second)
                {
                    node_state = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
                }
                else
                {
                    g_editor_node_state_array.emplace_back(std::pair(name.c_str(), node_state));
                    return;
                }
            }
            else
            {
                node_state = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
            }
            g_editor_node_state_array.emplace_back(std::pair(name.c_str(), node_state));
        };
        m_editor_ui_creator["TreeNodePop"] = [this](const std::string& name, void* value_ptr) -> void {
            if (g_editor_node_state_array[g_node_depth].second)
            {
                ImGui::TreePop();
            }
            g_editor_node_state_array.pop_back();
            g_node_depth--;
        };
        m_editor_ui_creator["Transform"] = [this](const std::string& name, void* value_ptr) -> void {
            if (g_editor_node_state_array[g_node_depth].second)
            {
                Transform* trans_ptr = static_cast<Transform*>(value_ptr);

                Vector3 degrees_val;

                degrees_val.x = trans_ptr->m_rotation.getPitch(false).valueDegrees();
                degrees_val.y = trans_ptr->m_rotation.getRoll(false).valueDegrees();
                degrees_val.z = trans_ptr->m_rotation.getYaw(false).valueDegrees();

                DrawVecControl("Position", trans_ptr->m_position);
                DrawVecControl("Rotation", degrees_val);
                DrawVecControl("Scale", trans_ptr->m_scale);

                trans_ptr->m_rotation.w = Math::cos(Math::degreesToRadians(degrees_val.x / 2)) *
                                              Math::cos(Math::degreesToRadians(degrees_val.y / 2)) *
                                              Math::cos(Math::degreesToRadians(degrees_val.z / 2)) +
                                          Math::sin(Math::degreesToRadians(degrees_val.x / 2)) *
                                              Math::sin(Math::degreesToRadians(degrees_val.y / 2)) *
                                              Math::sin(Math::degreesToRadians(degrees_val.z / 2));
                trans_ptr->m_rotation.x = Math::sin(Math::degreesToRadians(degrees_val.x / 2)) *
                                              Math::cos(Math::degreesToRadians(degrees_val.y / 2)) *
                                              Math::cos(Math::degreesToRadians(degrees_val.z / 2)) -
                                          Math::cos(Math::degreesToRadians(degrees_val.x / 2)) *
                                              Math::sin(Math::degreesToRadians(degrees_val.y / 2)) *
                                              Math::sin(Math::degreesToRadians(degrees_val.z / 2));
                trans_ptr->m_rotation.y = Math::cos(Math::degreesToRadians(degrees_val.x / 2)) *
                                              Math::sin(Math::degreesToRadians(degrees_val.y / 2)) *
                                              Math::cos(Math::degreesToRadians(degrees_val.z / 2)) +
                                          Math::sin(Math::degreesToRadians(degrees_val.x / 2)) *
                                              Math::cos(Math::degreesToRadians(degrees_val.y / 2)) *
                                              Math::sin(Math::degreesToRadians(degrees_val.z / 2));
                trans_ptr->m_rotation.z = Math::cos(Math::degreesToRadians(degrees_val.x / 2)) *
                                              Math::cos(Math::degreesToRadians(degrees_val.y / 2)) *
                                              Math::sin(Math::degreesToRadians(degrees_val.z / 2)) -
                                          Math::sin(Math::degreesToRadians(degrees_val.x / 2)) *
                                              Math::sin(Math::degreesToRadians(degrees_val.y / 2)) *
                                              Math::cos(Math::degreesToRadians(degrees_val.z / 2));
                trans_ptr->m_rotation.normalise();

                g_editor_global_context.m_scene_manager->drawSelectedEntityAxis();
            }
        };
        m_editor_ui_creator["bool"] = [this](const std::string& name, void* value_ptr)  -> void {
            if(g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::Checkbox(label.c_str(), static_cast<bool*>(value_ptr));
            }
            else
            {
                if(g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", name.c_str());
                    ImGui::Checkbox(full_label.c_str(), static_cast<bool*>(value_ptr));
                }
            }
        };
        m_editor_ui_creator["int"] = [this](const std::string& name, void* value_ptr) -> void {
            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::InputInt(label.c_str(), static_cast<int*>(value_ptr));
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    ImGui::InputInt(full_label.c_str(), static_cast<int*>(value_ptr));
                }
            }
        };
        m_editor_ui_creator["float"] = [this](const std::string& name, void* value_ptr) -> void {
            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::InputFloat(label.c_str(), static_cast<float*>(value_ptr));
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    ImGui::InputFloat(full_label.c_str(), static_cast<float*>(value_ptr));
                }
            }
        };
        m_editor_ui_creator["Vector3"] = [this](const std::string& name, void* value_ptr) -> void {
            Vector3* vec_ptr = static_cast<Vector3*>(value_ptr);
            float    val[3]  = {vec_ptr->x, vec_ptr->y, vec_ptr->z};
            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::DragFloat3(label.c_str(), val);
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
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
        m_editor_ui_creator["Quaternion"] = [this](const std::string& name, void* value_ptr) -> void {
            Quaternion* qua_ptr = static_cast<Quaternion*>(value_ptr);
            float       val[4]  = {qua_ptr->x, qua_ptr->y, qua_ptr->z, qua_ptr->w};
            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::DragFloat4(label.c_str(), val);
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
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
        m_editor_ui_creator["std::string"] = [this, &asset_folder](const std::string& name, void* value_ptr) -> void {
            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::Text("%s", (*static_cast<std::string*>(value_ptr)).c_str());
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    std::string value_str = *static_cast<std::string*>(value_ptr);
                    if (value_str.find_first_of('/') != std::string::npos)
                    {
                        std::filesystem::path value_path(value_str);
                        if (value_path.is_absolute())
                        {
                            value_path = Path::getRelativePath(asset_folder, value_path);
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

    void MEditorUI::Initialize(WindowUIInitInfo initInfo) 
    {
        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        ASSERT(config_manager);

        // create imgui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // set ui content scale
        float x_scale, y_scale;
        glfwGetWindowContentScale(init_info.window_system->getWindow(), &x_scale, &y_scale);
        float content_scale = fmaxf(1.0f, fmaxf(x_scale, y_scale));
        windowContentScaleUpdate(content_scale);
        glfwSetWindowContentScaleCallback(init_info.window_system->getWindow(), windowContentScaleCallback);

        // load font for imgui
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigDockingAlwaysTabBar         = true;
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.Fonts->AddFontFromFileTTF(
            config_manager->getEditorFontPath().generic_string().data(), content_scale * 16, nullptr, nullptr);
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
        std::string big_icon_path_string   = config_manager->getEditorBigIconPath().generic_string();
        std::string small_icon_path_string = config_manager->getEditorSmallIconPath().generic_string();
        window_icon[0].pixels =
            stbi_load(big_icon_path_string.data(), &window_icon[0].width, &window_icon[0].height, 0, 4);
        window_icon[1].pixels =
            stbi_load(small_icon_path_string.data(), &window_icon[1].width, &window_icon[1].height, 0, 4);
        glfwSetWindowIcon(init_info.window_system->getWindow(), 2, window_icon);
        stbi_image_free(window_icon[0].pixels);
        stbi_image_free(window_icon[1].pixels);

        // initialize imgui vulkan render backend
        init_info.render_system->initializeUIRenderBackend(this);
    }

    void MEditorUI::PreRender()
    {
        showEditorUI();
    }

    void MEditorUI::onFileContentItemClicked(EditorFileNode *node)
    {
        if (node->m_file_type != "object")
            return;

        std::shared_ptr<Level> level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
        if (level == nullptr)
            return;

        const unsigned int new_object_index = ++m_new_object_index_map[node->m_file_name];

        ObjectInstanceRes new_object_instance_res;
        new_object_instance_res.m_name =
            "New_" + Path::getFilePureName(node->m_file_name) + "_" + std::to_string(new_object_index);
        new_object_instance_res.m_definition =
            g_runtime_global_context.m_asset_manager->getFullPath(node->m_file_path).generic_string();

        size_t new_gobject_id = level->createObject(new_object_instance_res);
        if (new_gobject_id != k_invalid_gobject_id)
        {
            g_editor_global_context.m_scene_manager->onGObjectSelected(new_gobject_id);
        }
    }

    void MEditorUI::buildEditorFileAssetsUITree(EditorFileNode *node)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        const bool is_folder = (node->m_child_nodes.size() > 0);
        if (is_folder)
        {
            bool open = ImGui::TreeNodeEx(node->m_file_name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(100.0f);
            ImGui::TextUnformatted(node->m_file_type.c_str());
            if (open)
            {
                for (int child_n = 0; child_n < node->m_child_nodes.size(); child_n++)
                    buildEditorFileAssetsUITree(node->m_child_nodes[child_n].get());
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::TreeNodeEx(node->m_file_name.c_str(),
                              ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                  ImGuiTreeNodeFlags_SpanFullWidth);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                onFileContentItemClicked(node);
            }
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(100.0f);
            ImGui::TextUnformatted(node->m_file_type.c_str());
        }
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

    void EditorUI::initialize(WindowUIInitInfo init_info)
    {
        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        ASSERT(config_manager);

        // create imgui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // set ui content scale
        float x_scale, y_scale;
        glfwGetWindowContentScale(init_info.window_system->getWindow(), &x_scale, &y_scale);
        float content_scale = fmaxf(1.0f, fmaxf(x_scale, y_scale));
        windowContentScaleUpdate(content_scale);
        glfwSetWindowContentScaleCallback(init_info.window_system->getWindow(), windowContentScaleCallback);

        // load font for imgui
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigDockingAlwaysTabBar         = true;
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.Fonts->AddFontFromFileTTF(
            config_manager->getEditorFontPath().generic_string().data(), content_scale * 16, nullptr, nullptr);
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
        std::string big_icon_path_string   = config_manager->getEditorBigIconPath().generic_string();
        std::string small_icon_path_string = config_manager->getEditorSmallIconPath().generic_string();
        window_icon[0].pixels =
            stbi_load(big_icon_path_string.data(), &window_icon[0].width, &window_icon[0].height, 0, 4);
        window_icon[1].pixels =
            stbi_load(small_icon_path_string.data(), &window_icon[1].width, &window_icon[1].height, 0, 4);
        glfwSetWindowIcon(init_info.window_system->getWindow(), 2, window_icon);
        stbi_image_free(window_icon[0].pixels);
        stbi_image_free(window_icon[1].pixels);

        // initialize imgui vulkan render backend
        init_info.render_system->initializeUIRenderBackend(this);
    }

    void MEditorUI::drawAxisToggleButton(const char *stringID, bool checkState, int axisMode)
    {
    }

    void MEditorUI::createClassUI(Reflection::ReflectionInstance &instance)
    {
    }

    void MEditorUI::createLeafNodeUI(Reflection::ReflectionInstance &instance)
    {
    }

    std::string MEditorUI::getLeafUINodeParentLabel()
    {
        return std::string();
    }

    void MEditorUI::showEditorUI()
    {
    }

    void MEditorUI::showEditorMenu(bool *pOpen)
    {
    }

    void MEditorUI::showEditorWorldObjectsWindow(bool *pOpen)
    {
    }

    void MEditorUI::showEditorFileContentWindow(bool *pOpen)
    {
    }

    void MEditorUI::showEditorGameWindow(bool *pOpen)
    {
    }

    void MEditorUI::showEditorDetailWindow(bool *pOpen)
    {
    }

    void MEditorUI::setUIColorStyle()
    {
    }
    
    void DrawVecControl(const std::string &label, MiniEngine::Vector3 &values, float resetValue, float columnWidth)
    {
    }

    void DrawVecControl(const std::string &label, MiniEngine::Quaternion &values, float resetValue, float columnWidth)
    {
    }
}