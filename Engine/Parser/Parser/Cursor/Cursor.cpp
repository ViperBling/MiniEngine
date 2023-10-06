#include "Common/PreCompiled.hpp"
#include "Meta/MetaUtils.hpp"

#include "Cursor.hpp"

Cursor::Cursor(const CXCursor &handle)
    : mHandle(handle)
{}

CXCursorKind Cursor::GetKind(void) const
{
    return mHandle.kind;
}

std::string Cursor::GetSpelling(void) const
{
    std::string spelling;

    Utils::ToString(clang_getCursorSpelling(mHandle), spelling);
    return spelling;
}

std::string Cursor::GetDisplayName(void) const
{
    std::string displayName;

    Utils::ToString(clang_getCursorDisplayName(mHandle), displayName);

    return displayName;
}

std::string Cursor::GetSourceFile(void) const
{
    auto range = clang_Cursor_getSpellingNameRange(mHandle, 0, 0);

    auto start = clang_getRangeStart(range);

    CXFile   file;
    unsigned line, column, offset;

    clang_getFileLocation(start, &file, &line, &column, &offset);

    std::string filename;

    Utils::ToString(clang_getFileName(file), filename);

    return filename;
}

bool Cursor::IsDefinition(void) const
{
    return clang_isCursorDefinition(mHandle);
}

CursorType Cursor::GetType(void) const
{
    return clang_getCursorType(mHandle);
}

Cursor::List Cursor::GetChildren(void) const
{
    List children;

    auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data) {
        auto container = static_cast<List*>(data);

        container->emplace_back(cursor);

        if (cursor.kind == CXCursor_LastPreprocessing)
            return CXChildVisit_Break;

        return CXChildVisit_Continue;
    };

    clang_visitChildren(mHandle, visitor, &children);

    return children;
}

void Cursor::VisitChildren(Visitor visitor, void *data)
{
    clang_visitChildren(mHandle, visitor, data);
}
