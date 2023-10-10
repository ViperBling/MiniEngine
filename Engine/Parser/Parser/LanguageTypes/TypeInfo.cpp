#include "Common/PreCompiled.hpp"

#include "TypeInfo.hpp"

TypeInfo::TypeInfo(const Cursor &cursor, const Namespace &currentNamespace)
    : mMetaData(cursor)
    , mbEnabled(mMetaData.GetFlag(NativeProperty::Enable))
    , mRootCursor(cursor)
    , mNamespace(currentNamespace)
{
}

const MetaInfo &TypeInfo::GetMetaData(void) const
{
    return mMetaData;
}

std::string TypeInfo::GetSourceFile(void) const
{
    return mRootCursor.GetSourceFile();
}

Namespace TypeInfo::GetCurrentNamespace() const
{
    return mNamespace;
}

Cursor &TypeInfo::GetCurosr()
{
    return mRootCursor;
}
