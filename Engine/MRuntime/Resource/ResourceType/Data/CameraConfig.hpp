#pragma once

#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

#include "MRuntime/Core/Math/Vector2.hpp"
#include "MRuntime/Core/Math/Vector3.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(CameraPose)
    CLASS(CameraPose, Fields)
    {
        REFLECTION_BODY(CameraPose);

    public:
        Vector3 mPosition;
        Vector3 mRotation;
        Vector3 mTarget;
        Vector3 mUp;
    };

    REFLECTION_TYPE(CameraConfig)
    CLASS(CameraConfig, Fields)
    {
        REFLECTION_BODY(CameraConfig);

    public:
        CameraPose mPose;
        Vector2    mAspect;
        float      mZFar;
        float      mZNear;
    };
} // namespace MiniEngine