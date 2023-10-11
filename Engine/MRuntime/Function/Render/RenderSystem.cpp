#include "RenderSystem.hpp"
#include "Function/Render/Interface/RHI.hpp"
#include "Function/Render/Interface/Vulkan/VulkanRHI.hpp"
#include "Function/Render/RenderPipeline.hpp"

namespace MiniEngine
{
    RenderSystem::~RenderSystem()
    {
        Clear();
    }

    void RenderSystem::Initialize(RenderSystemInitInfo initInfo) {

        RHIInitInfo rhiInitInfo;
        rhiInitInfo.windowSystem = initInfo.mWindowSystem;

        mRHI = std::make_shared<VulkanRHI>();
        mRHI->Initialize(rhiInitInfo);

        // Initialize Render Pipeline
        mRenderPipeline = std::make_shared<RenderPipeline>();
        mRenderPipeline->mRHI = mRHI;
    }

    void RenderSystem::Tick(float DeltaTime) 
    {
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

    void RenderSystem::Clear()
    {
        if (mRHI)
        {
            mRHI->Clear();
        }
        mRHI.reset();

        if (mRenderScene)
        {
            mRenderScene->Clear();
        }
        mRenderScene.reset();

        if (mRenderResource)
        {
            mRenderResource->Clear();
        }
        mRenderResource.reset();
        
        if (mRenderPipeline)
        {
            mRenderPipeline->Clear();
        }
        mRenderPipeline.reset();
    }
}