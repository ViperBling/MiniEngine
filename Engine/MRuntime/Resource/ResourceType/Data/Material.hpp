#pragma once

#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(MaterialRes)
    CLASS(MaterialRes, Fields)
    {
        REFLECTION_BODY(MaterialRes);

    public:
        std::string mBaseColorTextureFile;
        std::string mMetallicRoughnessTextureFile;
        std::string mNormalTextureFile;
        std::string mOcclusionTextureFile;
        std::string mEmissiveTextureFile;
    };
} // namespace MiniEngine