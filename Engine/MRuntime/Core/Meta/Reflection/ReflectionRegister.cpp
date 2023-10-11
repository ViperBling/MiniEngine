#include <cassert>
#include "MRuntime/Core/Meta/Reflection/ReflectionRegister.hpp"

#include "MRuntime/Core/Meta/Json.hpp"
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"
#include "MRuntime/Core/Meta/Serializer/Serializer.hpp"

#include "Generated/Reflection/all_reflection.h"
#include "Generated/Serializer/all_serializer.ipp"

namespace MiniEngine
{
    namespace Reflection
    {
        void TypeMetaRegister::MetaUnregister() { TypeMetaRegisterInterface::UnregisterAll(); }
    } // namespace Reflection
} // namespace MiniEngine
