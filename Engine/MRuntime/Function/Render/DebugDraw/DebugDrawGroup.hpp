#pragma once

#include "Function/Render/DebugDraw/DebugDrawPrimitive.hpp"

namespace MiniEngine
{
    class DebugDrawGroup
    {
    public:
        DebugDrawGroup();
        void WriteTriangleData(std::vector<DebugDrawVertex>& vertices, bool bDepthTest);

    private:
        std::list<DebugDrawTriangle> mTriangles;
        void addTriangle(
            const Vector2& point0,
            const Vector2& point1,
            const Vector2& point2,
            const Vector3& color0,
            const Vector3& color1,
            const Vector3& color2
        );
        size_t getTriangleCount(bool bDepthTest) const;
    };
} // namespace MiniEngine
