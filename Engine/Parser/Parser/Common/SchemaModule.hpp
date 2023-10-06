#pragma once
#include "PreCompiled.hpp"

class Class;
class Global;
class Function;
class Enum;

struct SchemaMoudle
{
    std::string name;

    std::vector<std::shared_ptr<Class>> classes;
};