#pragma once

#include "MRuntime/Core/Math/Vector3.hpp"
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(Geometry)
    CLASS(Geometry, Fields)
    {
        REFLECTION_BODY(Geometry);

    public:
        virtual ~Geometry() {}
    };

    REFLECTION_TYPE(Box)
    CLASS(Box : public Geometry, Fields)
    {
        REFLECTION_BODY(Box);

    public:
        ~Box() override {}

        Vector3 mHalfExtents {0.5f, 0.5f, 0.5f};
    };

    REFLECTION_TYPE(Sphere)
    CLASS(Sphere : public Geometry, Fields)
    {
        REFLECTION_BODY(Sphere);

    public:
        ~Sphere() override {}
        float mRadius {0.5f};
    };

    REFLECTION_TYPE(Capsule)
    CLASS(Capsule : public Geometry, Fields)
    {
        REFLECTION_BODY(Capsule);

    public:
        ~Capsule() override {}
        float mRadius {0.3f};
        float mHalfHeight {0.7f};
    };
} // namespace MiniEngine