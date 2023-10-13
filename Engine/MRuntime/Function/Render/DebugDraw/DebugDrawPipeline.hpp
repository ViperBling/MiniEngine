#pragma once

#include "Function/Render/Interface/RHI.hpp"

namespace MiniEngine
{
    struct DebugDrawFrameBufferAttachment
    {
        RHIImage*        image = nullptr;
        RHIDeviceMemory* mem   = nullptr;
        RHIImageView*    view  = nullptr;
        RHIFormat        format;
    };

    struct DebugDrawFrameBuffer
    {
        int            width;
        int            height;
        RHIRenderPass* renderPass = nullptr;

        std::vector<RHIFrameBuffer*>                framebuffers;
        std::vector<DebugDrawFrameBufferAttachment> attachments;
    };

    struct DebugDrawPipelineBase
    {
        RHIPipelineLayout* layout = nullptr;
        RHIPipeline*       pipeline = nullptr;
    };

    enum DebugDrawPipelineType : uint8_t
    {
        Point = 0,
        Line,
        Triangle,
        PointNoDepthTest,
        LineNoDepthTest,
        TriangleNoDepthTest,
        Count
    };

    class DebugDrawPipeline
    {
    public:
        explicit DebugDrawPipeline(DebugDrawPipelineType pipelineType) { mPipelineType = pipelineType; }
        void Initialize();
        void Destory();
        void RecreateAfterSwapChain();

        DebugDrawPipelineType GetPipelineType() const { return mPipelineType; }
        const DebugDrawFrameBuffer& GetFrameBuffer() const { return mFrameBuffer; }
        const DebugDrawPipelineBase& GetPipeline() const { return mRenderPipelines[0]; }

    private:
        void setupAttachments();
        void setupFrameBuffer();
        void setupRenderPass();
        void setupDescriptorLayout();
        void setupPipelines();
    
    public:
        DebugDrawPipelineType               mPipelineType;

    private:    
        RHIDescriptorSetLayout*             mDescriptorLayout; // 管线布局描述器
        std::vector<DebugDrawPipelineBase>  mRenderPipelines; // 渲染管线
        DebugDrawFrameBuffer                mFrameBuffer;
        std::shared_ptr<RHI>                mRHI;
    };
}