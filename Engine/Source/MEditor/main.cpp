#include <filesystem>

#include "MEditor/MEditor.h"
#include "MRuntime/MEngine.h"

int main(int argc, char* argv[])
{
    std::filesystem::path executablePath(argv[0]);
    std::filesystem::path configFilePath = executablePath.parent_path();

    // 初始化
    MiniEngine::MEngine* Engine =
        new MiniEngine::MEngine();

    Engine->Initialize(configFilePath.generic_string());

    // 创建编辑器
    MiniEngine::MEditor* Editor = new MiniEngine::MEditor();
    Editor->Initialize(Engine);

    // 引擎运行
    Editor->Run();

    // 关闭
    Editor->Finalize();
    Engine->Finalize();

    return 0;
}