#include "TemplateManager.hpp"

void TemplateManager::LoadTemplates(std::string path, std::string templateName)
{
    mTemplatePool.insert_or_assign(templateName,
                                     Utils::LoadFile(path + "/Parser/Template/" + templateName + ".mustache"));
}

std::string TemplateManager::RenderByTemplate(std::string templateName, Mustache::data &templateData)
{
    if (mTemplatePool.end() == mTemplatePool.find(templateName))
    {
        return "";
    }
    Mustache::mustache tmpl(mTemplatePool[templateName]);
    return tmpl.render(templateData);
}
