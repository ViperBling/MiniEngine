#pragma once

#include "DebugDrawBuffer.hpp"
#include "DebugDrawFont.hpp"
#include "DebugDrawContext.hpp"
#include "DebugDrawPipeline.hpp"
#include "MRuntime/Function/Render/Interface/RHI.hpp"
#include "MRuntime/Function/Render/RenderResource.hpp"

namespace MiniEngine
{
    class DebugDrawManager
    {
    public:
        void Initialize();
        void SetupPipelines();
        void PreparePassData(std::shared_ptr<RenderResourceBase> renderResource);
        void Destroy();
        void Clear();
        void Tick(float deltaTime);
        void UpdateAfterRecreateSwapChain();
        DebugDrawGroup* TryGetOrCreateDebugDrawGroup(const std::string& name);

        void Draw(uint32_t currentSwapChainImageIndex);
        ~DebugDrawManager() { Destroy(); }

    private:
        void swapDataToRender();
        void prepareDrawBuffer();
        void drawDebugObject(uint32_t currentSwapChainImageIndex);
        void drawWireFrameObject(uint32_t current_swapchain_image_index);
        void drawPointLineTriangleBox(uint32_t currentSwapChainImageIndex);

    private:
        std::mutex mMutex;
        std::shared_ptr<RHI> mRHI = nullptr;
        DebugDrawPipeline* mDebugDrawPipelines[DebugDrawPipelineType::DebugDrawPipelineTypeCount];
        DebugDrawAllocator* mBufferAllocator = nullptr;
        DebugDrawContext mDebugDrawContext;
        DebugDrawGroup mDebugDrawGroupForRender;
        DebugDrawFont* mFont = nullptr;

        Matrix4x4 mProjViewMatrix;
        
        size_t mPointStartOffset;
        size_t mPointEndOffset;
        size_t mLineStartOffset;
        size_t mLineEndOffset;
        size_t mTriangleStartOffset;
        size_t mTriangleEndOffset;
        size_t mNoDepthTestPointStartOffset;
        size_t mNoDepthTestPointEndOffset;
        size_t mNoDepthTestLineStartOffset;
        size_t mNoDepthTestLineEndOffset;
        size_t mNoDepthTestTriangleStartOffset;
        size_t mNoDepthTestTriangleEndOffset;
        size_t mTextStartOffset;
        size_t mTextEndOffset;
    };
}