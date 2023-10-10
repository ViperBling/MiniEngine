#include "Common/PreCompiled.hpp"

#include "MetaUtils.hpp"

static int parseFlag = 0;

namespace Utils
{
    void ToString(const CXString &str, std::string &output)
    {
        auto cstr = clang_getCString(str);
        output = cstr;
        clang_disposeString(str);
    }

    std::string GetQualifiedName(const CursorType &type)
    {
        return type.GetDisplayName();
    }

    std::string GetQualifiedName(const std::string &display_name, const Namespace &currentNamespace)
    {
        std::string name = "";
        for (auto& iter_string : currentNamespace)
        {
            name += (iter_string + "::");
        }

        name += display_name;

        return name;
    }

    std::string GetQualifiedName(const Cursor &cursor, const Namespace &currentNamespace)
    {
        return GetQualifiedName(cursor.GetSpelling(), currentNamespace);
    }

    std::string FormatQualifiedName(std::string &sourceString)
    {
        Utils::Replace(sourceString, '<', 'L');
        Utils::Replace(sourceString, ':', 'S');
        Utils::Replace(sourceString, '>', 'R');
        Utils::Replace(sourceString, '*', 'P');
        return sourceString;
    }

    fs::path MakeRelativePath(const fs::path &from, const fs::path &to)
    {
        // Start at the root path and while they are the same then do nothing then when they first
        // diverge take the remainder of the two path and replace the entire from path with ".."
        // segments.
        std::string form_complete_string;
        std::string to_complete_string;

        /*remove ".." and "."*/
        (void)FormatPathString(from.string(), form_complete_string);
        (void)FormatPathString(to.string(), to_complete_string);

        fs::path form_complete = form_complete_string;
        fs::path to_complete   = to_complete_string;

        auto iter_from = form_complete.begin();
        auto iter_to   = to_complete.begin();

        // Loop through both
        while (iter_from != form_complete.end() && iter_to != to_complete.end() && (*iter_to) == (*iter_from))
        {
            ++iter_to;
            ++iter_from;
        }

        fs::path final_path;
        while (iter_from != form_complete.end())
        {
            final_path /= "..";

            ++iter_from;
        }

        while (iter_to != to_complete.end())
        {
            final_path /= *iter_to;

            ++iter_to;
        }

        return final_path;
    }

    void FatalError(const std::string &error)
    {
        std::cerr << "Error: " << error << std::endl;

        exit(EXIT_FAILURE);
    }

    std::vector<std::string> Split(std::string input, std::string pat)
    {
        std::vector<std::string> ret_list;
        while (true)
        {
            size_t      index    = input.find(pat);
            std::string sub_list = input.substr(0, index);
            if (!sub_list.empty())
            {

                ret_list.push_back(sub_list);
            }
            input.erase(0, index + pat.size());
            if (index == -1)
            {
                break;
            }
        }
        return ret_list;
    }

    std::string GetFileName(std::string path)
    {
        if (path.size() < 1)
        {
            return std::string();
        }
        std::vector<std::string> result = Split(path, "/");
        if (result.size() < 1)
        {
            return std::string();
        }
        return result[result.size() - 1];
    }

    std::string GetNameWithoutFirstM(std::string &name)
    {
        std::string result = name;
        if (name.size() > 2 && name[0] == 'm' && name[1] == '_')
        {
            result = name.substr(2);
        }
        return result;
    }

    std::string GetTypeNameWithoutNamespace(const CursorType &type)
    {
        std::string&& type_name = type.GetDisplayName();
        return type_name;
    }

    std::string GetNameWithoutContainer(std::string name)
    {
        size_t left  = name.find_first_of('<') + 1;
        size_t right = name.find_last_of('>');
        if (left > 0 && right < name.size() && left < right)
        {
            return name.substr(left, right - left);
        }
        else
        {
            return nullptr;
        }
    }

    std::string GetStringWithoutQuot(std::string input)
    {
        size_t left  = input.find_first_of('\"') + 1;
        size_t right = input.find_last_of('\"');
        if (left > 0 && right < input.size() && left < right)
        {
            return input.substr(left, right - left);
        }
        else
        {
            return input;
        }
    }

    std::string Replace(std::string &sourceString, std::string subString, const std::string newString)
    {
        std::string::size_type pos = 0;
        while ((pos = sourceString.find(subString)) != std::string::npos)
        {
            sourceString.replace(pos, subString.length(), newString);
        }
        return sourceString;
    }

    std::string Replace(std::string &sourceString, char tagetChar, const char newChar)
    {
        std::replace(sourceString.begin(), sourceString.end(), tagetChar, newChar);
        return sourceString;
    }

    std::string ToUpper(std::string &sourceString)
    {
        transform(sourceString.begin(), sourceString.end(), sourceString.begin(), ::toupper);
        return sourceString;
    }

    std::string Join(std::vector<std::string> context_list, std::string separator)
    {
        std::string ret_string;
        if (context_list.size() == 0)
        {
            return ret_string;
        }
        ret_string = context_list[0];
        for (int index = 1; index < context_list.size(); ++index)
        {
            ret_string += separator + context_list[index];
        }

        return ret_string;
    }

    std::string Trim(std::string &sourceString, const std::string trimChars)
    {
        size_t left_pos = sourceString.find_first_not_of(trimChars);
        if (left_pos == std::string::npos)
        {
            sourceString = std::string();
        }
        else
        {
            size_t right_pos = sourceString.find_last_not_of(trimChars);
            sourceString    = sourceString.substr(left_pos, right_pos - left_pos + 1);
        }
        return sourceString;
    }

    std::string LoadFile(std::string path)
    {
        std::ifstream      iFile(path);
        std::string        line_string;
        std::ostringstream template_stream;
        if (false == iFile.is_open())
        {
            iFile.close();
            return "";
        }
        while (std::getline(iFile, line_string))
        {
            template_stream << line_string << std::endl;
        }
        iFile.close();

        return template_stream.str();
    }

    void SaveFile(const std::string &outputString, const std::string &outputFile)
    {
        fs::path out_path(outputFile);

        if (!fs::exists(out_path.parent_path()))
        {
            fs::create_directories(out_path.parent_path());
        }
        std::fstream output_file_stream(outputFile, std::ios_base::out);

        output_file_stream << outputString << std::endl;
        output_file_stream.flush();
        output_file_stream.close();
        return;
    }

    void ReplaceAll(std::string &resourceStr, std::string subStr, std::string newStr)
    {
        std::string::size_type pos = 0;
        while ((pos = resourceStr.find(subStr)) != std::string::npos)
        {
            resourceStr = resourceStr.replace(pos, subStr.length(), newStr);
        }
        return;
    }

    unsigned long FormatPathString(const std::string &pathString, std::string &outString)
    {
        unsigned int ulRet             = 0;
        auto         local_path_string = pathString;
        fs::path     local_path;

        local_path = local_path_string;
        if (local_path.is_relative())
        {
            local_path_string = fs::current_path().string() + "/" + local_path_string;
        }

        ReplaceAll(local_path_string, "\\", "/");
        std::vector<std::string> subString = Split(local_path_string, "/");
        std::vector<std::string> out_sub_string;
        for (auto p : subString)
        {
            if (p == "..")
            {
                out_sub_string.pop_back();
            }
            else if (p != ".")
            {
                out_sub_string.push_back(p);
            }
        }
        for (int i = 0; i < out_sub_string.size() - 1; i++)
        {
            outString.append(out_sub_string[i] + "/");
        }
        outString.append(out_sub_string[out_sub_string.size() - 1]);
        return 0;
    }

} // namespace Utils
