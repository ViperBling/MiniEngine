#include "RenderSwapContext.hpp"

#include <utility>

namespace MiniEngine
{
    void GameObjectResourceDesc::Add(GameObjectDesc& desc) { mGameObjectDesc.push_back(desc); }

    bool GameObjectResourceDesc::IsEmpty() const { return mGameObjectDesc.empty(); }

    GameObjectDesc& GameObjectResourceDesc::GetNextProcessObject() { return mGameObjectDesc.front(); }

    void GameObjectResourceDesc::Pop() { mGameObjectDesc.pop_front(); }

    RenderSwapData& RenderSwapContext::GetLogicSwapData() { return mSwapData[mLogicSwapDataIndex]; }

    RenderSwapData& RenderSwapContext::GetRenderSwapData() { return mSwapData[mRenderSwapDataIndex]; }

    void RenderSwapContext::SwapLogicRenderData()
    {
        if (isReadyToSwap())
        {
            swap();
        }
    }

    bool RenderSwapContext::isReadyToSwap() const
    {
        // return false;
        return !(mSwapData[mRenderSwapDataIndex].mSceneResourceDesc.has_value() ||
                 mSwapData[mRenderSwapDataIndex].mGameObjectResourceDesc.has_value() ||
                 mSwapData[mRenderSwapDataIndex].mGameObjectToDelete.has_value() ||
                 mSwapData[mRenderSwapDataIndex].mCameraSwapData.has_value());
    }

    void RenderSwapContext::ResetSceneResourceSwapData()
    {
        mSwapData[mRenderSwapDataIndex].mSceneResourceDesc.reset();
    }

    void RenderSwapContext::ResetGameObjectResourceSwapData()
    {
        mSwapData[mRenderSwapDataIndex].mGameObjectResourceDesc.reset();
    }

    void RenderSwapContext::ResetGameObjectToDelete()
    {
        mSwapData[mRenderSwapDataIndex].mGameObjectToDelete.reset();
    }

    void RenderSwapContext::ResetCameraSwapData() 
    { 
        mSwapData[mRenderSwapDataIndex].mCameraSwapData.reset();
    }

    void RenderSwapContext::swap()
    {
        ResetSceneResourceSwapData();
        ResetGameObjectResourceSwapData();
        ResetGameObjectToDelete();
        ResetCameraSwapData();
        std::swap(mLogicSwapDataIndex, mRenderSwapDataIndex);
    }

    void RenderSwapData::AddDirtyGameObject(GameObjectDesc&& desc)
    {
        if (mGameObjectResourceDesc.has_value())
        {
            mGameObjectResourceDesc->Add(desc);
        }
        else
        {
            GameObjectResourceDesc go_descs;
            go_descs.Add(desc);
            mGameObjectResourceDesc = go_descs;
        }
    }

    void RenderSwapData::AddDeleteGameObject(GameObjectDesc&& desc)
    {
        if (mGameObjectToDelete.has_value())
        {
            mGameObjectToDelete->Add(desc);
        }
        else
        {
            GameObjectResourceDesc go_descs;
            go_descs.Add(desc);
            mGameObjectToDelete = go_descs;
        }
    }
}