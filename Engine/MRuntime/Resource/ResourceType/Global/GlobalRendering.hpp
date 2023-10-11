#pragma once

#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

#include "MRuntime/Core/Math/Color.hpp"
#include "MRuntime/Core/Math/Vector3.hpp"

#include "MRuntime/Resource/ResourceType/Data/CameraConfig.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(SkyBoxIrradianceMap)
    CLASS(SkyBoxIrradianceMap, Fields)
    {
        REFLECTION_BODY(SkyBoxIrradianceMap);

    public:
        std::string mNegativeXMap;
        std::string mPositiveXMap;
        std::string mNegativeYMap;
        std::string mPositiveYMap;
        std::string mNegativeZMap;
        std::string mPositiveZMap;
    };

    REFLECTION_TYPE(SkyBoxSpecularMap)
    CLASS(SkyBoxSpecularMap, Fields)
    {
        REFLECTION_BODY(SkyBoxSpecularMap);

    public:
        std::string mNegativeXMap;
        std::string mPositiveXMap;
        std::string mNegativeYMap;
        std::string mPositiveYMap;
        std::string mNegativeZMap;
        std::string mPositiveZMap;
    };

    REFLECTION_TYPE(DirectionalLight)
    CLASS(DirectionalLight, Fields)
    {
        REFLECTION_BODY(DirectionalLight);

    public:
        Vector3 mDirection;
        Color   mColor;
    };

    REFLECTION_TYPE(GlobalRenderingRes)
    CLASS(GlobalRenderingRes, Fields)
    {
        REFLECTION_BODY(GlobalRenderingRes);

    public:
        bool                mbEnableFXAA {false};
        SkyBoxIrradianceMap mSkyboxIrradianceMap;
        SkyBoxSpecularMap   mSkyboxSpecularMap;
        std::string         mBrdfMap;
        std::string         mColorGradientMap;

        Color            mSkyColor;
        Color            mAmbientLight;
        CameraConfig     mCameraConfig;
        DirectionalLight mDirectionalLight;
    };
} // namespace MiniEngine