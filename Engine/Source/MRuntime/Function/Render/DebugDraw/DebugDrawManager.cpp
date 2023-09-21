#include <cstdint>

#include "DebugDrawManager.h"
#include "Function/Global/GlobalContext.h"


namespace MiniEngine
{
    void DebugDrawManager::Initialize() 
    {
        mRHI = gRuntimeGlobalContext.mRenderSystem->GetRHI();
        SetupPipelines();
    }

    void DebugDrawManager::SetupPipelines()
    {
        for (uint8_t i = 0; static_cast<DebugDrawPipelineType>(i) < DebugDrawPipelineType::count; i++) {
            mDebugDrawPipeline[i] = new DebugDrawPipeline(static_cast<DebugDrawPipelineType>(i));
            mDebugDrawPipeline[i]->Initialize();
        }
    }

    void DebugDrawManager::UpdateAfterRecreateSwapChain()
    {
        for (auto & pipeline : mDebugDrawPipeline)
        {
            pipeline->RecreateAfterSwapChain();
        }
    }

    void DebugDrawManager::Draw(uint32_t currentSwapChainImageIndex) 
    {
        DrawDebugObject(currentSwapChainImageIndex);
    }

    void DebugDrawManager::DrawDebugObject(uint32_t currentSwapChainImageIndex) 
    {
        DrawPointLineTriangleBox(currentSwapChainImageIndex);
    }

    void DebugDrawManager::DrawPointLineTriangleBox(uint32_t currentSwapChainImageIndex) 
    {
        std::vector<DebugDrawPipeline*> vcPipelines{
            mDebugDrawPipeline[static_cast<uint32_t>(DebugDrawPipelineType::triangle)],
        };
        std::vector<size_t> vcStartOffset {mTriangleStartOffset = 0};
        std::vector<size_t> vcEndOffset {mTriangleEndOffset = 3};

        RHIClearValue clearValues[1];
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f}; // use black color as clear value
        // clearValues[1].depthStencil = {1.0f, 0};

        RHIRenderPassBeginInfo renderPassBeginInfo {};
        renderPassBeginInfo.sType             = RHI_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = mRHI->GetSwapChainInfo().extent;
        renderPassBeginInfo.clearValueCount   = (sizeof(clearValues) / sizeof(clearValues[0]));
        renderPassBeginInfo.pClearValues      = clearValues;

        for (size_t i = 0; i < vcPipelines.size(); i++) 
        {
            renderPassBeginInfo.renderPass = vcPipelines[i]->GetFrameBuffer().renderPass;
            renderPassBeginInfo.framebuffer = vcPipelines[i]->GetFrameBuffer().framebuffers[currentSwapChainImageIndex];
            mRHI->CmdBeginRenderPassPFN(
                mRHI->GetCurrentCommandBuffer(),
                &renderPassBeginInfo,
                RHI_SUBPASS_CONTENTS_INLINE);
            mRHI->CmdBindPipelinePFN(
                mRHI->GetCurrentCommandBuffer(),
                RHI_PIPELINE_BIND_POINT_GRAPHICS,
                vcPipelines[i]->GetPipeline().pipeline);
            mRHI->CmdDraw(
                mRHI->GetCurrentCommandBuffer(),
                vcEndOffset[i] - vcStartOffset[i], 1,
                vcStartOffset[i], 0);
            mRHI->CmdEndRenderPassPFN(mRHI->GetCurrentCommandBuffer());
        }
    }
}
