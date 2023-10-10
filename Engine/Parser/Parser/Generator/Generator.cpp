#include "Common/PreCompiled.hpp"
#include "LanguageTypes/Class.hpp"

#include "Generator.hpp"

namespace Generator
{
    void GeneratorInterface::PrepareStatus(std::string path)
    {
        if (!fs::exists(path))
        {
            fs::create_directories(path);
        }
    }

    void GeneratorInterface::GenClassRenderData(std::shared_ptr<Class> classTmp, Mustache::data &classDef)
    {
        classDef.set("class_name", classTmp->GetClassName());
        classDef.set("class_base_class_size", std::to_string(classTmp->mBaseClasses.size()));
        classDef.set("class_need_register", true);

        if (classTmp->mBaseClasses.size() > 0)
        {
            Mustache::data class_base_class_defines(Mustache::data::type::list);
            classDef.set("class_has_base", true);
            for (int index = 0; index < classTmp->mBaseClasses.size(); ++index)
            {
                Mustache::data class_base_class_def;
                class_base_class_def.set("class_base_class_name", classTmp->mBaseClasses[index]->mName);
                class_base_class_def.set("class_base_class_index", std::to_string(index));
                class_base_class_defines.push_back(class_base_class_def);
            }
            classDef.set("class_base_class_defines", class_base_class_defines);
        }

        Mustache::data class_field_defines = Mustache::data::type::list;
        GenClassFieldRenderData(classTmp, class_field_defines);
        classDef.set("class_field_defines", class_field_defines);
    }

    void GeneratorInterface::GenClassFieldRenderData(std::shared_ptr<Class> classTmp, Mustache::data &fieldDefs)
    {
        static const std::string vector_prefix = "std::vector<";

        for (auto& field : classTmp->mFields)
        {
            if (!field->ShouldCompile())
                continue;
            Mustache::data filed_define;

            filed_define.set("class_field_name", field->mName);
            filed_define.set("class_field_type", field->mType);
            filed_define.set("class_field_display_name", field->mDisplayName);
            bool is_vector = field->mType.find(vector_prefix) == 0;
            filed_define.set("class_field_is_vector", is_vector);
            fieldDefs.push_back(filed_define);
        }
    }
}
