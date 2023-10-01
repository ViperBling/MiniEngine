#include <cstdint>
#include <cstddef>

#include "DebugDrawManager.hpp"
#include "Function/Global/GlobalContext.hpp"
#include "Function/Render/RenderType.hpp"

namespace MiniEngine
{
    void DebugDrawManager::Initialize() 
    {
        mRHI = gRuntimeGlobalContext.mRenderSystem->GetRHI();
        SetupPipelines();
        mBufferAllocator = new DebugDrawAllocator();
    }

    void DebugDrawManager::SetupPipelines()
    {
        for (uint8_t i = 0; static_cast<DebugDrawPipelineType>(i) < DebugDrawPipelineType::Count; i++) {
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
        prepareDrawBuffer();
        DrawDebugObject(currentSwapChainImageIndex);
    }

    void DebugDrawManager::prepareDrawBuffer()
    {
        mBufferAllocator->Clear();
        std::vector<DebugDrawVertex> vertices;

        mDebugDrawGroupForRender.WriteTriangleData(vertices, true);
        mTriangleStartOffset = mBufferAllocator->CacheVertexs(vertices);
        mTriangleEndOffset = mBufferAllocator->GetVertexCacheOffset();
        mBufferAllocator->Allocator();
    }

    void DebugDrawManager::DrawDebugObject(uint32_t currentSwapChainImageIndex)
    {
        DrawPointLineTriangleBox(currentSwapChainImageIndex);
    }

    void DebugDrawManager::DrawPointLineTriangleBox(uint32_t currentSwapChainImageIndex) 
    {
        RHIBuffer* vertexBuffers[] = {mBufferAllocator->GetVertexBuffer()};
        if (vertexBuffers[0] == nullptr) return;

        RHIDeviceSize offsets[] = {0};
        mRHI->CmdBindVertexBuffersPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, vertexBuffers, offsets);

        std::vector<DebugDrawPipeline*> vcPipelines{
            mDebugDrawPipeline[static_cast<uint32_t>(DebugDrawPipelineType::Triangle)],
        };
        std::vector<size_t> vcStartOffset {mTriangleStartOffset};
        std::vector<size_t> vcEndOffset {mTriangleEndOffset};

        RHIClearValue clearValues[1];
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f}; // use black color as clear value
        // clearValues[1].depthStencil = {1.0f, 0};

        RHIRenderPassBeginInfo renderPassBeginInfo {};
        renderPassBeginInfo.sType             = RHI_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = mRHI->GetSwapChainInfo().extent;
        renderPassBeginInfo.clearValueCount   = (sizeof(clearValues) / sizeof(clearValues[0]));
        renderPassBeginInfo.pClearValues      = clearValues;

        size_t pipelineSize = vcPipelines.size();
        for (size_t i = 0; i < pipelineSize; i++) 
        {
            if (vcEndOffset[i] - vcStartOffset[i] == 0) continue;

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
