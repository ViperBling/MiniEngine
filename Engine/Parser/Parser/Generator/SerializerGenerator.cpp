
#include "Common/PreCompiled.hpp"
#include "TemplateManager/TemplateManager.hpp"
#include "LanguageTypes/Class.hpp"

#include "SerializerGenerator.hpp"

namespace Generator
{
    SerializerGenerator::SerializerGenerator(std::string sourceDirectory, std::function<std::string(std::string)> getIncludeFunc)
        : GeneratorInterface(sourceDirectory + "/Generated/Serializer", sourceDirectory, getIncludeFunc)
    {
        PrepareStatus(mOutPath);
    }

    int SerializerGenerator::Generate(std::string path, SchemaMoudle schema)
    {
        std::string file_path = ProcessFileName(path);

        Mustache::data muatache_data;
        Mustache::data include_headfiles(Mustache::data::type::list);
        Mustache::data class_defines(Mustache::data::type::list);

        include_headfiles.push_back(
            Mustache::data("headfile_name", Utils::MakeRelativePath(mRootPath, path).string()));
        for (auto class_temp : schema.classes)
        {
            if (!class_temp->ShouldCompileFields())
                continue;

            Mustache::data class_def;
            GenClassRenderData(class_temp, class_def);

            // deal base class
            for (int index = 0; index < class_temp->mBaseClasses.size(); ++index)
            {
                auto include_file = mGetIncludeFunc(class_temp->mBaseClasses[index]->mName);
                if (!include_file.empty())
                {
                    auto include_file_base = ProcessFileName(include_file);
                    if (file_path != include_file_base)
                    {
                        include_headfiles.push_back(Mustache::data(
                            "headfile_name", Utils::MakeRelativePath(mRootPath, include_file_base).string()));
                    }
                }
            }
            for (auto field : class_temp->mFields)
            {
                if (!field->ShouldCompile())
                    continue;
                // deal vector
                if (field->mType.find("std::vector") == 0)
                {
                    auto include_file = mGetIncludeFunc(field->mName);
                    if (!include_file.empty())
                    {
                        auto include_file_base = ProcessFileName(include_file);
                        if (file_path != include_file_base)
                        {
                            include_headfiles.push_back(Mustache::data(
                                "headfile_name", Utils::MakeRelativePath(mRootPath, include_file_base).string()));
                        }
                    }
                }
                // deal normal
            }
            class_defines.push_back(class_def);
            mClassDefines.push_back(class_def);
        }

        muatache_data.set("class_defines", class_defines);
        muatache_data.set("include_headfiles", include_headfiles);
        std::string render_string =
            TemplateManager::GetInstance()->RenderByTemplate("commonSerializerGenFile", muatache_data);
        Utils::SaveFile(render_string, file_path);

        mIncludeHeadFiles.push_back(
            Mustache::data("headfile_name", Utils::MakeRelativePath(mRootPath, file_path).string()));
        return 0;
    }

    void SerializerGenerator::Finish()
    {
        Mustache::data mustache_data;
        mustache_data.set("class_defines", mClassDefines);
        mustache_data.set("include_headfiles", mIncludeHeadFiles);

        std::string render_string = TemplateManager::GetInstance()->RenderByTemplate("allSerializer.h", mustache_data);
        Utils::SaveFile(render_string, mOutPath + "/all_serializer.h");
        render_string = TemplateManager::GetInstance()->RenderByTemplate("allSerializer.ipp", mustache_data);
        Utils::SaveFile(render_string, mOutPath + "/all_serializer.ipp");
    }

    SerializerGenerator::~SerializerGenerator()
    {
    }

    void SerializerGenerator::PrepareStatus(std::string path)
    {
        GeneratorInterface::PrepareStatus(path);
        TemplateManager::GetInstance()->LoadTemplates(mRootPath, "allSerializer.h");
        TemplateManager::GetInstance()->LoadTemplates(mRootPath, "allSerializer.ipp");
        TemplateManager::GetInstance()->LoadTemplates(mRootPath, "commonSerializerGenFile");
    }

    std::string SerializerGenerator::ProcessFileName(std::string path)
    {
        auto relativeDir = fs::path(path).filename().replace_extension("serializer.gen.h").string();
        return mOutPath + "/" + relativeDir;
    }
}