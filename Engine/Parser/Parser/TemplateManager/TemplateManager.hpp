#pragma once

#include "Common/PreCompiled.hpp"

class TemplateManager
{
public:
    static TemplateManager* GetInstance()
    {
        static TemplateManager* m_pInstance;
        if (nullptr == m_pInstance)
            m_pInstance = new TemplateManager();
        return m_pInstance;
    }
    void LoadTemplates(std::string path, std::string templateName);

    std::string RenderByTemplate(std::string templateName, Mustache::data& templateData);

private:
    TemplateManager() {}
    TemplateManager(const TemplateManager&);
    TemplateManager&                             operator=(const TemplateManager&);
    std::unordered_map<std::string, std::string> mTemplatePool;
};
