#pragma once
#include "MRuntime/Core/Meta/Json.hpp"
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

#include <cassert>

namespace MiniEngine
{
    template<typename...>
    inline constexpr bool always_false = false;

    class Serializer
    {
    public:
        template<typename T>
        static Json WritePointer(T* instance)
        {
            return Json::object {{"$typeName", Json {"*"}}, {"$context", Serializer::Write(*instance)}};
        }

        template<typename T>
        static T*& ReadPointer(const Json& json_context, T*& instance)
        {
            assert(instance == nullptr);
            std::string type_name = json_context["$typeName"].string_value();
            assert(!type_name.empty());
            if ('*' == type_name[0])
            {
                instance = new T;
                Read(json_context["$context"], *instance);
            }
            else
            {
                instance = static_cast<T*>(
                    Reflection::TypeMeta::NewFromNameAndJson(type_name, json_context["$context"]).m_instance);
            }
            return instance;
        }

        template<typename T>
        static Json Write(const Reflection::ReflectionPtr<T>& instance)
        {
            T*          instance_ptr = static_cast<T*>(instance.operator->());
            std::string type_name    = instance.GetTypeName();
            return Json::object {{"$typeName", Json(type_name)},
                                  {"$context", Reflection::TypeMeta::WriteByName(type_name, instance_ptr)}};
        }

        template<typename T>
        static T*& Read(const Json& json_context, Reflection::ReflectionPtr<T>& instance)
        {
            std::string type_name = json_context["$typeName"].string_value();
            instance.SetTypeName(type_name);
            return ReadPointer(json_context, instance.GetPtrReference());
        }

        template<typename T>
        static Json Write(const T& instance)
        {

            if constexpr (std::is_pointer<T>::value)
            {
                return WritePointer((T)instance);
            }
            else
            {
                static_assert(always_false<T>, "Serializer::Write<T> has not been implemented yet!");
                return Json();
            }
        }

        template<typename T>
        static T& Read(const Json& json_context, T& instance)
        {
            if constexpr (std::is_pointer<T>::value)
            {
                return ReadPointer(json_context, instance);
            }
            else
            {
                static_assert(always_false<T>, "Serializer::Read<T> has not been implemented yet!");
                return instance;
            }
        }
    };

    // implementation of base types
    template<>
    Json Serializer::Write(const char& instance);
    template<>
    char& Serializer::Read(const Json& json_context, char& instance);

    template<>
    Json Serializer::Write(const int& instance);
    template<>
    int& Serializer::Read(const Json& json_context, int& instance);

    template<>
    Json Serializer::Write(const unsigned int& instance);
    template<>
    unsigned int& Serializer::Read(const Json& json_context, unsigned int& instance);

    template<>
    Json Serializer::Write(const float& instance);
    template<>
    float& Serializer::Read(const Json& json_context, float& instance);

    template<>
    Json Serializer::Write(const double& instance);
    template<>
    double& Serializer::Read(const Json& json_context, double& instance);

    template<>
    Json Serializer::Write(const bool& instance);
    template<>
    bool& Serializer::Read(const Json& json_context, bool& instance);

    template<>
    Json Serializer::Write(const std::string& instance);
    template<>
    std::string& Serializer::Read(const Json& json_context, std::string& instance);
} // namespace MiniEngine
