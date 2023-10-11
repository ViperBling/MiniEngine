#pragma once

#include "MRuntime/Core/Math/Matrix4.hpp"
#include "MRuntime/Core/Math/Quaternion.hpp"
#include "MRuntime/Core/Math/Vector3.hpp"
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(Transform)
    CLASS(Transform, Fields)
    {
        REFLECTION_BODY(Transform);

    public:
        Vector3    m_position {Vector3::ZERO};
        Vector3    m_scale {Vector3::UNIT_SCALE};
        Quaternion m_rotation {Quaternion::IDENTITY};

        Transform() = default;
        Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale) :
            m_position {position}, m_scale {scale}, m_rotation {rotation}
        {}

        Matrix4x4 GetMatrix() const
        {
            Matrix4x4 temp;
            temp.MakeTransform(m_position, m_scale, m_rotation);
            return temp;
        }
    };
} // namespace MiniEngine
