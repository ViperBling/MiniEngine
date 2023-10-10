#pragma once

#include "TypeInfo.hpp"
#include "Field.hpp"

struct BaseClass
{
    BaseClass(const Cursor& cursor);

    std::string mName;
};

class Class : public TypeInfo
{
    // to access m_qualifiedName
    friend class Field;
    friend class MetaParser;

public:
    Class(const Cursor& cursor, const Namespace& currentNamespace);

    virtual bool ShouldCompile(void) const;
    bool ShouldCompileFields(void) const;

    template<typename T>
    using SharedPtrVector = std::vector<std::shared_ptr<T>>;

    std::string GetClassName(void);
    bool IsAccessible(void) const;

public:
    SharedPtrVector<BaseClass> mBaseClasses;
    std::string mName;
    std::string mQualifiedName;
    SharedPtrVector<Field> mFields;
    std::string mDisplayName;
};
