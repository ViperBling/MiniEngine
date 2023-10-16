#pragma once


#include "MRuntime/Function/Render/RenderCamera.hpp"
#include "MRuntime/Function/Render/RenderObject.hpp"

#include "MRuntime/Resource/ResourceType/Global/GlobalRendering.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>

namespace MiniEngine
{
    struct SceneIBLResourceDesc
    {
        SkyBoxIrradianceMap mSkyboxIrradianceMap;
        SkyBoxSpecularMap   mSkyboxSpecularMap;
        std::string         mBrdfMap;
    };

    struct SceneColorGradingResourceDesc
    {
        std::string mColorGradientMap;
    };

    struct SceneResourceDesc
    {
        SceneIBLResourceDesc          mIBLResourceDesc;
        SceneColorGradingResourceDesc mColorGradientResourceDesc;
    };

    struct CameraSwapData
    {
        std::optional<float>            mFovX;
        std::optional<RenderCameraType> mCameraType;
        std::optional<Matrix4x4>        mViewMatrix;
    };

    struct GameObjectResourceDesc
    {
        void Add(GameObjectDesc& desc);
        void Pop();

        bool IsEmpty() const;

        GameObjectDesc& GetNextProcessObject();

        std::deque<GameObjectDesc> mGameObjectDesc;
    };

    struct RenderSwapData
    {
        void AddDirtyGameObject(GameObjectDesc&& desc);
        void AddDeleteGameObject(GameObjectDesc&& desc);

        std::optional<SceneResourceDesc>       mSceneResourceDesc;
        std::optional<GameObjectResourceDesc>  mGameObjectResourceDesc;
        std::optional<GameObjectResourceDesc>  mGameObjectToDelete;
        std::optional<CameraSwapData>          mCameraSwapData;
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
        void            ResetSceneResourceSwapData();
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
