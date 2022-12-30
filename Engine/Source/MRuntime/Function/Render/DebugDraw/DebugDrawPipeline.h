#pragma once

#include "Function/Render/Interface/RHI.h"

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

    enum class DebugDrawPipelineType : uint8_t
    {
        triangle,
        count,
    };

    class DebugDrawPipeline
    {
    public:
        explicit DebugDrawPipeline(DebugDrawPipelineType pipelineType) { mPipelineType = pipelineType; }
        void Initialize();
        DebugDrawPipelineType GetPipelineType() const { return mPipelineType; }
        const DebugDrawFrameBuffer& GetFrameBuffer() const { return mFrameBuffer; }
        const DebugDrawPipelineBase& GetPipeline() const { return mRenderPipelines[0]; }

    private:
        void SetupRenderPass();
        void SetupPipelines();
        void SetupFrameBuffers();

    private:
        DebugDrawPipelineType mPipelineType;
        // RHIDescriptorSetLayout*            mDescriptorLayout; // 管线布局描述器
        std::vector<DebugDrawPipelineBase> mRenderPipelines; // 渲染管线
        DebugDrawFrameBuffer mFrameBuffer;
        std::shared_ptr<RHI>               mRHI;
    };
}