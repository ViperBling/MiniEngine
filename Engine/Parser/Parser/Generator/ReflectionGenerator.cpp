#include <iostream>
#include <map>
#include <set>

#include "Common/PreCompiled.hpp"
#include "LanguageTypes/Class.hpp"
#include "TemplateManager/TemplateManager.hpp"

#include "ReflectionGenerator.hpp"

namespace Generator
{
    ReflectionGenerator::ReflectionGenerator(std::string sourceDirectory, std::function<std::string(std::string)> getIncludeFunc)
         : GeneratorInterface(sourceDirectory + "/Generated/Reflection", sourceDirectory, getIncludeFunc)
    {
        PrepareStatus(mOutPath);
    }

    int ReflectionGenerator::Generate(std::string path, SchemaMoudle schema)
    {
        static const std::string vectorPrefix = "std::vector<";

        std::string    filePath = ProcessFileName(path);
        Mustache::data mustacheData;
        Mustache::data includeHeadfiles(Mustache::data::type::list);
        Mustache::data classDefines(Mustache::data::type::list);

        includeHeadfiles.push_back(
            Mustache::data("headfile_name", Utils::MakeRelativePath(mRootPath, path).string()));

        std::map<std::string, bool> classNames;
        // class defs
        for (auto class_temp : schema.classes)
        {
            if (!class_temp->ShouldCompileFields())
                continue;

            classNames.insert_or_assign(class_temp->GetClassName(), false);
            classNames[class_temp->GetClassName()] = true;

            std::vector<std::string>                                   field_names;
            std::map<std::string, std::pair<std::string, std::string>> vector_map;

            Mustache::data class_def;
            Mustache::data vector_defines(Mustache::data::type::list);

            GenClassRenderData(class_temp, class_def);
            for (auto field : class_temp->mFields)
            {
                if (!field->ShouldCompile())
                    continue;
                field_names.emplace_back(field->mName);
                bool is_array = field->mType.find(vectorPrefix) == 0;
                if (is_array)
                {
                    std::string array_useful_name = field->mType;

                    Utils::FormatQualifiedName(array_useful_name);

                    std::string item_type = field->mType;

                    item_type = Utils::GetNameWithoutContainer(item_type);

                    vector_map[field->mType] = std::make_pair(array_useful_name, item_type);
                }
            }

            if (vector_map.size() > 0)
            {
                if (nullptr == class_def.get("vector_exist"))
                {
                    class_def.set("vector_exist", true);
                }
                for (auto vector_item : vector_map)
                {
                    std::string    array_useful_name = vector_item.second.first;
                    std::string    item_type         = vector_item.second.second;
                    Mustache::data vector_define;
                    vector_define.set("vector_useful_name", array_useful_name);
                    vector_define.set("vector_type_name", vector_item.first);
                    vector_define.set("vector_element_type_name", item_type);
                    vector_defines.push_back(vector_define);
                }
            }
            class_def.set("vector_defines", vector_defines);
            classDefines.push_back(class_def);
        }

        mustacheData.set("class_defines", classDefines);
        mustacheData.set("include_headfiles", includeHeadfiles);
        std::string render_string =
            TemplateManager::GetInstance()->RenderByTemplate("commonReflectionFile", mustacheData);
        Utils::SaveFile(render_string, filePath);

        for (auto class_item : classNames)
        {
            mTypeList.emplace_back(class_item.first);
        }

        mHeadFileList.emplace_back(Utils::MakeRelativePath(mRootPath, filePath).string());

        return 0;
    }

    void ReflectionGenerator::Finish()
    {
        Mustache::data mustache_data;
        Mustache::data include_headfiles = Mustache::data::type::list;
        Mustache::data class_defines     = Mustache::data::type::list;

        for (auto& head_file : mHeadFileList)
        {
            include_headfiles.push_back(Mustache::data("headfile_name", head_file));
        }
        for (auto& class_name : mTypeList)
        {
            class_defines.push_back(Mustache::data("class_name", class_name));
        }
        mustache_data.set("include_headfiles", include_headfiles);
        mustache_data.set("class_defines", class_defines);
        std::string render_string =
            TemplateManager::GetInstance()->RenderByTemplate("allReflectionFile", mustache_data);
        Utils::SaveFile(render_string, mOutPath + "/all_reflection.h");
    }

    ReflectionGenerator::~ReflectionGenerator()
    {
    }

    void ReflectionGenerator::PrepareStatus(std::string path)
    {
        GeneratorInterface::PrepareStatus(path);
        TemplateManager::GetInstance()->LoadTemplates(mRootPath, "commonReflectionFile");
        TemplateManager::GetInstance()->LoadTemplates(mRootPath, "allReflectionFile");
    }

    std::string ReflectionGenerator::ProcessFileName(std::string path)
    {
        auto relativeDir = fs::path(path).filename().replace_extension("reflection.gen.h").string();
        return mOutPath + "/" + relativeDir;
    }
}