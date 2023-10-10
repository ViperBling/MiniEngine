#include "Common/PreCompiled.hpp"

#include "Class.hpp"

BaseClass::BaseClass(const Cursor &cursor) : mName(Utils::GetTypeNameWithoutNamespace(cursor.GetType()))
{
}

Class::Class(const Cursor &cursor, const Namespace &currentNamespace)
    : TypeInfo(cursor, currentNamespace)
    , mName(cursor.GetDisplayName())
    , mQualifiedName(Utils::GetTypeNameWithoutNamespace(cursor.GetType()))
    , mDisplayName(Utils::GetNameWithoutFirstM(mQualifiedName))
{
    Utils::ReplaceAll(mName, " ", "");
    Utils::ReplaceAll(mName, "MiniEngine::", "");

    for (auto& child : cursor.GetChildren())
    {
        switch (child.GetKind())
        {
            case CXCursor_CXXBaseSpecifier: {
                auto base_class = new BaseClass(child);

                mBaseClasses.emplace_back(base_class);
            }
            break;
            // field
            case CXCursor_FieldDecl:
                mFields.emplace_back(new Field(child, currentNamespace, this));
                break;
            default:
                break;
        }
    }
}

bool Class::ShouldCompile(void) const
{
    return ShouldCompileFields();
}

bool Class::ShouldCompileFields(void) const
{
    return mMetaData.GetFlag(NativeProperty::All) || 
           mMetaData.GetFlag(NativeProperty::Fields) ||
           mMetaData.GetFlag(NativeProperty::WhiteListFields);
}

std::string Class::GetClassName(void)
{
    return mName;
}

bool Class::IsAccessible(void) const
{
    return mbEnabled;
}
