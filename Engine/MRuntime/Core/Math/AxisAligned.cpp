#include "AxisAligned.hpp"

namespace MiniEngine
{
    AxisAlignedBox::AxisAlignedBox(const Vector3& center, const Vector3& half_extent) { Update(center, half_extent); }

    void AxisAlignedBox::Merge(const Vector3& new_point)
    {
        mMinCorner.MakeFloor(new_point);
        mMaxCorner.MakeCeil(new_point);

        mCenter      = 0.5f * (mMinCorner + mMaxCorner);
        mHalfExtent = mCenter - mMinCorner;
    }

    void AxisAlignedBox::Update(const Vector3& center, const Vector3& half_extent)
    {
        mCenter      = center;
        mHalfExtent = half_extent;
        mMinCorner  = center - half_extent;
        mMaxCorner  = center + half_extent;
    }
}