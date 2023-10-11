#pragma once

#include "MRuntime/Core/Math/Matrix4.hpp"
#include "MRuntime/Function/Framework/Object/ObjectIDAllocator.hpp"

#include <string>
#include <vector>

namespace MiniEngine
{
    REFLECTION_TYPE(GameObjectMeshDesc)
    STRUCT(GameObjectMeshDesc, Fields)
    {
        REFLECTION_BODY(GameObjectMeshDesc)
        std::string mMeshFile;
    };

    REFLECTION_TYPE(SkeletonBindingDesc)
    STRUCT(SkeletonBindingDesc, Fields)
    {
        REFLECTION_BODY(SkeletonBindingDesc)
        std::string mSkeletonBindingFile;
    };

    REFLECTION_TYPE(SkeletonAnimationResultTransform)
    STRUCT(SkeletonAnimationResultTransform, WhiteListFields)
    {
        REFLECTION_BODY(SkeletonAnimationResultTransform)
        Matrix4x4 mMatrix;
    };

    REFLECTION_TYPE(SkeletonAnimationResult)
    STRUCT(SkeletonAnimationResult, Fields)
    {
        REFLECTION_BODY(SkeletonAnimationResult)
        std::vector<SkeletonAnimationResultTransform> mTransform;
    };

    REFLECTION_TYPE(GameObjectMaterialDesc)
    STRUCT(GameObjectMaterialDesc, Fields)
    {
        REFLECTION_BODY(GameObjectMaterialDesc)
        std::string mBaseColorTextureFile;
        std::string mMetallicRoughnessTextureFile;
        std::string mNormalTextureFile;
        std::string mOcclusionTextureFile;
        std::string mEmissiveTextureFile;
        bool        mbWithTexture {false};
    };

    REFLECTION_TYPE(GameObjectTransformDesc)
    STRUCT(GameObjectTransformDesc, WhiteListFields)
    {
        REFLECTION_BODY(GameObjectTransformDesc)
        Matrix4x4 mTransformMatrix {Matrix4x4::IDENTITY};
    };

    REFLECTION_TYPE(GameObjectPartDesc)
    STRUCT(GameObjectPartDesc, Fields)
    {
        REFLECTION_BODY(GameObjectPartDesc)
        GameObjectMeshDesc      mMeshDesc;
        GameObjectMaterialDesc  mMaterialDesc;
        GameObjectTransformDesc mTransformDesc;
        bool                    mbWithAnimation {false};
        SkeletonBindingDesc     mSkeletonBindingDesc;
        SkeletonAnimationResult mSkeletonAnimationResult;
    };

    constexpr size_t kInvalidPartID = std::numeric_limits<size_t>::max();

    struct GameObjectPartId
    {
        bool   operator==(const GameObjectPartId& rhs) const { return mGOID == rhs.mGOID && mPartID == rhs.mPartID; }
        size_t GetHashValue() const { return mGOID ^ (mPartID << 1); }
        bool   IsValid() const { return mGOID != kInvalidGObjectID && mPartID != kInvalidPartID; }

        GObjectID mGOID {kInvalidGObjectID};
        size_t    mPartID {kInvalidPartID};
    };

    class GameObjectDesc
    {
    public:
        GameObjectDesc() : mGOID(0) {}
        GameObjectDesc(size_t go_id, const std::vector<GameObjectPartDesc>& parts) :
            mGOID(go_id), mObjectParts(parts)
        {}

        GObjectID                              GetId() const { return mGOID; }
        const std::vector<GameObjectPartDesc>& GetObjectParts() const { return mObjectParts; }

    private:
        GObjectID                       mGOID {kInvalidGObjectID};
        std::vector<GameObjectPartDesc> mObjectParts;
    };
} // namespace MiniEngine

template<>
struct std::hash<MiniEngine::GameObjectPartId>
{
    size_t operator()(const MiniEngine::GameObjectPartId& rhs) const noexcept { return rhs.GetHashValue(); }
};
