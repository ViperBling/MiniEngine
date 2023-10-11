#pragma once

#include "MRuntime/Core/Math/AxisAligned.hpp"
#include "MRuntime/Core/Math/Matrix4.hpp"

#include <cstdint>
#include <vector>

namespace MiniEngine
{
    class RenderEntity
    {
    public:
        uint32_t  mInstanceID {0};
        Matrix4x4 mModelMatrix {Matrix4x4::IDENTITY};

        // mesh
        size_t                 mMeshAssetID {0};
        bool                   mbEnableVertexBlending {false};
        std::vector<Matrix4x4> mJointMatrices;
        AxisAlignedBox         mBoundingBox;

        // material
        size_t  mMaterialAssetID {0};
        bool    mBlend {false};
        bool    mDoubleSided {false};
        Vector4 mBaseColorFactor {1.0f, 1.0f, 1.0f, 1.0f};
        float   mMetalicFactor {1.0f};
        float   mRoughnessFactor {1.0f};
        float   mNormalScale {1.0f};
        float   mOcclusionStrength {1.0f};
        Vector3 mEmissiveFactor {0.0f, 0.0f, 0.0f};
    };
} // namespace MiniEngine
