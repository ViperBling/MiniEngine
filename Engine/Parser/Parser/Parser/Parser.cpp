#include "Common/PreCompiled.hpp"
#include "LanguageTypes/Class.hpp"

#include "Generator/ReflectionGenerator.hpp"
#include "Generator/SerializerGenerator.hpp"

#include "Parser.hpp"

#define RECURSE_NAMESPACES(kind, cursor, method, namespaces) \
    { \
        if (kind == CXCursor_Namespace) \
        { \
            auto display_name = cursor.GetDisplayName(); \
            if (!display_name.empty()) \
            { \
                namespaces.emplace_back(display_name); \
                method(cursor, namespaces); \
                namespaces.pop_back(); \
            } \
        } \
    }

#define TRY_ADD_LANGUAGE_TYPE(handle, container) \
    { \
        if (handle->ShouldCompile()) \
        { \
            auto file = handle->GetSourceFile(); \
            mSchemaModules[file].container.emplace_back(handle); \
            mTypeTable[handle->mDisplayName] = file; \
        } \
    }

MetaParser::MetaParser(
    const std::string projectInputFile, 
    const std::string includeFilePath, 
    const std::string includePath, 
    const std::string includeSys, 
    const std::string moduleName, bool isShowErrors)
    : mProjectInputFile(projectInputFile)
    , mSrcIncludeFileName(includeFilePath)
    , mIndex(nullptr)
    , mTranslationUnit(nullptr)
    , mSysInclude(includeSys)
    , mModuleName(moduleName)
    , mbIsShowErrors(isShowErrors)
{
    mWorkPaths = Utils::Split(includePath, ";");

    mGenerators.emplace_back(new Generator::SerializerGenerator(
        mWorkPaths[0], 
        std::bind(&MetaParser::getIncludeFile, this, std::placeholders::_1))
    );
    mGenerators.emplace_back(new Generator::ReflectionGenerator(
        mWorkPaths[0], 
        std::bind(&MetaParser::getIncludeFile, this, std::placeholders::_1))
    );
}

MetaParser::~MetaParser(void)
{
    for (auto item : mGenerators)
    {
        delete item;
    }
    mGenerators.clear();

    if (mTranslationUnit)
        clang_disposeTranslationUnit(mTranslationUnit);

    if (mIndex)
        clang_disposeIndex(mIndex);
}

void MetaParser::Prepare(void)
{
}

void MetaParser::Finish(void)
{
    for (auto generator_iter : mGenerators)
    {
        generator_iter->Finish();
    }
}

int MetaParser::Parse(void)
{
    bool parse_include_ = parseProject();
    if (!parse_include_)
    {
        std::cerr << "Parsing project file error! " << std::endl;
        return -1;
    }

    std::cerr << "Parsing the whole project..." << std::endl;
    int is_show_errors      = mbIsShowErrors ? 1 : 0;
    mIndex                 = clang_createIndex(true, is_show_errors);
    std::string pre_include = "-I";
    std::string sys_include_temp;
    if (!(mSysInclude == "*"))
    {
        sys_include_temp = pre_include + mSysInclude;
        arguments.emplace_back(sys_include_temp.c_str());
    }

    auto paths = mWorkPaths;
    for (int index = 0; index < paths.size(); ++index)
    {
        paths[index] = pre_include + paths[index];

        arguments.emplace_back(paths[index].c_str());
    }

    fs::path input_path(mSrcIncludeFileName);
    if (!fs::exists(input_path))
    {
        std::cerr << input_path << " is not exist" << std::endl;
        return -2;
    }

    mTranslationUnit = clang_createTranslationUnitFromSourceFile(
        mIndex, mSrcIncludeFileName.c_str(), static_cast<int>(arguments.size()), arguments.data(), 0, nullptr);
    auto cursor = clang_getTranslationUnitCursor(mTranslationUnit);

    Namespace temp_namespace;

    buildClassAST(cursor, temp_namespace);

    temp_namespace.clear();

    return 0;
}

void MetaParser::GenerateFiles(void)
{
    std::cerr << "Start generate runtime schemas(" << mSchemaModules.size() << ")..." << std::endl;
    for (auto& schema : mSchemaModules)
    {
        for (auto& generator_iter : mGenerators)
        {
            generator_iter->Generate(schema.first, schema.second);
        }
    }

    Finish();
}

bool MetaParser::parseProject(void)
{
    bool result = true;
    std::cout << "Parsing project file: " << mProjectInputFile << std::endl;

    std::fstream include_txt_file(mProjectInputFile, std::ios::in);

    if (include_txt_file.fail())
    {
        std::cout << "Could not load file: " << mProjectInputFile << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << include_txt_file.rdbuf();

    std::string context = buffer.str();

    auto         inlcude_files = Utils::Split(context, ";");
    std::fstream include_file;

    include_file.open(mSrcIncludeFileName, std::ios::out);
    if (!include_file.is_open())
    {
        std::cout << "Could not open the Source Include file: " << mSrcIncludeFileName << std::endl;
        return false;
    }

    std::cout << "Generating the Source Include file: " << mSrcIncludeFileName << std::endl;

    std::string output_filename = Utils::GetFileName(mSrcIncludeFileName);

    if (output_filename.empty())
    {
        output_filename = "META_INPUT_HEADER_H";
    }
    else
    {
        Utils::Replace(output_filename, ".", "_");
        Utils::Replace(output_filename, " ", "_");
        Utils::ToUpper(output_filename);
    }
    include_file << "#ifndef __" << output_filename << "__" << std::endl;
    include_file << "#define __" << output_filename << "__" << std::endl;

    for (auto include_item : inlcude_files)
    {
        std::string temp_string(include_item);
        Utils::Replace(temp_string, '\\', '/');
        include_file << "#include  \"" << temp_string << "\"" << std::endl;
    }

    include_file << "#endif" << std::endl;
    include_file.close();
    return result;
}

void MetaParser::buildClassAST(const Cursor &cursor, Namespace &currentNamespace)
{
    for (auto& child : cursor.GetChildren())
    {
        auto kind = child.GetKind();

        // actual definition and a class or struct
        if (child.IsDefinition() && (kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl))
        {
            auto class_ptr = std::make_shared<Class>(child, currentNamespace);

            TRY_ADD_LANGUAGE_TYPE(class_ptr, classes);
        }
        else
        {
            RECURSE_NAMESPACES(kind, child, buildClassAST, currentNamespace);
        }
    }
}

std::string MetaParser::getIncludeFile(std::string name)
{
    auto iter = mTypeTable.find(name);
    return iter == mTypeTable.end() ? std::string() : iter->second;
}
