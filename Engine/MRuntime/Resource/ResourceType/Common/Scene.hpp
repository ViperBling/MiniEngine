#pragma once

#include "MRuntime/Core/Math/Vector3.hpp"
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"
#include "MRuntime/Resource/ResourceType/Common/Object.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(SceneRes)
    CLASS(SceneRes, Fields)
    {
        REFLECTION_BODY(SceneRes);

    public:
        std::vector<ObjectInstanceRes> mObjects;
    };
} // namespace MiniEngine
