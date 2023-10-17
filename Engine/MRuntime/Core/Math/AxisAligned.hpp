#pragma once

#include "Vector3.hpp"
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

#include <limits>

namespace MiniEngine
{
    REFLECTION_TYPE(AxisAlignedBox)
    CLASS(AxisAlignedBox, Fields)
    {
        REFLECTION_BODY(AxisAlignedBox)
    public:
        AxisAlignedBox() {}
        AxisAlignedBox(const Vector3& center, const Vector3& half_extent);

        void Merge(const Vector3& new_point);
        void Update(const Vector3& center, const Vector3& half_extent);

        const Vector3& GetCenter() const { return mCenter; }
        const Vector3& GetHalfExtent() const { return mHalfExtent; }
        const Vector3& GetMinCorner() const { return mMinCorner; }
        const Vector3& GetMaxCorner() const { return mMaxCorner; }

    private:
        Vector3 mCenter {Vector3::ZERO};
        Vector3 mHalfExtent {Vector3::ZERO};

        Vector3 mMinCorner {
            std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        Vector3 mMaxCorner {
            -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
    };
}