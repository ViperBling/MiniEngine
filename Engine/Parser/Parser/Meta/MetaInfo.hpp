#pragma once

#include "Cursor/Cursor.hpp"

class MetaInfo
{
public:
    MetaInfo(const Cursor& cursor);
    std::string GetProperty(const std::string& key) const;
    bool GetFlag(const std::string& key) const;
    
private:
    typedef std::pair<std::string, std::string> Property;
    std::unordered_map<std::string, std::string> mProperties;

private:
    std::vector<Property> extractProperties(const Cursor& cursor) const;
};