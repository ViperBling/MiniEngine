#pragma once

#include "Common/PreCompiled.hpp"
#include "Common/Namespace.hpp"
#include "Common/SchemaModule.hpp"

#include "Cursor/Cursor.hpp"

#include "Generator/Generator.hpp"
#include "TemplateManager/TemplateManager.hpp"

class Class;

class MetaParser
{
public:
    
    MetaParser(
        const std::string projectInputFile,
        const std::string includeFilePath,
        const std::string includePath,
        const std::string includeSys,
        const std::string moduleName,
        bool              isShowErrors);
    ~MetaParser(void);

    static void Prepare(void);
    void Finish(void);
    int  Parse(void);
    void GenerateFiles(void);

private:
    bool        parseProject(void);
    void        buildClassAST(const Cursor& cursor, Namespace& currentNamespace);
    std::string getIncludeFile(std::string name);

private:
    std::string mProjectInputFile;

    std::vector<std::string> mWorkPaths;
    std::string              mModuleName;
    std::string              mSysInclude;
    std::string              mSrcIncludeFileName;

    CXIndex           mIndex;
    CXTranslationUnit mTranslationUnit;

    std::unordered_map<std::string, std::string>  mTypeTable;
    std::unordered_map<std::string, SchemaMoudle> mSchemaModules;

    std::vector<const char*> arguments = {
        {"-x",
        "c++",
        "-std=c++11",
        "-D__REFLECTION_PARSER__",
        "-DNDEBUG",
        "-D__clang__",
        "-w",
        "-MG",
        "-M",
        "-ferror-limit=0",
        "-o clangLog.txt"}
    };
    std::vector<Generator::GeneratorInterface*> mGenerators;

    bool mbIsShowErrors;
};
