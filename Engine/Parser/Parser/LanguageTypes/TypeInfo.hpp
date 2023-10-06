#pragma once

#include "Common/Namespace.hpp"

#include "Cursor/Cursor.hpp"

#include "Meta/MetaInfo.hpp"
#include "Parser/Parser.hpp"

class TypeInfo
{
public:
    TypeInfo(const Cursor& cursor, const Namespace& currentNamespace);
    virtual ~TypeInfo(void) {}

    const MetaInfo& GetMetaData(void) const;
    std::string GetSourceFile(void) const;
    Namespace GetCurrentNamespace() const;
    Cursor& GetCurosr();

protected:
    MetaInfo mMetaData;
    bool mbEnabled;
    std::string mAliasCN;
    Namespace mNamespace;

private:
    // cursor that represents the root of this language type
    Cursor mRootCursor;
};