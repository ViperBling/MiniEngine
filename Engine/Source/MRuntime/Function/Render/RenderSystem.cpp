#include "RenderSystem.h"
#include "Function/Render/Interface/RHI.h"
#include "Function/Render/Interface/Vulkan/VulkanRHI.h"
#include "Function/Render/RenderPipeline.h"

namespace MiniEngine
{
    void RenderSystem::Initialize(RenderSystemInitInfo initInfo) {

        RHIInitInfo rhiInitInfo;
        rhiInitInfo.windowSystem = initInfo.mWindowSystem;

        mRHI = std::make_shared<VulkanRHI>();
        mRHI->Initialize(rhiInitInfo);

        // Initialize Render Pipeline
        mRenderPipeline = std::make_shared<RenderPipeline>();
        mRenderPipeline->mRHI = mRHI;
    }

    void RenderSystem::Tick(float DeltaTime) {

        mRHI->PrepareContext();

        switch (mRenderPipelineType)
        {
            case RENDER_PIPELINE_TYPE::FORWARD_PIPELINE :
                mRenderPipeline->ForwardRender(mRHI, mRenderResource);
                break;

            case RENDER_PIPELINE_TYPE::DEFERRED_PIPELINE:
            case RENDER_PIPELINE_TYPE::PIPELINE_TYPE_COUNT:
                break;
        }
    }
}