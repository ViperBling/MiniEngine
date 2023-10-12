#include "RenderPipeline.hpp"
#include "MRuntime/Core/Base/Marco.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Render/Interface/Vulkan/VulkanRHI.hpp"
#include "MRuntime/Function/Render/DebugDraw/DebugDrawManager.hpp"

namespace MiniEngine
{
    void RenderPipeline::ForwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> renderResource) {

        auto vulkanRHI = static_cast<VulkanRHI*>(rhi.get());

        vulkanRHI->WaitForFences();
        if (!vulkanRHI->PrepareBeforePass([this] { PassUpdateAfterRecreateSwapChain(); }))
        {
            return;
        }

        mRHI->CmdSetViewportPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().viewport);
        mRHI->CmdSetScissorPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().scissor);

        gRuntimeGlobalContext.mDebugDrawManager->Draw(vulkanRHI->mCurrentSwapChainImageIndex);
        vulkanRHI->SubmitRendering([this] { PassUpdateAfterRecreateSwapChain(); });
    }

    void RenderPipeline::PassUpdateAfterRecreateSwapChain()
    {
        gRuntimeGlobalContext.mDebugDrawManager->UpdateAfterRecreateSwapChain();
    }
}