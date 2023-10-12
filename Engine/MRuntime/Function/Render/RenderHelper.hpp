#pragma once

#include "MRuntime/Core/Math/Vector3.hpp"
#include "MRuntime/Core/Math/Vector4.hpp"

namespace MiniEngine
{
    class RenderScene;
    class RenderCamera;
    class Matrix4x4;

    static inline uint32_t RoundUp(uint32_t value, uint32_t alignment)
    {
        uint32_t temp = value + alignment - static_cast<uint32_t>(1);
        return (temp - temp % alignment);
    }

    // TODO: support cluster lighting
    struct ClusterFrustum
    {
        // we don't consider the near and far plane currently
        Vector4 mPlaneRight;
        Vector4 mPlaneLeft;
        Vector4 mPlaneTop;
        Vector4 mPlaneBottom;
        Vector4 mPlaneNear;
        Vector4 mPlaneFar;
    };

    struct BoundingBox
    {
        Vector3 mMinBound {std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max()};
        Vector3 mMaxBound {std::numeric_limits<float>::min(),
                           std::numeric_limits<float>::min(),
                           std::numeric_limits<float>::min()};

        BoundingBox() {}

        BoundingBox(const Vector3& minv, const Vector3& maxv)
        {
            mMinBound = minv;
            mMaxBound = maxv;
        }

        void Merge(const BoundingBox& rhs)
        {
            mMinBound.MakeFloor(rhs.mMinBound);
            mMaxBound.MakeCeil(rhs.mMaxBound);
        }

        void Merge(const Vector3& point)
        {
            mMinBound.MakeFloor(point);
            mMaxBound.MakeCeil(point);
        }
    };

    struct BoundingSphere
    {
        Vector3   mCenter;
        float     mRadius;
    };

    struct FrustumPoints
    {
        Vector3 mFrustumPoints;
    };

    ClusterFrustum CreateClusterFrustumFromMatrix(
        Matrix4x4 mat,
        float     x_left,
        float     x_right,
        float     y_top,
        float     y_bottom,
        float     z_near,
        float     z_far);

    bool TiledFrustumIntersectBox(ClusterFrustum const& f, BoundingBox const& b);

    BoundingBox BoundingBoxTransform(BoundingBox const& b, Matrix4x4 const& m);

    bool BoxIntersectsWithSphere(BoundingBox const& b, BoundingSphere const& s);

    Matrix4x4 CalculateDirectionalLightCamera(RenderScene& scene, RenderCamera& camera);
}