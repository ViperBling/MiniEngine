#include "Common/PreCompiled.hpp"
#include "Class.hpp"

#include "Field.hpp"

Field::Field(const Cursor &cursor, const Namespace &currentNamespace, Class *parent)
    : TypeInfo(cursor, currentNamespace)
    , mbIsConst(cursor.GetType().IsConst())
    , mParent(parent)
    , mName(cursor.GetSpelling())
    , mDisplayName(Utils::GetNameWithoutFirstM(mName))
    , mType(Utils::GetTypeNameWithoutNamespace(cursor.GetType()))
{
    Utils::ReplaceAll(mType, " ", "");
    Utils::ReplaceAll(mType, "MiniEngine::", "");

    auto ret_string = Utils::GetStringWithoutQuot(mMetaData.GetProperty("default"));
    mDefault = ret_string;
}

bool Field::ShouldCompile(void) const
{
    return IsAccessible();
}

bool Field::IsAccessible(void) const
{
    return ((mParent->mMetaData.GetFlag(NativeProperty::Fields) ||
             mParent->mMetaData.GetFlag(NativeProperty::All)) &&
            !mMetaData.GetFlag(NativeProperty::Disable)) ||
            (mParent->mMetaData.GetFlag(NativeProperty::WhiteListFields) &&
             mMetaData.GetFlag(NativeProperty::Enable));
}
