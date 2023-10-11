#include "Camera.hpp"
#include "MRuntime/Core/Base/Marco.hpp"

namespace MiniEngine
{
    CameraComponentRes::CameraComponentRes(const CameraComponentRes& res)
    {
        const std::string& camera_type_name = res.mParameter.GetTypeName();
        if (camera_type_name == "FirstPersonCameraParameter")
        {
            mParameter = ME_REFLECTION_NEW(FirstPersonCameraParameter);
            ME_REFLECTION_DEEP_COPY(FirstPersonCameraParameter, mParameter, res.mParameter);
        }
        else if (camera_type_name == "ThirdPersonCameraParameter")
        {
            mParameter = ME_REFLECTION_NEW(ThirdPersonCameraParameter);
            ME_REFLECTION_DEEP_COPY(ThirdPersonCameraParameter, mParameter, res.mParameter);
        }
        else if (camera_type_name == "FreeCameraParameter")
        {
            mParameter = ME_REFLECTION_NEW(FreeCameraParameter);
            ME_REFLECTION_DEEP_COPY(FreeCameraParameter, mParameter, res.mParameter);
        }
        else
        {
            LOG_ERROR("invalid camera type");
        }
    }

    CameraComponentRes::~CameraComponentRes() { ME_REFLECTION_DELETE(mParameter); }
} // namespace MiniEngine