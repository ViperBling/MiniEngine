#pragma once

#include "MRuntime/Core/Math/Quaternion.hpp"
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(CameraParameter)
    CLASS(CameraParameter, Fields)
    {
        REFLECTION_BODY(CameraParameter);

    public:
        float mFov {50.f};

        virtual ~CameraParameter() {}
    };

    REFLECTION_TYPE(FirstPersonCameraParameter)
    CLASS(FirstPersonCameraParameter : public CameraParameter, Fields)
    {
        REFLECTION_BODY(FirstPersonCameraParameter);

    public:
        float mVerticalOffset {0.6f};
    };

    REFLECTION_TYPE(ThirdPersonCameraParameter)
    CLASS(ThirdPersonCameraParameter : public CameraParameter, WhiteListFields)
    {
        REFLECTION_BODY(ThirdPersonCameraParameter);

    public:
        META(Enable)
        float mHorizontalOffset {3.f};
        META(Enable)
        float      mVerticalOffset {2.5f};
        Quaternion mCursorPitch;
        Quaternion mCursorYaw;
    };

    REFLECTION_TYPE(FreeCameraParameter)
    CLASS(FreeCameraParameter : public CameraParameter, Fields)
    {
        REFLECTION_BODY(FreeCameraParameter);

    public:
        float mSpeed {1.f};
    };

    REFLECTION_TYPE(CameraComponentRes)
    CLASS(CameraComponentRes, Fields)
    {
        REFLECTION_BODY(CameraComponentRes);

    public:
        Reflection::ReflectionPtr<CameraParameter> mParameter;

        CameraComponentRes() = default;
        CameraComponentRes(const CameraComponentRes& res);

        ~CameraComponentRes();
    };
} // namespace MiniEngine