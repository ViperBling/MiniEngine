#include "RenderSystem.h"
#include "Function/Render/Interface/RHI.h"
#include "Function/Render/Interface/Vulkan/VulkanRHI.h"

namespace MiniEngine
{
    void RenderSystem::Initialize(RenderSystemInitInfo initInfo) {

        RHIInitInfo rhiInitInfo;
        rhiInitInfo.windowSystem = initInfo.windowSystem;

        rhi = std::make_shared<VulkanRHI>();
        rhi->Initialize(rhiInitInfo);
    }
}