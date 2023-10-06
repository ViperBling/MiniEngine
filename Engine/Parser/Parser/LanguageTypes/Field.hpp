#pragma once

#include "TypeInfo.hpp"

class Class;

class Field : public TypeInfo
{
public:
    Field(const Cursor& cursor, const Namespace& currentNamespace, Class* parent = nullptr);

    virtual ~Field(void) {}

    bool ShouldCompile(void) const;
    bool IsAccessible(void) const;

public:
    bool mbIsConst;

    Class* mParent;

    std::string mName;
    std::string mDisplayName;
    std::string mType;

    std::string mDefault;
};