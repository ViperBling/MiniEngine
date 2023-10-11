#pragma once
#include "MRuntime/Core/Math/Transform.hpp"
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(SubMeshRes)
    CLASS(SubMeshRes, Fields)
    {
        REFLECTION_BODY(SubMeshRes);

    public:
        std::string mObjectFileRef;
        Transform   mTransform;
        std::string mMaterial;
    };

    REFLECTION_TYPE(MeshComponentRes)
    CLASS(MeshComponentRes, Fields)
    {
        REFLECTION_BODY(MeshComponentRes);

    public:
        std::vector<SubMeshRes> mSubMeshes;
    };
} // namespace MiniEngine