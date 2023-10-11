#pragma once

#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

#include <string>
#include <vector>

namespace MiniEngine
{
    REFLECTION_TYPE(WorldRes)
    CLASS(WorldRes, Fields)
    {
        REFLECTION_BODY(WorldRes);

    public:
        // world name
        std::string              mName;

        // all scene urls for this world
        std::vector<std::string> mSceneURLs;

        // the default scene for this world, which should be first loading scene
        std::string mDefaultSceneURL;
    };
} // namespace MiniEngine
