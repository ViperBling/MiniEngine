#pragma once

#include "MRuntime/Function/Render/RenderEntity.hpp"
#include "MRuntime/Function/Render/RenderType.hpp"

namespace MiniEngine
{
    class EditorTranslationAxis : public RenderEntity
    {
    public:
        EditorTranslationAxis();
        RenderMeshData mMeshData;
    };

    class EditorRotationAxis : public RenderEntity
    {
    public:
        EditorRotationAxis();
        RenderMeshData mMeshData;
    };

    class EditorScaleAxis : public RenderEntity
    {
    public:
        EditorScaleAxis();
        RenderMeshData mMeshData;
    };
} // namespace MiniEngine
