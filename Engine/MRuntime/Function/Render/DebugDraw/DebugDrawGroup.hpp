#pragma once

#include "DebugDrawPrimitive.hpp"
#include "DebugDrawFont.hpp"

#include <mutex>
#include <list>

namespace MiniEngine
{
    class DebugDrawGroup
    {
    public:
        virtual ~DebugDrawGroup();
        void Initialize();
        void Clear();
        void ClearData();
        void SetName(const std::string& name);
        const std::string& GetName() const;
        
        void AddPoint(
            const Vector3& position, 
            const Vector4& color, 
            const float    life_time = kDebugDrawOneFrame, 
            const bool     no_depth_test = true
        );
        void AddLine(
            const Vector3& point0,
            const Vector3& point1,
            const Vector4& color0,
            const Vector4& color1,
            const float    life_time = kDebugDrawOneFrame,
            const bool     no_depth_test = true
        );
        void AddTriangle(
            const Vector3& point0,
            const Vector3& point1,
            const Vector3& point2,
            const Vector4& color0,
            const Vector4& color1,
            const Vector4& color2,
            const float    life_time = kDebugDrawOneFrame,
            const bool     no_depth_test = true,
            const FillMode fillmod = FillMode::Wireframe
        );
        void AddQuad(
            const Vector3& point0,
            const Vector3& point1,
            const Vector3& point2,
            const Vector3& point3,
            const Vector4& color0,
            const Vector4& color1,
            const Vector4& color2,
            const Vector4& color3,
            const float    life_time = kDebugDrawOneFrame,
            const bool     no_depth_test = true,
            const FillMode fillmode = FillMode::Wireframe
        );
        void AddBox(
            const Vector3& center_point,
            const Vector3& half_extends,
            const Vector4& rotate,
            const Vector4& color,
            const float    life_time = kDebugDrawOneFrame,
            const bool     no_depth_test = true
        );
        void AddSphere(
            const Vector3& center, 
            const float    radius,
            const Vector4& color, 
            const float    life_time, 
            const bool     no_depth_test = true
        );
        void AddCylinder(
            const Vector3& center,
            const float    radius,
            const float    height,
            const Vector4& rotate,
            const Vector4& color,
            const float    life_time = kDebugDrawOneFrame, 
            const bool     no_depth_test = true
        );
        void AddCapsule(
            const Vector3& center,
            const Vector4& rotation,
            const Vector3& scale,
            const float    radius,
            const float    height,
            const Vector4& color,
            const float    life_time = kDebugDrawOneFrame,
            const bool     no_depth_test = true
        );
        void AddText(
            const std::string& content,
            const Vector4&     color,
            const Vector3&     coordinate,
            const int          size,
            const bool         is_screen_text,
            const float        life_time = kDebugDrawOneFrame
        );

        void RemoveDeadPrimitives(float delta_time);
        void MergeFrom(DebugDrawGroup* group);

        size_t GetPointCount(bool no_depth_test) const;
        size_t GetLineCount(bool no_depth_test) const;
        size_t GetTriangleCount(bool no_depth_test) const;
        size_t GetUniformDynamicDataCount() const;
        size_t GetSphereCount(bool no_depth_test) const;
        size_t GetCylinderCount(bool no_depth_test) const;
        size_t GetCapsuleCount(bool no_depth_test) const;
        size_t GetTextCharacterCount() const;

        void WritePointData(std::vector<DebugDrawVertex>& vertexs, bool no_depth_test);
        void WriteLineData(std::vector<DebugDrawVertex>& vertexs, bool no_depth_test);
        void WriteTriangleData(std::vector<DebugDrawVertex>& vertexs, bool no_depth_test);
        void WriteUniformDynamicDataToCache(std::vector<std::pair<Matrix4x4, Vector4> >& datas);
        void WriteTextData(std::vector<DebugDrawVertex>& vertexs, DebugDrawFont* font, Matrix4x4 m_proj_view_matrix);
        
    private:
        std::mutex mMutex;
        std::string mName;
        
        std::list<DebugDrawTriangle> mTriangles;
        std::list<DebugDrawPoint>    mPoints;
        std::list<DebugDrawLine>     mLines;
        std::list<DebugDrawQuad>     mQuads;
        std::list<DebugDrawBox>      mBoxes;
        std::list<DebugDrawCylinder> mCylinders;
        std::list<DebugDrawSphere>   mSpheres;
        std::list<DebugDrawCapsule>  mCapsules;
        std::list<DebugDrawText>     mTexts;
    };
} // namespace MiniEngine
