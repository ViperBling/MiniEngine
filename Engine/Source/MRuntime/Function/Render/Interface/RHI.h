#pragma once

#define GLFW_INCLUDE_VULKAN

#include <memory>

#include "Function/Render/Interface/RHIStruct.h"
#include "Function/Render/WindowSystem.h"

namespace MiniEngine
{
    struct RHIInitInfo
    {
        std::shared_ptr<WindowSystem> windowSystem;
    };

    class RHI
    {
    public:
        virtual void Initialize(RHIInitInfo initInfo) = 0;
        virtual void PrepareContext()                 = 0;

        // allocate and create
        virtual void CreateSwapChain()                                                      = 0;
        virtual void CreateSwapChainImageViews()                                            = 0;
        virtual RHIShader* CreateShaderModule(const std::vector<unsigned char>& shaderCode) = 0;
        virtual bool CreateGraphicsPipeline(
            RHIPipelineCache* pipelineCache,
            uint32_t createInfoCnt,
            const RHIGraphicsPipelineCreateInfo* pCreateInfo,
            RHIPipeline*& pPipelines)                                                       = 0;
        virtual bool CreatePiplineLayout(
            const RHIPipelineLayoutCreateInfo* pCreateInfo,
            RHIPipelineLayout*& pPipelineLayout)                                            = 0;

        virtual bool CreateRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass) = 0;
        virtual bool CreateFrameBuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFrameBuffer*& pFramebuffer) = 0;

        // command and command wirte
        virtual void CmdBeginRenderPassPFN(
            RHICommandBuffer*             commandBuffer,
            const RHIRenderPassBeginInfo* pRenderPassBegin,
            RHISubpassContents            contents) = 0;
        virtual void CmdBindPipelinePFN(
            RHICommandBuffer*    commandBuffer,
            RHIPipelineBindPoint pipelineBindPoint,
            RHIPipeline*         pipeline) = 0;
        virtual void CmdDraw(
            RHICommandBuffer* commandBuffer,
            uint32_t          vertexCount,
            uint32_t          instanceCount,
            uint32_t          firstVertex,
            uint32_t          firstInstance) = 0;
        virtual void CmdEndRenderPassPFN(RHICommandBuffer* commandBuffer) = 0;
        virtual void CmdSetViewportPFN(
            RHICommandBuffer*  commandBuffer,
            uint32_t           firstViewport,
            uint32_t           viewportCount,
            const RHIViewport* pViewports) = 0;
        virtual void CmdSetScissorPFN(
            RHICommandBuffer* commandBuffer,
            uint32_t          firstScissor,
            uint32_t          scissorCount,
            const RHIRect2D*  pScissors) = 0;
        virtual void WaitForFences() = 0;

        // query
        virtual RHISwapChainDesc GetSwapChainInfo() = 0;
        virtual RHICommandBuffer* GetCurrentCommandBuffer() const = 0;

        // command write
        virtual bool PrepareBeforePass() = 0;
        virtual void SubmitRendering()   = 0;
    };
}