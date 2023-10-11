#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace MiniEngine
{
    enum class PrimitiveType
    {
        Point,
        Line,
        Triangle,
        Quad
    };

    struct RawVertexBuffer
    {
        uint32_t           vertexCount {0};
        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<float> tangents;
        std::vector<float> uvs;
    };

    struct RawIndexBuffer
    {
        PrimitiveType         primitiveType {PrimitiveType::Triangle};
        uint32_t              primitiveCount {0};
        std::vector<uint32_t> indices;
    };

    struct MaterialTexture
    {
        std::string baseColor;
        std::string metallicRoughness;
        std::string normal;
    };

    struct StaticMeshData
    {
        RawVertexBuffer vertexBuffer;
        RawIndexBuffer  indexBuffer;
        MaterialTexture materialTexture;
    };
} // namespace MiniEngine
