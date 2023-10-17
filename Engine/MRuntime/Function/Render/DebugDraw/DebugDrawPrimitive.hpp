#pragma once

#include "MRuntime/Core/Math/MathHeaders.hpp"
#include "MRuntime/Function/Render/Interface/RHIStruct.hpp"

#include <array>

namespace MiniEngine
{
    static const float kDebugDrawInfinityLifeTime = -2.0f;
    static const float kDebugDrawOneFrame = 0.0f;

    struct DebugDrawVertex
    {
        static std::array<RHIVertexInputBindingDescription, 1> GetBindingDescriptions()
        {
            std::array<RHIVertexInputBindingDescription, 1> bindingDesc;
            bindingDesc[0].binding = 0;
            bindingDesc[0].stride = sizeof(DebugDrawVertex);
            bindingDesc[0].inputRate = RHI_VERTEX_INPUT_RATE_VERTEX;
            return bindingDesc;
        }

        static std::array<RHIVertexInputAttributeDescription, 3> GetAttributeDescriptions()
        {
            std::array<RHIVertexInputAttributeDescription, 3> attributeDesc;
            // position
            attributeDesc[0].binding  = 0;
            attributeDesc[0].location = 0;                        // layout(location)
            attributeDesc[0].format   = RHI_FORMAT_R32G32B32_SFLOAT; // 三个通道都是float
            attributeDesc[0].offset   = offsetof(DebugDrawVertex, mPos);

            // color
            attributeDesc[1].binding  = 0;
            attributeDesc[1].location = 1;
            attributeDesc[1].format   = RHI_FORMAT_R32G32B32A32_SFLOAT; // 四个通道都是float
            attributeDesc[1].offset   = offsetof(DebugDrawVertex, mColor);

            // texcoord
            attributeDesc[2].binding = 0;
            attributeDesc[2].location = 2;
            attributeDesc[2].format = RHI_FORMAT_R32G32_SFLOAT;
            attributeDesc[2].offset = offsetof(DebugDrawVertex, mTexCoord);

            return attributeDesc;
        }
        DebugDrawVertex() { mPos = Vector3(-1.0f, -1.0f, -1.0f); mColor = Vector4(-1.0f, -1.0f, -1.0f, -1.0f); mTexCoord = Vector2(-1.0f, -1.0f); }

        Vector3 mPos;
        Vector4 mColor;
        Vector2 mTexCoord;
    };

    enum class DebugDrawTimeType : uint8_t
    {
        Infinity,
        OneFrame,
        Common
    };

    enum class DebugDrawPrimitiveType : uint8_t
    {
        Point = 0,
        Line,
        Triangle,
        Quad,
        DrawBox,
        Cylinder,
        Sphere,
        Capsule,
        Text,
        Count
    };

    enum FillMode : uint8_t
    {
        Wireframe = 0,
        Solid = 1,
        Count,
    };

    class DebugDrawPrimitive
    {
    public:
        bool IsTimeOut(float deltaTime);
        void SetTime(float inLifeTime);

    public:
        DebugDrawTimeType mTimeType {DebugDrawTimeType::Infinity};
        float             mLifeTime {0.f};
        FillMode          mFillMode {FillMode::Wireframe};
        bool mbNoDepthTest = false;

    private:
        bool mbRendered = false; // for one frame object
    };

    class DebugDrawPoint : public DebugDrawPrimitive
    {
    public:
        DebugDrawVertex mVertex;
        static const DebugDrawPrimitiveType meTypeValue = DebugDrawPrimitiveType::Point;
    };

    class DebugDrawLine : public DebugDrawPrimitive
    {
    public:
        DebugDrawVertex mVertex[2];
        static const DebugDrawPrimitiveType meTypeValue = DebugDrawPrimitiveType::Line;
    };

    class DebugDrawTriangle : public DebugDrawPrimitive
    {
    public:
        DebugDrawVertex                     mVertex[3];
        static const DebugDrawPrimitiveType meTypeValue = DebugDrawPrimitiveType::Triangle;
    };

    class DebugDrawQuad : public DebugDrawPrimitive
    {
    public:
        DebugDrawVertex mVertex[4];

        static const DebugDrawPrimitiveType meTypeValue = DebugDrawPrimitiveType::Quad;
    };

    class DebugDrawBox : public DebugDrawPrimitive
    {
    public:
        Vector3 mCenterPoint;
        Vector3 mHalfExtents;
        Vector4 mColor;
        Vector4 mRotation;

        static const DebugDrawPrimitiveType meTypeValue = DebugDrawPrimitiveType::DrawBox;
    };

    class DebugDrawCylinder : public DebugDrawPrimitive
    {
    public:
        Vector3 mCenter;
        Vector4 mRotation;
        float   mRadius{ 0.f };
        float   mHeight{ 0.f };
        Vector4 mColor;

        static const DebugDrawPrimitiveType meTypeValue = DebugDrawPrimitiveType::Cylinder;
    };
    class DebugDrawSphere : public DebugDrawPrimitive
    {
    public:
        Vector3 mCenter;
        float   mRadius{ 0.f };
        Vector4 mColor;

        static const DebugDrawPrimitiveType meTypeValue = DebugDrawPrimitiveType::Sphere;
    };

    class DebugDrawCapsule : public DebugDrawPrimitive
    {
    public:
        Vector3 mCenter;
        Vector4 mRotation;
        Vector3 mScale;
        float   mRadius{ 0.f };
        float   mHeight{ 0.f };
        Vector4 mColor;

        static const DebugDrawPrimitiveType meTypeValue = DebugDrawPrimitiveType::Capsule;
    };

    class DebugDrawText : public DebugDrawPrimitive
    {
    public:
        std::string mContent;
        Vector4     mColor;
        Vector3     mCoordinate;
        int         mSize;
        bool        mbIsScreenText;

        static const DebugDrawPrimitiveType meTypeValue = DebugDrawPrimitiveType::Text;
    };
} // namespace MiniEngine
