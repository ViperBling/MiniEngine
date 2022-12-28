#pragma once

#include <cstdint>

#include "DebugDrawPipeline.h"
#include "Function/Render/Interface/RHI.h"

namespace MiniEngine
{
    class DebugDrawManager
    {
    public:
        void Initialize();
        void SetupPipelines();

    private:
        std::shared_ptr<RHI> mRHI {nullptr};
        DebugDrawPipeline* mDebugDrawPipeline[static_cast<uint32_t>(DebugDrawPipelineType::count)] = {};
    };
}