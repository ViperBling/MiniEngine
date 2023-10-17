#include <filesystem>

#include "MEditor/MEditor.hpp"
#include "MRuntime/MEngine.hpp"

int main(int argc, char* argv[])
{
    std::filesystem::path executablePath(argv[0]);
    std::filesystem::path configFilePath = executablePath.parent_path() / "MiniEngineEditor.ini";

    // 初始化
    auto Engine = new MiniEngine::MEngine();

    Engine->StartEngine(configFilePath.generic_string());
    Engine->Initialize();

    // 创建编辑器
    auto Editor = new MiniEngine::MEditor();
    Editor->Initialize(Engine);

    // 引擎运行
    Editor->Run();

    // 关闭
    Editor->Finalize();

    Engine->Clear();
    Engine->ShutdownEngine();

    return 0;
}