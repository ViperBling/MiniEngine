#include "RenderPipeline.h"
#include "Function/Global/GlobalContext.h"
#include "Function/Render/Interface/Vulkan/VulkanRHI.h"

namespace MiniEngine
{
    void RenderPipeline::ForwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> renderResource) {

        auto vulkanRHI = static_cast<VulkanRHI*>(rhi.get());

        vulkanRHI->WaitForFences();
        if (!vulkanRHI->PrepareBeforePass()) {
            return;
        }

        mRHI->CmdSetViewportPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().viewport);
        mRHI->CmdSetScissorPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().scissor);

        gRuntimeGlobalContext.mDebugDrawManager->Draw(vulkanRHI->mCurrentSwapChainImageIndex);
        vulkanRHI->SubmitRendering();
    }
}