#include "DebugDrawManager.hpp"
#include "MRuntime/Core/Math/MathHeaders.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Render/RenderSystem.hpp"

namespace MiniEngine
{
    void DebugDrawManager::Initialize()
    {
        mRHI = gRuntimeGlobalContext.mRenderSystem->GetRHI();
        SetupPipelines();
    }

    void DebugDrawManager::SetupPipelines()
    {
        //setup pipelines
        for (uint8_t i = 0; i < static_cast<uint8_t>(DebugDrawPipelineType::DebugDrawPipelineTypeCount); i++)
        {
            mDebugDrawPipelines[i] = new DebugDrawPipeline((DebugDrawPipelineType)i);
            mDebugDrawPipelines[i]->Initialize();
        }
        mBufferAllocator = new DebugDrawAllocator();
        mFont = new DebugDrawFont();
        mFont->Initialize();
        mBufferAllocator->Initialize(mFont);
    }

    void DebugDrawManager::PreparePassData(std::shared_ptr<RenderResourceBase> renderResource)
    {
        const RenderResource* resource = static_cast<const RenderResource*>(renderResource.get());
        mProjViewMatrix = resource->mMeshPerFrameStorageBufferObject.proj_view_matrix;
    }

    void DebugDrawManager::Destroy()
    {
        for (uint8_t i = 0; i < static_cast<uint8_t>(DebugDrawPipelineType::DebugDrawPipelineTypeCount); i++)
        {
            mDebugDrawPipelines[i]->Destory();
            delete mDebugDrawPipelines[i];
        }

        mBufferAllocator->Destory();
        delete mBufferAllocator;
        
        //mFont->destroy();
        delete mFont;
    }

    void DebugDrawManager::Clear()
    {
        std::lock_guard<std::mutex> guard(mMutex);
        mDebugDrawContext.Clear();
    }

