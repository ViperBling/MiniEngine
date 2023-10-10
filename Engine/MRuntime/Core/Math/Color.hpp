#pragma once

#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"
#include "MRuntime/Core/Math/Vector3.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(Color)
    CLASS(Color, Fields)
    {
        REFLECTION_BODY(Color);

    public:
        float r;
        float g;
        float b;

        Vector3 ToVector3() const { return Vector3(r, g, b); }
    };
} // namespace MiniEngine
