#pragma once

#include <cstdint>

#include "DebugDrawBuffer.hpp"
#include "DebugDrawGroup.hpp"
#include "DebugDrawPipeline.hpp"
#include "Function/Render/Interface/RHI.hpp"

namespace MiniEngine
{
    class DebugDrawManager
    {
    public:
        void Initialize();
        void SetupPipelines();
        void UpdateAfterRecreateSwapChain();

        void Draw(uint32_t currentSwapChainImageIndex);

    private:
        void prepareDrawBuffer();
        void DrawDebugObject(uint32_t currentSwapChainImageIndex);
        void DrawPointLineTriangleBox(uint32_t currentSwapChainImageIndex);

    private:
        std::shared_ptr<RHI> mRHI {nullptr};
        DebugDrawPipeline* mDebugDrawPipeline[static_cast<uint32_t>(DebugDrawPipelineType::Count)] = {};
        DebugDrawAllocator* mBufferAllocator = nullptr;
        DebugDrawGroup mDebugDrawGroupForRender;

        size_t mTriangleStartOffset;
        size_t mTriangleEndOffset;
    };
}