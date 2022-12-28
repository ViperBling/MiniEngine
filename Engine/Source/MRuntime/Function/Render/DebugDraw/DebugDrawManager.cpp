#include "DebugDrawManager.h"
#include "Function/Global/GlobalContext.h"


namespace MiniEngine
{
    void DebugDrawManager::Initialize() {

        mRHI = gRuntimeGlobalContext.mRenderSystem->GetRHI();
        SetupPipelines();
    }

    void DebugDrawManager::SetupPipelines() {

        for (uint8_t i = 0; static_cast<DebugDrawPipelineType>(i) < DebugDrawPipelineType::count; i++) {
            mDebugDrawPipeline[i] = new DebugDrawPipeline(static_cast<DebugDrawPipelineType>(i));
            mDebugDrawPipeline[i]->Initialize();
        }
    }
}
