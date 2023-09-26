#pragma once

#define GLFW_INCLUDE_VULKAN

#include <memory>

#include "Function/Render/Interface/RHIStruct.hpp"
#include "Function/Render/WindowSystem.hpp"

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
            RHIPipelineCache*                    pipelineCache,
            uint32_t                             createInfoCnt,
            const RHIGraphicsPipelineCreateInfo* pCreateInfo,
            RHIPipeline*&                        pPipelines
        )  = 0;
        virtual bool CreatePipelineLayout(
            const RHIPipelineLayoutCreateInfo*  pCreateInfo,
            RHIPipelineLayout*&                 pPipelineLayout
        ) = 0;

        virtual bool CreateRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass) = 0;
        virtual bool CreateFrameBuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFrameBuffer*& pFrameBuffer) = 0;
        virtual void RecreateSwapChain() = 0;
        virtual void CreateBuffer(
            RHIDeviceSize           size,
            RHIBufferUsageFlags     usageFlags,
            RHIMemoryPropertyFlags  properties,
            RHIBuffer*&             buffer,
            RHIDeviceMemory*&       bufferMemory
        ) = 0;

        // command and command write
        virtual void CmdBindVertexBuffersPFN(
            RHICommandBuffer*       cmdBuffer,
            uint32_t                firstBinding,
            uint32_t                bindingCount,
            RHIBuffer* const*       pBuffers,
            const RHIDeviceSize*    pOffsets
        ) = 0;
        virtual void CmdBeginRenderPassPFN(
            RHICommandBuffer* commandBuffer, 
            const RHIRenderPassBeginInfo* pRenderPassBegin, 
            RHISubpassContents contents
        ) = 0;
        virtual void CmdBindPipelinePFN(
            RHICommandBuffer* commandBuffer, 
            RHIPipelineBindPoint pipelineBindPoint, 
            RHIPipeline* pipeline
        ) = 0;
        virtual void CmdDraw(
            RHICommandBuffer* commandBuffer, 
            uint32_t vertexCount, 
            uint32_t instanceCount, 
            uint32_t firstVertex, 
            uint32_t firstInstance
        ) = 0;
        virtual void CmdEndRenderPassPFN(RHICommandBuffer* commandBuffer) = 0;
        virtual void CmdSetViewportPFN(
            RHICommandBuffer* commandBuffer, 
            uint32_t firstViewport, 
            uint32_t viewportCount, 
            const RHIViewport* pViewports
        ) = 0;
        virtual void CmdSetScissorPFN(
            RHICommandBuffer* commandBuffer, 
            uint32_t firstScissor, 
            uint32_t scissorCount, 
            const RHIRect2D* pScissors
        ) = 0;
        virtual void WaitForFences() = 0;

        // query
        virtual RHISwapChainDesc GetSwapChainInfo() = 0;
        virtual RHICommandBuffer* GetCurrentCommandBuffer() const = 0;

        // command write
        virtual bool PrepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapChain) = 0;
        virtual void SubmitRendering(std::function<void()> passUpdateAfterRecreateSwapChain)   = 0;

        virtual void DestroyFrameBuffer(RHIFrameBuffer* frameBuffer) = 0;

        // Buffer Memory
        virtual bool MapMemory(
            RHIDeviceMemory* memory,
            RHIDeviceSize offset,
            RHIDeviceSize size,
            RHIMemoryMapFlags flags,
            void** ppData
        ) = 0;
        virtual void UnmapMemory(RHIDeviceMemory* memory) = 0;
    };
}