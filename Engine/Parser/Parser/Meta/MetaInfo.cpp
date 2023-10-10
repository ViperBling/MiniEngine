#include "Common/PreCompiled.hpp"
#include "Parser/Parser.hpp"

#include "MetaInfo.hpp"

MetaInfo::MetaInfo(const Cursor &cursor)
{
    for (auto& child : cursor.GetChildren())
    {
        if (child.GetKind() != CXCursor_AnnotateAttr)
            continue;

        for (auto& prop : extractProperties(child))
            mProperties[prop.first] = prop.second;
    }
}

std::string MetaInfo::GetProperty(const std::string &key) const
{
    auto search = mProperties.find(key);

    // use an empty string by default
    return search == mProperties.end() ? "" : search->second;
}

bool MetaInfo::GetFlag(const std::string &key) const
{
    return mProperties.find(key) != mProperties.end();
}

std::vector<MetaInfo::Property> MetaInfo::extractProperties(const Cursor &cursor) const
{
    std::vector<Property> ret_list;

    auto propertyList = cursor.GetDisplayName();

    auto&& properties = Utils::Split(propertyList, ",");

    static const std::string white_space_string = " \t\r\n";

    for (auto& property_item : properties)
    {
        auto&& item_details = Utils::Split(property_item, ":");
        auto&& temp_string  = Utils::Trim(item_details[0], white_space_string);
        if (temp_string.empty())
        {
            continue;
        }
        ret_list.emplace_back(temp_string, item_details.size() > 1 ? Utils::Trim(item_details[1], white_space_string) : "");
    }
    return ret_list;
}
