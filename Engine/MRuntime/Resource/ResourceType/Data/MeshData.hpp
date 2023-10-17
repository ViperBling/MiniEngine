#pragma once

#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

#include <string>
#include <vector>

namespace MiniEngine
{
    REFLECTION_TYPE(VertexData)
    CLASS(VertexData, Fields)
    {
        REFLECTION_BODY(VertexData);

    public:
        float px;
        float py;
        float pz;
        float nx;
        float ny;
        float nz;
        float tx;
        float ty;
        float tz;
        float u;
        float v;
    };

    REFLECTION_TYPE(SkeletonBinding)
    CLASS(SkeletonBinding, Fields)
    {
        REFLECTION_BODY(SkeletonBinding);

    public:
        int   index0;
        int   index1;
        int   index2;
        int   index3;
        float weight0;
        float weight1;
        float weight2;
        float weight3;
    };

    REFLECTION_TYPE(MeshData)
    CLASS(MeshData, Fields)
    {
        REFLECTION_BODY(MeshData);

    public:
        // mesh Data
        std::vector<VertexData> mVertexBuffer;
        std::vector<int> mIndexBuffer;
        std::vector<SkeletonBinding> mBind;
    };
}
