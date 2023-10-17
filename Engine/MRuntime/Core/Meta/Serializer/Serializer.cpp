#include "Serializer.hpp"

#include <cassert>

namespace MiniEngine
{
    template<>
    Json Serializer::Write(const char& instance)
    {
        return Json(instance);
    }
    template<>
    char& Serializer::Read(const Json& json_context, char& instance)
    {
        assert(json_context.is_number());
        return instance = json_context.number_value();
    }

    template<>
    Json Serializer::Write(const int& instance)
    {
        return Json(instance);
    }
    template<>
    int& Serializer::Read(const Json& json_context, int& instance)
    {
        assert(json_context.is_number());
        return instance = static_cast<int>(json_context.number_value());
    }

    template<>
    Json Serializer::Write(const unsigned int& instance)
    {
        return Json(static_cast<int>(instance));
    }
    template<>
    unsigned int& Serializer::Read(const Json& json_context, unsigned int& instance)
    {
        assert(json_context.is_number());
        return instance = static_cast<unsigned int>(json_context.number_value());
    }

    template<>
    Json Serializer::Write(const float& instance)
    {
        return Json(instance);
    }
    template<>
    float& Serializer::Read(const Json& json_context, float& instance)
    {
        assert(json_context.is_number());
        return instance = static_cast<float>(json_context.number_value());
    }

    template<>
    Json Serializer::Write(const double& instance)
    {
        return Json(instance);
    }
    template<>
    double& Serializer::Read(const Json& json_context, double& instance)
    {
        assert(json_context.is_number());
        return instance = static_cast<float>(json_context.number_value());
    }

    template<>
    Json Serializer::Write(const bool& instance)
    {
        return Json(instance);
    }
    template<>
    bool& Serializer::Read(const Json& json_context, bool& instance)
    {
        assert(json_context.is_bool());
        return instance = json_context.bool_value();
    }

    template<>
    Json Serializer::Write(const std::string& instance)
    {
        return Json(instance);
    }
    template<>
    std::string& Serializer::Read(const Json& json_context, std::string& instance)
    {
        assert(json_context.is_string());
        return instance = json_context.string_value();
    }
}