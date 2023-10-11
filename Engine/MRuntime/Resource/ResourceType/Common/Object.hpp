#pragma once

#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

#include <string>
#include <vector>

namespace MiniEngine
{
    class Component;

    REFLECTION_TYPE(ComponentDefinitionRes)
    CLASS(ComponentDefinitionRes, Fields)
    {
        REFLECTION_BODY(ComponentDefinitionRes);

    public:
        std::string mTypeName;
        std::string mComponent;
    };

    REFLECTION_TYPE(ObjectDefinitionRes)
    CLASS(ObjectDefinitionRes, Fields)
    {
        REFLECTION_BODY(ObjectDefinitionRes);
    public:
        std::vector<Reflection::ReflectionPtr<Component>> mComponents;
    };

    REFLECTION_TYPE(ObjectInstanceRes)
    CLASS(ObjectInstanceRes, Fields)
    {
        REFLECTION_BODY(ObjectInstanceRes);

    public:
        std::string mName;
        std::string mDefinition;

        std::vector<Reflection::ReflectionPtr<Component>> mInstancedComponents;
    };
} // namespace MiniEngine
