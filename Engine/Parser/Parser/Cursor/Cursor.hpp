#pragma once

#include "CursorType.hpp"

class Cursor
{
public:
    typedef std::vector<Cursor> List;
    typedef CXCursorVisitor Visitor;

    Cursor(const CXCursor& handle);
    CXCursorKind GetKind(void) const;
    std::string GetSpelling(void) const;
    std::string GetDisplayName(void) const;
    std::string GetSourceFile(void) const;
    bool IsDefinition(void) const;
    CursorType GetType(void) const;
    List GetChildren(void) const;
    void VisitChildren(Visitor visitor, void* data = nullptr);

private:
    CXCursor mHandle;
};