    void DebugDrawManager::Tick(float deltaTime)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        mBufferAllocator->Tick();
        mDebugDrawContext.Tick(deltaTime);
    }

    void DebugDrawManager::UpdateAfterRecreateSwapChain()
    {
        for (uint8_t i = 0; i < static_cast<uint8_t>(DebugDrawPipelineType::DebugDrawPipelineTypeCount); i++)
        {
            mDebugDrawPipelines[i]->RecreateAfterSwapChain();
        }
    }

    DebugDrawGroup *DebugDrawManager::TryGetOrCreateDebugDrawGroup(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        return mDebugDrawContext.TryGetOrCreateDebugDrawGroup(name);
    }

    void DebugDrawManager::Draw(uint32_t currentSwapChainImageIndex)
    {
        static uint32_t once = 1;
        swapDataToRender();
        once = 0;

        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        mRHI->PushEvent(mRHI->GetCurrentCommandBuffer(), "DebugDrawManager", color);
        mRHI->CmdSetViewportPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().viewport);
        mRHI->CmdSetScissorPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().scissor);

        drawDebugObject(currentSwapChainImageIndex);

        mRHI->PopEvent(mRHI->GetCurrentCommandBuffer());
    }

    void DebugDrawManager::swapDataToRender()
    {
        std::lock_guard<std::mutex> guard(mMutex);

        mDebugDrawGroupForRender.Clear();
        size_t debug_draw_group_count = mDebugDrawContext.mDebugDrawGroup.size();
        for (size_t debug_draw_group_index = 0; debug_draw_group_index < debug_draw_group_count; debug_draw_group_index++)
        {
            DebugDrawGroup* debug_draw_group = mDebugDrawContext.mDebugDrawGroup[debug_draw_group_index];
            if (debug_draw_group == nullptr)continue;
            mDebugDrawGroupForRender.MergeFrom(debug_draw_group);
        }
    }

    void DebugDrawManager::prepareDrawBuffer()
    {
        mBufferAllocator->Clear();

        std::vector<DebugDrawVertex> vertexs;

        mDebugDrawGroupForRender.WritePointData(vertexs, false);
        mPointStartOffset = mBufferAllocator->CacheVertexs(vertexs);
        mPointEndOffset = mBufferAllocator->GetVertexCacheOffset();

        mDebugDrawGroupForRender.WriteLineData(vertexs, false);
        mLineStartOffset = mBufferAllocator->CacheVertexs(vertexs);
        mLineEndOffset = mBufferAllocator->GetVertexCacheOffset();

        mDebugDrawGroupForRender.WriteTriangleData(vertexs, false);
        mTriangleStartOffset = mBufferAllocator->CacheVertexs(vertexs);
        mTriangleEndOffset = mBufferAllocator->GetVertexCacheOffset();

        mDebugDrawGroupForRender.WritePointData(vertexs, true);
        mNoDepthTestPointStartOffset = mBufferAllocator->CacheVertexs(vertexs);
        mNoDepthTestPointEndOffset = mBufferAllocator->GetVertexCacheOffset();

        mDebugDrawGroupForRender.WriteLineData(vertexs, true);
        mNoDepthTestLineStartOffset = mBufferAllocator->CacheVertexs(vertexs);
        mNoDepthTestLineEndOffset = mBufferAllocator->GetVertexCacheOffset();

        mDebugDrawGroupForRender.WriteTriangleData(vertexs, true);
        mNoDepthTestTriangleStartOffset = mBufferAllocator->CacheVertexs(vertexs);
        mNoDepthTestTriangleEndOffset = mBufferAllocator->GetVertexCacheOffset();

        mDebugDrawGroupForRender.WriteTextData(vertexs, mFont, mProjViewMatrix);
        mTextStartOffset = mBufferAllocator->CacheVertexs(vertexs);
        mTextEndOffset = mBufferAllocator->GetVertexCacheOffset();

        mBufferAllocator->CacheUniformObject(mProjViewMatrix);

        std::vector<std::pair<Matrix4x4, Vector4> > dynamicObject = { std::make_pair(Matrix4x4::IDENTITY,Vector4(0,0,0,0)) };
        mBufferAllocator->CacheUniformDynamicObject(dynamicObject);//cache the first model matrix as Identity matrix, color as empty color. (default object)

        mDebugDrawGroupForRender.WriteUniformDynamicDataToCache(dynamicObject);
        mBufferAllocator->CacheUniformDynamicObject(dynamicObject);//cache the wire frame uniform dynamic object

        mBufferAllocator->Allocator();
    }

    void DebugDrawManager::drawDebugObject(uint32_t currentSwapChainImageIndex)
    {
        prepareDrawBuffer();
        drawPointLineTriangleBox(currentSwapChainImageIndex);
        drawWireFrameObject(currentSwapChainImageIndex);
    }

    void DebugDrawManager::drawWireFrameObject(uint32_t current_swapchain_image_index)
    {
        //draw wire frame object : sphere, cylinder, capsule
        
        std::vector<DebugDrawPipeline*>vc_pipelines{ mDebugDrawPipelines[DebugDrawPipelineType::Line],
                                                     mDebugDrawPipelines[DebugDrawPipelineType::LineNoDepthTest] };
        std::vector<bool>no_depth_tests = { false,true };

        for (int32_t i = 0; i < 2; i++)
        {
            bool no_depth_test = no_depth_tests[i];

            RHIDeviceSize offsets[] = { 0 };
            RHIClearValue clear_values[2];
            clear_values[0].color = { 0.0f,0.0f,0.0f,0.0f };
            clear_values[1].depthStencil = { 1.0f, 0 };
            RHIRenderPassBeginInfo renderpass_begin_info{};
            renderpass_begin_info.sType = RHI_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderpass_begin_info.renderArea.offset = { 0, 0 };
            renderpass_begin_info.renderArea.extent = mRHI->GetSwapChainInfo().extent;
            renderpass_begin_info.clearValueCount = (sizeof(clear_values) / sizeof(clear_values[0]));
            renderpass_begin_info.pClearValues = clear_values;
            renderpass_begin_info.renderPass = vc_pipelines[i]->GetFrameBuffer().renderPass;
            renderpass_begin_info.framebuffer = vc_pipelines[i]->GetFrameBuffer().framebuffers[current_swapchain_image_index];
            mRHI->CmdBeginRenderPassPFN(mRHI->GetCurrentCommandBuffer(), &renderpass_begin_info, RHI_SUBPASS_CONTENTS_INLINE);
            mRHI->CmdBindPipelinePFN(mRHI->GetCurrentCommandBuffer(), RHI_PIPELINE_BIND_POINT_GRAPHICS, vc_pipelines[i]->GetPipeline().pipeline);

            size_t uniform_dynamic_size = mBufferAllocator->GetSizeOfUniformBufferObject();
            uint32_t dynamicOffset = uniform_dynamic_size;

            size_t sphere_count = mDebugDrawGroupForRender.GetSphereCount(no_depth_test);
            size_t cylinder_count = mDebugDrawGroupForRender.GetCylinderCount(no_depth_test);
            size_t capsule_count = mDebugDrawGroupForRender.GetCapsuleCount(no_depth_test);

            if (sphere_count > 0)
            {
                RHIBuffer* sphere_vertex_buffers[] = { mBufferAllocator->GetSphereVertexBuffer() };
                mRHI->CmdBindVertexBuffersPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, sphere_vertex_buffers, offsets);
                for (size_t j = 0; j < sphere_count; j++)
                {
                    
                    mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                        vc_pipelines[i]->GetPipeline().layout,
                        0,
                        1,
                        &mBufferAllocator->GetDescriptorSet(),
                        1,
                        &dynamicOffset);
                    mRHI->CmdDraw(mRHI->GetCurrentCommandBuffer(), mBufferAllocator->GetSphereVertexBufferSize(), 1, 0, 0);
                    dynamicOffset += uniform_dynamic_size;
                }
            }

            if (cylinder_count > 0)
            {
                RHIBuffer* cylinder_vertex_buffers[] = { mBufferAllocator->GetCylinderVertexBuffer() };
                mRHI->CmdBindVertexBuffersPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, cylinder_vertex_buffers, offsets);
                for (size_t j = 0; j < cylinder_count; j++)
                {
                    mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                        vc_pipelines[i]->GetPipeline().layout,
                        0,
                        1,
                        &mBufferAllocator->GetDescriptorSet(),
                        1,
                        &dynamicOffset);
                    mRHI->CmdDraw(mRHI->GetCurrentCommandBuffer(), mBufferAllocator->GetCylinderVertexBufferSize(), 1, 0, 0);
                    dynamicOffset += uniform_dynamic_size;
                }
            }

            if (capsule_count > 0)
            {
                RHIBuffer* capsule_vertex_buffers[] = { mBufferAllocator->GetCapsuleVertexBuffer() };
                mRHI->CmdBindVertexBuffersPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, capsule_vertex_buffers, offsets);
                for (size_t j = 0; j < capsule_count; j++)
                {
                    //draw capsule up part
                    mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                        vc_pipelines[i]->GetPipeline().layout,
                        0,
                        1,
                        &mBufferAllocator->GetDescriptorSet(),
                        1,
                        &dynamicOffset);
                    mRHI->CmdDraw(mRHI->GetCurrentCommandBuffer(), mBufferAllocator->GetCapsuleVertexBufferUpSize(), 1, 0, 0);
                    dynamicOffset += uniform_dynamic_size;

                    //draw capsule mid part
                    mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                        vc_pipelines[i]->GetPipeline().layout,
                        0,
                        1,
                        &mBufferAllocator->GetDescriptorSet(),
                        1,
                        &dynamicOffset);
                    mRHI->CmdDraw(mRHI->GetCurrentCommandBuffer(), mBufferAllocator->GetCapsuleVertexBufferMidSize(), 1, mBufferAllocator->GetCapsuleVertexBufferUpSize(), 0);
                    dynamicOffset += uniform_dynamic_size;

                    //draw capsule down part
                    mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                        vc_pipelines[i]->GetPipeline().layout,
                        0,
                        1,
                        &mBufferAllocator->GetDescriptorSet(),
                        1,
                        &dynamicOffset);
                    mRHI->CmdDraw(mRHI->GetCurrentCommandBuffer(),
                        mBufferAllocator->GetCapsuleVertexBufferDownSize(),
                        1,
                        mBufferAllocator->GetCapsuleVertexBufferUpSize() + mBufferAllocator->GetCapsuleVertexBufferMidSize(),
                        0);
                    dynamicOffset += uniform_dynamic_size;
                }
            }

            mRHI->CmdEndRenderPassPFN(mRHI->GetCurrentCommandBuffer());
        }
    }

    void DebugDrawManager::drawPointLineTriangleBox(uint32_t currentSwapChainImageIndex)
    {
        // draw point, line ,triangle , triangle_without_depth_test
        RHIBuffer* vertex_buffers[] = { mBufferAllocator->GetVertexBuffer() };
        if (vertex_buffers[0] == nullptr)
        {
            return;
        }
        RHIDeviceSize offsets[] = { 0 };
        mRHI->CmdBindVertexBuffersPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, vertex_buffers, offsets);

        std::vector<DebugDrawPipeline*>vc_pipelines{ mDebugDrawPipelines[DebugDrawPipelineType::Point],
                                                     mDebugDrawPipelines[DebugDrawPipelineType::Line],
                                                     mDebugDrawPipelines[DebugDrawPipelineType::Triangle],
                                                     mDebugDrawPipelines[DebugDrawPipelineType::PointNoDepthTest],
                                                     mDebugDrawPipelines[DebugDrawPipelineType::LineNoDepthTest],
                                                     mDebugDrawPipelines[DebugDrawPipelineType::TriangleNoDepthTest],
                                                     mDebugDrawPipelines[DebugDrawPipelineType::TriangleNoDepthTest] };
        std::vector<size_t>vc_start_offsets{ mPointStartOffset,
                                             mLineStartOffset,
                                             mTriangleStartOffset,
                                             mNoDepthTestPointStartOffset,
                                             mNoDepthTestLineStartOffset,
                                             mNoDepthTestTriangleStartOffset,
                                             mTextStartOffset };
        std::vector<size_t>vc_end_offsets{ mPointEndOffset,
                                           mLineEndOffset,
                                           mTriangleEndOffset,
                                           mNoDepthTestPointEndOffset,
                                           mNoDepthTestLineEndOffset,
                                           mNoDepthTestTriangleEndOffset,
                                           mTextEndOffset };
        RHIClearValue clear_values[2];
        clear_values[0].color = { 0.0f,0.0f,0.0f,0.0f };
        clear_values[1].depthStencil = { 1.0f, 0 };
        RHIRenderPassBeginInfo renderpass_begin_info{};
        renderpass_begin_info.sType = RHI_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderpass_begin_info.renderArea.offset = { 0, 0 };
        renderpass_begin_info.renderArea.extent = mRHI->GetSwapChainInfo().extent;
        renderpass_begin_info.clearValueCount = (sizeof(clear_values) / sizeof(clear_values[0]));
        renderpass_begin_info.pClearValues = clear_values;

        for (size_t i = 0; i < vc_pipelines.size(); i++)
        {
            if (vc_end_offsets[i] - vc_start_offsets[i] == 0)
            {
                continue;
            }
            renderpass_begin_info.renderPass = vc_pipelines[i]->GetFrameBuffer().renderPass;
            renderpass_begin_info.framebuffer = vc_pipelines[i]->GetFrameBuffer().framebuffers[currentSwapChainImageIndex];
            mRHI->CmdBeginRenderPassPFN(mRHI->GetCurrentCommandBuffer(), &renderpass_begin_info, RHI_SUBPASS_CONTENTS_INLINE);

            mRHI->CmdBindPipelinePFN(mRHI->GetCurrentCommandBuffer(), RHI_PIPELINE_BIND_POINT_GRAPHICS, vc_pipelines[i]->GetPipeline().pipeline);

            uint32_t dynamicOffset = 0;
            mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                RHI_PIPELINE_BIND_POINT_GRAPHICS,
                vc_pipelines[i]->GetPipeline().layout,
                0,
                1,
                &mBufferAllocator->GetDescriptorSet(),
                1,
                &dynamicOffset);
            mRHI->CmdDraw(mRHI->GetCurrentCommandBuffer(), vc_end_offsets[i] - vc_start_offsets[i], 1, vc_start_offsets[i], 0);

            mRHI->CmdEndRenderPassPFN(mRHI->GetCurrentCommandBuffer());
        }
    }
}
