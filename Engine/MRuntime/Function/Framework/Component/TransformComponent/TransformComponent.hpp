#pragma once

#include "MRuntime/Core/Math/Matrix4.hpp"
#include "MRuntime/Core/Math/Transform.hpp"

#include "MRuntime/Function/Framework/Component/Component.hpp"
#include "MRuntime/Function/Framework/Object/Object.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(TransformComponent)
    CLASS(TransformComponent : public Component, WhiteListFields)
    {
        REFLECTION_BODY(TransformComponent)

    public:
        TransformComponent() = default;

        void PostLoadResource(std::weak_ptr<GObject> parent_object) override;

        Vector3    GetPosition() const { return mTransformBuffer[mCurrentIndex].m_position; }
        Vector3    GetScale() const { return mTransformBuffer[mCurrentIndex].m_scale; }
        Quaternion GetRotation() const { return mTransformBuffer[mCurrentIndex].m_rotation; }

        void SetPosition(const Vector3& new_translation);
        void SetScale(const Vector3& new_scale);
        void SetRotation(const Quaternion& new_rotation);

        const Transform& GetTransformConst() const { return mTransformBuffer[mCurrentIndex]; }
        Transform&       GetTransform() { return mTransformBuffer[mNextIndex]; }

        Matrix4x4 GetMatrix() const { return mTransformBuffer[mCurrentIndex].GetMatrix(); }

        void Tick(float delta_time) override;

    protected:
        META(Enable)
        Transform mTransform;

        Transform mTransformBuffer[2];
        size_t    mCurrentIndex {0};
        size_t    mNextIndex {1};
    };
} // namespace MiniEngine
