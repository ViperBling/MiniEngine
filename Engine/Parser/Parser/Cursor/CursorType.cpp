#include "Common/PreCompiled.hpp"
#include "Cursor.hpp"

#include "CursorType.hpp"

CursorType::CursorType(const CXType &handle)
    : mHandle(handle)
{}

std::string CursorType::GetDisplayName(void) const
{
    std::string displayName;

    Utils::ToString(clang_getTypeSpelling(mHandle), displayName);

    return displayName;
}

int CursorType::GetArgumentCount(void) const
{
    return clang_getNumArgTypes(mHandle);
}

CursorType CursorType::GetArgument(unsigned index) const
{
    return clang_getArgType(mHandle, index);
}

CursorType CursorType::GetCanonicalType(void) const
{
    return clang_getCanonicalType(mHandle);
}

Cursor CursorType::GetDeclaration(void) const
{
    return clang_getTypeDeclaration(mHandle);
}

CXTypeKind CursorType::GetKind(void) const
{
    return mHandle.kind;
}

bool CursorType::IsConst(void) const
{
    return clang_isConstQualifiedType(mHandle) ? true : false;
}
