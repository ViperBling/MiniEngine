#pragma once

#include "Common/PreCompiled.hpp"
#include "Common/Namespace.hpp"
#include "Cursor/Cursor.hpp"

namespace Utils
{   
    void ToString(const CXString& str, std::string& output);
    std::string GetQualifiedName(const CursorType& type);
    std::string GetQualifiedName(const std::string& display_name, const Namespace& currentNamespace);
    std::string GetQualifiedName(const Cursor& cursor, const Namespace& currentNamespace);
    std::string FormatQualifiedName(std::string& sourceString);
    fs::path MakeRelativePath(const fs::path& from, const fs::path& to);
    void FatalError(const std::string& error);

    template<typename A, typename B>
    bool RangeEqual(A startA, A endA, B startB, B endB);

    std::vector<std::string> Split(std::string input, std::string pat);
    std::string GetFileName(std::string path);
    std::string GetNameWithoutFirstM(std::string& name);
    std::string GetTypeNameWithoutNamespace(const CursorType& type);
    std::string GetNameWithoutContainer(std::string name);
    std::string GetStringWithoutQuot(std::string input);
    std::string Replace(std::string& sourceString, std::string subString, const std::string newString);
    std::string Replace(std::string& sourceString, char tagetChar, const char newChar);
    std::string ToUpper(std::string& sourceString);
    std::string Join(std::vector<std::string> context_list, std::string separator);
    std::string Trim(std::string& sourceString, const std::string trimChars);
    std::string LoadFile(std::string path);
    void SaveFile(const std::string& outputString, const std::string& outputFile);
    void ReplaceAll(std::string& resourceStr, std::string subStr, std::string newStr);
    unsigned long FormatPathString(const std::string& pathString, std::string& outString);

    template<typename A, typename B>
    bool RangeEqual(A startA, A endA, B startB, B endB)
    {
        while (startA != endA && startB != endB)
        {
            if (*startA != *startB)
                return false;

            ++startA;
            ++startB;
        }

        return (startA == endA) && (startB == endB);
    }

} // namespace Utils
