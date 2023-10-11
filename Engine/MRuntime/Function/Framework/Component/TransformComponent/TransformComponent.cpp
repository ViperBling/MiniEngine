#include "TransformComponent.hpp"

#include "MRuntime/MEngine.hpp"

namespace MiniEngine
{
    void TransformComponent::PostLoadResource(std::weak_ptr<GObject> parent_gobject)
    {
        mParentObject       = parent_gobject;
        mTransformBuffer[0] = mTransform;
        mTransformBuffer[1] = mTransform;
        mbIsDirty            = true;
    }

    void TransformComponent::SetPosition(const Vector3& new_translation)
    {
        mTransformBuffer[mNextIndex].m_position = new_translation;
        mTransform.m_position                      = new_translation;
        mbIsDirty                                  = true;
    }

    void TransformComponent::SetScale(const Vector3& new_scale)
    {
        mTransformBuffer[mNextIndex].m_scale = new_scale;
        mTransform.m_scale                      = new_scale;
        mbIsDirty                               = true;
        mbIsScaleDirty                         = true;
    }

    void TransformComponent::SetRotation(const Quaternion& new_rotation)
    {
        mTransformBuffer[mNextIndex].m_rotation = new_rotation;
        mTransform.m_rotation                      = new_rotation;
        mbIsDirty                                  = true;
    }

    void TransformComponent::Tick(float delta_time)
    {
        std::swap(mCurrentIndex, mNextIndex);

        if (gbIsEditorMode)
        {
            mTransformBuffer[mNextIndex] = mTransform;
        }
    }
} // namespace MiniEngine
