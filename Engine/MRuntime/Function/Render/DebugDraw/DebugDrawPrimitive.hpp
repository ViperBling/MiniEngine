#pragma once

#include <array>

#include "Core/Math/Vector2.hpp"
#include "Core/Math/Vector3.hpp"
#include "Function/Render/Interface/RHIStruct.hpp"

namespace MiniEngine
{
    struct DebugDrawVertex
    {
        Vector2 mPos;
        Vector3 mColor;

        static std::array<RHIVertexInputBindingDescription, 1> GetBindingDescriptions()
        {
            std::array<RHIVertexInputBindingDescription, 1> bindingDesc;
            bindingDesc[0].binding = 0;
            bindingDesc[0].stride = sizeof(DebugDrawVertex);
            bindingDesc[0].inputRate = RHI_VERTEX_INPUT_RATE_VERTEX;
            return bindingDesc;
        }

        static std::array<RHIVertexInputAttributeDescription, 2> GetAttributeDescriptions()
        {
            std::array<RHIVertexInputAttributeDescription, 2> attributeDesc;
            // position
            attributeDesc[0].binding  = 0;
            attributeDesc[0].location = 0;                        // layout(location)
            attributeDesc[0].format   = RHI_FORMAT_R32G32_SFLOAT; // 三个通道都是float
            attributeDesc[0].offset   = offsetof(DebugDrawVertex, mPos);

            // color
            attributeDesc[1].binding  = 0;
            attributeDesc[1].location = 1;
            attributeDesc[1].format   = RHI_FORMAT_R32G32B32_SFLOAT; // 四个通道都是float
            attributeDesc[1].offset   = offsetof(DebugDrawVertex, mColor);

            return attributeDesc;
        }
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

    class DebugDrawPrimitive
    {
    public:
        // DebugDrawTimeType m_time_type {DebugDrawTimeType::infinity};
        // float             m_life_time {0.f};
        // FillMode          m_fill_mode {_FillMode_wireframe};
        bool mbNoDepthTest = false;

        // bool isTimeOut(float delta_time);
        // void setTime(float in_life_time);

    private:
        // bool m_rendered = false; // for one frame object
    };

    class DebugDrawTriangle : public DebugDrawPrimitive
    {
    public:
        DebugDrawVertex                     mVertex[3];
        static const DebugDrawPrimitiveType meTypeValue = DebugDrawPrimitiveType::Triangle;
    };
} // namespace MiniEngine
