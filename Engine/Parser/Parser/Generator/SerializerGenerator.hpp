#pragma once

#include "Generator/Generator.hpp"

namespace Generator
{
    class SerializerGenerator : public GeneratorInterface
    {
    public:
        SerializerGenerator() = delete;
        SerializerGenerator(std::string sourceDirectory, std::function<std::string(std::string)> getIncludeFunc);

        virtual int Generate(std::string path, SchemaMoudle schema) override;
        virtual void Finish() override;

        virtual ~SerializerGenerator() override;

    protected:
        virtual void PrepareStatus(std::string path) override;
        virtual std::string ProcessFileName(std::string path) override;

    private:
        Mustache::data mClassDefines {Mustache::data::type::list};
        Mustache::data mIncludeHeadFiles {Mustache::data::type::list};
    };
} // namespace Generator
