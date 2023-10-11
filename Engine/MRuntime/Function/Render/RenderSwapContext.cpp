#pragma once


#include "MRuntime/Function/Render/RenderCamera.hpp"
// #include "MRuntime/Function/Render/RenderObject.hpp"

#include "MRuntime/Resource/ResourceType/Global/GlobalRendering.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>

namespace MiniEngine
{
    struct SceneIBLResourceDesc
    {
        SkyBoxIrradianceMap m_skybox_irradiance_map;
        SkyBoxSpecularMap   m_skybox_specular_map;
        std::string         m_brdf_map;
    };

    struct SceneColorGradingResourceDesc
    {
        std::string m_color_grading_map;
    };

    struct SceneResourceDesc
    {
        SceneIBLResourceDesc          m_ibl_resource_desc;
        SceneColorGradingResourceDesc m_color_grading_resource_desc;
    };

    struct CameraSwapData
    {
        std::optional<float>            m_fov_x;
        std::optional<RenderCameraType> m_camera_type;
        std::optional<Matrix4x4>        m_view_matrix;
    };

    struct GameObjectResourceDesc
    {
        std::deque<GameObjectDesc> m_game_object_descs;

        void add(GameObjectDesc& desc);
        void pop();

        bool isEmpty() const;

        GameObjectDesc& getNextProcessObject();
    };

    struct RenderSwapData
    {
        std::optional<SceneResourceDesc>       m_scene_resource_desc;
        std::optional<GameObjectResourceDesc>  m_game_object_resource_desc;
        std::optional<GameObjectResourceDesc>  m_game_object_to_delete;
        std::optional<CameraSwapData>          m_camera_swap_data;

        void addDirtyGameObject(GameObjectDesc&& desc);
        void addDeleteGameObject(GameObjectDesc&& desc);

    };

    enum SwapDataType : uint8_t
    {
        LogicSwapDataType = 0,
        RenderSwapDataType,
        SwapDataTypeCount
    };

    class RenderSwapContext
    {
    public:
        RenderSwapData& GetLogicSwapData();
        RenderSwapData& GetRenderSwapData();
        void            SwapLogicRenderData();
        void            ResetSceneRsourceSwapData();
        void            ResetGameObjectResourceSwapData();
        void            ResetGameObjectToDelete();
        void            ResetCameraSwapData();
    
    private:
        bool isReadyToSwap() const;
        void swap();
    
    private:
        uint8_t        mLogicSwapDataIndex {LogicSwapDataType};
        uint8_t        mRenderSwapDataIndex {RenderSwapDataType};
        RenderSwapData mSwapData[SwapDataTypeCount];    
    };
} // namespace MiniEngine
