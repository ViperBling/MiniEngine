#pragma once

#include "MRuntime/Core/Math/Vector3.hpp"
#include "MRuntime/Function/Render/RenderType.hpp"

#include <vector>

namespace MiniEngine
{
    struct PointLight
    {
        // calculate an appropriate radius for light culling
        // a windowing function in the shader will perform a smooth transition to zero
        // this is not physically based and usually artist controlled
        float CalculateRadius() const
        {
            // radius = where attenuation would lead to an intensity of 1W/m^2
            const float INTENSITY_CUTOFF    = 1.0f;
            const float ATTENTUATION_CUTOFF = 0.05f;
            Vector3     intensity           = mFlux / (4.0f * MATH_PI);
            float       maxIntensity        = Vector3::GetMaxElement(intensity);
            float       attenuation = Math::Max(INTENSITY_CUTOFF, ATTENTUATION_CUTOFF * maxIntensity) / maxIntensity;
            return 1.0f / sqrtf(attenuation);
        }

        Vector3 mPosition;
        // radiant flux in W
        Vector3 mFlux;
    };

    struct AmbientLight
    {
        Vector3 mIrradiance;
    };

    struct PDirectionalLight
    {
        Vector3 mDirection;
        Vector3 mColor;
    };

    struct LightList
    {
        // vertex buffers seem to be aligned to 16 bytes
        struct PointLightVertex
        {
            Vector3 mPosition;
            float   mPadding;
            // radiant intensity in W/sr
            // can be calculated from radiant flux
            Vector3 mIntensity;
            float   mRadius;
        };
    };

    class PointLightList : public LightList
    {
    public:
        void Init() {}
        void Shutdown() {}

        // upload changes to GPU
        void Update() {}

        std::vector<PointLight> mLights;

        std::shared_ptr<BufferData> mBuffer;
    };

} // namespace MiniEngine