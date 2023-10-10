#pragma once

#include "Common/SchemaModule.hpp"

#include <functional>
#include <string>

namespace Generator
{
    class GeneratorInterface
    {
    public:
        GeneratorInterface(
            std::string                             outPath,
            std::string                             rootPath,
            std::function<std::string(std::string)> getIncludeFunc
        ) 
            : mOutPath(outPath)
            , mRootPath(rootPath)
            , mGetIncludeFunc(getIncludeFunc)
        {}
        virtual int  Generate(std::string path, SchemaMoudle schema) = 0;
        virtual void Finish() {};

        virtual ~GeneratorInterface() {};

    protected:
        virtual void        PrepareStatus(std::string path);
        virtual void        GenClassRenderData(std::shared_ptr<Class> classTmp, Mustache::data& classDef);
        virtual void        GenClassFieldRenderData(std::shared_ptr<Class> classTmp, Mustache::data& fieldDefs);
        virtual std::string ProcessFileName(std::string path) = 0;

        std::string                             mOutPath {"GenSrc"};
        std::string                             mRootPath;
        std::function<std::string(std::string)> mGetIncludeFunc;
    };
} // namespace Generator
