#include "DebugDrawGroup.hpp"

namespace MiniEngine
{
    DebugDrawGroup::DebugDrawGroup()
    {
        Vector2 point0(0.0f, -0.5f);
        Vector2 point1(0.5f, 0.5f);
        Vector2 point2(-0.5f, 0.5f);
        Vector3 color0(1.0f, 1.0f, 0.0f);
        Vector3 color1(0.0f, 1.0f, 1.0f);
        Vector3 color2(1.0f, 0.0f, 1.0f);
        addTriangle(point0, point1, point2, color0, color1, color2);
    }

    void DebugDrawGroup::WriteTriangleData(std::vector<DebugDrawVertex> &vertices, bool bDepthTest)
    {
        size_t vertexsCount = getTriangleCount(bDepthTest) * 3;
        vertices.resize(vertexsCount);

        size_t current_index = 0;
        for (DebugDrawTriangle triangle : mTriangles)
        {
            // if (triangle.m_fill_mode == FillMode::_FillMode_solid && triangle.m_no_depth_test == no_depth_test)
            // {
            vertices[current_index++] = triangle.mVertex[0];
            vertices[current_index++] = triangle.mVertex[1];
            vertices[current_index++] = triangle.mVertex[2];
            // }
        }
    }

    void DebugDrawGroup::addTriangle(const Vector2 &point0, const Vector2 &point1, const Vector2 &point2, const Vector3 &color0, const Vector3 &color1, const Vector3 &color2)
    {
        DebugDrawTriangle triangle;
        // triangle.setTime(life_time);
        // triangle.m_fill_mode     = fillmod;
        // triangle.m_no_depth_test = no_depth_test;

        triangle.mVertex[0].mPos   = point0;
        triangle.mVertex[0].mColor = color0;

        triangle.mVertex[1].mPos   = point1;
        triangle.mVertex[1].mColor = color1;

        triangle.mVertex[2].mPos   = point2;
        triangle.mVertex[2].mColor = color2;

        mTriangles.push_back(triangle);
    }

    size_t DebugDrawGroup::getTriangleCount(bool bDepthTest) const
    {
        size_t triangleCount = 0;
        for (const DebugDrawTriangle triangle : mTriangles)
        {
            // if (triangle.m_fill_mode == FillMode::_FillMode_solid && triangle.m_no_depth_test == no_depth_test)
            triangleCount++;
        }
            
        return triangleCount;
    }

}
