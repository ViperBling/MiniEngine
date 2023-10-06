#pragma once

#include "Generator/Generator.hpp"

namespace Generator
{
    class ReflectionGenerator : public GeneratorInterface
    {
    public:
        ReflectionGenerator() = delete;
        ReflectionGenerator(std::string sourceDirectory, std::function<std::string(std::string)> getIncludeFunc);
        virtual int  Generate(std::string path, SchemaMoudle schema) override;
        virtual void Finish() override;
        virtual ~ReflectionGenerator() override;

    protected:
        virtual void        PrepareStatus(std::string path) override;
        virtual std::string ProcessFileName(std::string path) override;

    private:
        std::vector<std::string> mHeadFileList;
        std::vector<std::string> mTypeList;
    };
} // namespace Generator
