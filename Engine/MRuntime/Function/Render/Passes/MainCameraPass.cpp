#include "MainCameraPass.hpp"

namespace MiniEngine
{
    void MainCameraPass::Initialize(const RenderPassInitInfo *initInfo)
    {
        RenderPass::Initialize(nullptr);

        const MainCameraPassInitInfo* _init_info = static_cast<const MainCameraPassInitInfo*>(initInfo);
        mbEnableFXAA                            = _init_info->mbEnableFXAA;

        setupAttachments();
        setupRenderPass();
        setupDescriptorSetLayout();
        setupPipelines();
        setupDescriptorSet();
        setupFrameBufferDescriptorSet();
        setupSwapChainFrameBuffers();
    }

    void MainCameraPass::PreparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        const RenderResource* resource = static_cast<const RenderResource*>(render_resource.get());
        if (resource)
        {
            mPerFrameStorageBufferObject = resource->mMeshPerFrameStorageBufferObject;
            mAxisStorageBufferObject     = resource->mAxisStorageBufferObject;
        }
    }

    void MainCameraPass::DrawDeferred(
        ColorGradientPass &colorGradientPass, 
        ToneMappingPass &toneMappingPass, 
        UIPass &uiPass, 
        CombineUIPass &combineUIPass, 
        uint32_t currentSwapChaingIndex)
    {
        RHICommandBuffer* cmdBuffer = mRHI->GetCurrentCommandBuffer();
        {
            RHIRenderPassBeginInfo rpBI {};
            rpBI.renderPass = mFrameBuffer.renderPass;
            rpBI.framebuffer = mSwapChainFrameBuffers[currentSwapChaingIndex];
            rpBI.renderArea.offset = {0, 0};
            rpBI.renderArea.extent = mRHI->GetSwapChainInfo().extent;

            RHIClearValue clearValue[MAIN_CAMERA_PASS_ATTACHMENT_COUNT];
            clearValue[MAIN_CAMERA_PASS_GBUFFER_A].color                = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValue[MAIN_CAMERA_PASS_GBUFFER_B].color                = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValue[MAIN_CAMERA_PASS_GBUFFER_C].color                = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValue[MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValue[MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN].color       = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValue[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_ODD].color  = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValue[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_EVEN].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValue[MAIN_CAMERA_PASS_DEPTH].depthStencil             = {1.0f, 0};
            clearValue[MAIN_CAMERA_PASS_SWAP_CHAIN_IMAGE].color         = {{0.0f, 0.0f, 0.0f, 1.0f}};
            rpBI.clearValueCount                                        = (sizeof(clearValue) / sizeof(clearValue[0]));
            rpBI.pClearValues                                           = clearValue;

            mRHI->CmdBeginRenderPassPFN(cmdBuffer, &rpBI, RHI_SUBPASS_CONTENTS_INLINE);
        }

        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        mRHI->PushEvent(cmdBuffer, "BasePass", color);
        drawMeshGBuffer();
        mRHI->PopEvent(cmdBuffer);

        mRHI->CmdNextSubpassPFN(cmdBuffer, RHI_SUBPASS_CONTENTS_INLINE);
        mRHI->PushEvent(cmdBuffer, "Deferred Lighting", color);
        drawDeferredLighting();
        mRHI->PopEvent(cmdBuffer);

        mRHI->CmdNextSubpassPFN(cmdBuffer, RHI_SUBPASS_CONTENTS_INLINE);
        toneMappingPass.Draw();

        mRHI->CmdNextSubpassPFN(cmdBuffer, RHI_SUBPASS_CONTENTS_INLINE);
        colorGradientPass.Draw();

        mRHI->CmdNextSubpassPFN(cmdBuffer, RHI_SUBPASS_CONTENTS_INLINE);
        RHIClearAttachment clearAttachs[1];
        clearAttachs[0].aspectMask                  = RHI_IMAGE_ASPECT_COLOR_BIT;
        clearAttachs[0].colorAttachment             = 0;
        clearAttachs[0].clearValue.color.float32[0] = 0.0;
        clearAttachs[0].clearValue.color.float32[1] = 0.0;
        clearAttachs[0].clearValue.color.float32[2] = 0.0;
        clearAttachs[0].clearValue.color.float32[3] = 0.0;
        RHIClearRect clearRects[1];
        clearRects[0].baseArrayLayer     = 0;
        clearRects[0].layerCount         = 1;
        clearRects[0].rect.offset.x      = 0;
        clearRects[0].rect.offset.y      = 0;
        clearRects[0].rect.extent.width  = mRHI->GetSwapChainInfo().extent.width;
        clearRects[0].rect.extent.height = mRHI->GetSwapChainInfo().extent.height;
        mRHI->CmdClearAttachmentsPFN(
            cmdBuffer,
            1,
            clearAttachs,
            1,
            clearRects);
        drawAxis();
        uiPass.Draw();

        mRHI->CmdNextSubpassPFN(cmdBuffer, RHI_SUBPASS_CONTENTS_INLINE);
        combineUIPass.Draw();

        mRHI->CmdEndRenderPassPFN(cmdBuffer);
    }

    void MainCameraPass::DrawForward(
        ColorGradientPass &colorGradientPass, 
        ToneMappingPass &toneMappingPass, 
        UIPass &uiPass, 
        CombineUIPass &combineUIPass, 
        uint32_t currentSwapChaingIndex)
    {
        RHICommandBuffer* cmdBuffer = mRHI->GetCurrentCommandBuffer();
        {
            RHIRenderPassBeginInfo rpBI {};
            rpBI.renderPass = mFrameBuffer.renderPass;
            rpBI.framebuffer = mSwapChainFrameBuffers[currentSwapChaingIndex];
            rpBI.renderArea.offset = {0, 0};
            rpBI.renderArea.extent = mRHI->GetSwapChainInfo().extent;

            RHIClearValue clearValue[MAIN_CAMERA_PASS_ATTACHMENT_COUNT];
            clearValue[MAIN_CAMERA_PASS_GBUFFER_A].color                = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValue[MAIN_CAMERA_PASS_GBUFFER_B].color                = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValue[MAIN_CAMERA_PASS_GBUFFER_C].color                = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValue[MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValue[MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN].color       = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValue[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_ODD].color  = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValue[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_EVEN].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValue[MAIN_CAMERA_PASS_DEPTH].depthStencil             = {1.0f, 0};
            clearValue[MAIN_CAMERA_PASS_SWAP_CHAIN_IMAGE].color         = {{0.0f, 0.0f, 0.0f, 1.0f}};
            rpBI.clearValueCount                                        = (sizeof(clearValue) / sizeof(clearValue[0]));
            rpBI.pClearValues                                           = clearValue;

            mRHI->CmdBeginRenderPassPFN(cmdBuffer, &rpBI, RHI_SUBPASS_CONTENTS_INLINE);
        }

        mRHI->CmdNextSubpassPFN(cmdBuffer, RHI_SUBPASS_CONTENTS_INLINE);
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        mRHI->PushEvent(cmdBuffer, "Forward Lighting", color);
        drawMeshLighting();
        drawSkybox();
        mRHI->PopEvent(cmdBuffer);

        mRHI->CmdNextSubpassPFN(cmdBuffer, RHI_SUBPASS_CONTENTS_INLINE);
        toneMappingPass.Draw();

        mRHI->CmdNextSubpassPFN(cmdBuffer, RHI_SUBPASS_CONTENTS_INLINE);
        colorGradientPass.Draw();

        mRHI->CmdNextSubpassPFN(cmdBuffer, RHI_SUBPASS_CONTENTS_INLINE);
        RHIClearAttachment clearAttachs[1];
        clearAttachs[0].aspectMask                  = RHI_IMAGE_ASPECT_COLOR_BIT;
        clearAttachs[0].colorAttachment             = 0;
        clearAttachs[0].clearValue.color.float32[0] = 0.0;
        clearAttachs[0].clearValue.color.float32[1] = 0.0;
        clearAttachs[0].clearValue.color.float32[2] = 0.0;
        clearAttachs[0].clearValue.color.float32[3] = 0.0;
        RHIClearRect clearRects[1];
        clearRects[0].baseArrayLayer     = 0;
        clearRects[0].layerCount         = 1;
        clearRects[0].rect.offset.x      = 0;
        clearRects[0].rect.offset.y      = 0;
        clearRects[0].rect.extent.width  = mRHI->GetSwapChainInfo().extent.width;
        clearRects[0].rect.extent.height = mRHI->GetSwapChainInfo().extent.height;
        mRHI->CmdClearAttachmentsPFN(
            cmdBuffer,
            1,
            clearAttachs,
            1,
            clearRects);
        drawAxis();
        uiPass.Draw();

        mRHI->CmdNextSubpassPFN(cmdBuffer, RHI_SUBPASS_CONTENTS_INLINE);
        combineUIPass.Draw();

        mRHI->CmdEndRenderPassPFN(cmdBuffer);
    }

    void MainCameraPass::CopyNormalAndDepthImage()
    {
    }

    void MainCameraPass::UpdateAfterFramebufferRecreate()
    {
        for (size_t i = 0; i < mFrameBuffer.attachments.size(); i++)
        {
            mRHI->DestroyImage(mFrameBuffer.attachments[i].image);
            mRHI->DestroyImageView(mFrameBuffer.attachments[i].view);
            mRHI->FreeMemory(mFrameBuffer.attachments[i].mem);
        }
        for (auto framebuffer : mSwapChainFrameBuffers)
        {
            mRHI->DestroyFrameBuffer(framebuffer);
        }
        setupAttachments();
        setupFrameBufferDescriptorSet();
        setupSwapChainFrameBuffers();
    }

    RHICommandBuffer *MainCameraPass::GetRenderCommandBuffer()
    {
        return mRHI->GetCurrentCommandBuffer();
    }

    void MainCameraPass::setupAttachments()
    {
        mFrameBuffer.attachments.resize(MAIN_CAMERA_PASS_CUSTOM_ATTACHMENT_COUNT + MAIN_CAMERA_PASS_POST_PROCESS_ATTACHMENT_COUNT);

        mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_A].format          = RHI_FORMAT_R8G8B8A8_UNORM;
        mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_B].format          = RHI_FORMAT_R8G8B8A8_UNORM;
        mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_C].format          = RHI_FORMAT_R8G8B8A8_SRGB;
        mFrameBuffer.attachments[MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD].format  = RHI_FORMAT_R16G16B16A16_SFLOAT;
        mFrameBuffer.attachments[MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN].format = RHI_FORMAT_R16G16B16A16_SFLOAT;

        for (int buffer_index = 0; buffer_index < MAIN_CAMERA_PASS_CUSTOM_ATTACHMENT_COUNT; ++buffer_index)
        {
            if (buffer_index == MAIN_CAMERA_PASS_GBUFFER_A)
            {
                mRHI->CreateImage(
                    mRHI->GetSwapChainInfo().extent.width,
                    mRHI->GetSwapChainInfo().extent.height,
                    mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_A].format,
                    RHI_IMAGE_TILING_OPTIMAL,
                    RHI_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | RHI_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    RHI_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_A].image,
                    mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_A].mem,
                    0,
                    1,
                    1);
            }
            else
            {
                mRHI->CreateImage(
                    mRHI->GetSwapChainInfo().extent.width,
                    mRHI->GetSwapChainInfo().extent.height,
                    mFrameBuffer.attachments[buffer_index].format,
                    RHI_IMAGE_TILING_OPTIMAL,
                    RHI_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | RHI_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                    RHI_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                    RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    mFrameBuffer.attachments[buffer_index].image,
                    mFrameBuffer.attachments[buffer_index].mem,
                    0,
                    1,
                    1);
            }
            mRHI->CreateImageView(
                mFrameBuffer.attachments[buffer_index].image,
                mFrameBuffer.attachments[buffer_index].format,
                RHI_IMAGE_ASPECT_COLOR_BIT,
                RHI_IMAGE_VIEW_TYPE_2D,
                1,
                1,
                mFrameBuffer.attachments[buffer_index].view);
        }

        mFrameBuffer.attachments[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_ODD].format  = RHI_FORMAT_R16G16B16A16_SFLOAT;
        mFrameBuffer.attachments[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_EVEN].format = RHI_FORMAT_R16G16B16A16_SFLOAT;
        for (int attachment_index = MAIN_CAMERA_PASS_CUSTOM_ATTACHMENT_COUNT;
             attachment_index <
             MAIN_CAMERA_PASS_CUSTOM_ATTACHMENT_COUNT + MAIN_CAMERA_PASS_POST_PROCESS_ATTACHMENT_COUNT;
             ++attachment_index)
        {
            mRHI->CreateImage(
                mRHI->GetSwapChainInfo().extent.width,
                mRHI->GetSwapChainInfo().extent.height,
                mFrameBuffer.attachments[attachment_index].format,
                RHI_IMAGE_TILING_OPTIMAL,
                RHI_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | RHI_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                RHI_IMAGE_USAGE_SAMPLED_BIT,
                RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                mFrameBuffer.attachments[attachment_index].image,
                mFrameBuffer.attachments[attachment_index].mem,
                0,
                1,
                1);

            mRHI->CreateImageView(
                mFrameBuffer.attachments[attachment_index].image,
                mFrameBuffer.attachments[attachment_index].format,
                RHI_IMAGE_ASPECT_COLOR_BIT,
                RHI_IMAGE_VIEW_TYPE_2D,
                1,
                1,
                mFrameBuffer.attachments[attachment_index].view);
        }
    }

    void MainCameraPass::setupRenderPass()
    {
        RHIAttachmentDescription attachments[MAIN_CAMERA_PASS_ATTACHMENT_COUNT] = {};

        RHIAttachmentDescription& gbufferNormalAttacDesc = attachments[MAIN_CAMERA_PASS_GBUFFER_A];
        gbufferNormalAttacDesc.format         = mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_A].format;
        gbufferNormalAttacDesc.samples        = RHI_SAMPLE_COUNT_1_BIT;
        gbufferNormalAttacDesc.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        gbufferNormalAttacDesc.storeOp        = RHI_ATTACHMENT_STORE_OP_STORE;
        gbufferNormalAttacDesc.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        gbufferNormalAttacDesc.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        gbufferNormalAttacDesc.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        gbufferNormalAttacDesc.finalLayout    = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentDescription& gbufferMetalicRoughAttachDesc = attachments[MAIN_CAMERA_PASS_GBUFFER_B];
        gbufferMetalicRoughAttachDesc.format         = mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_B].format;
        gbufferMetalicRoughAttachDesc.samples        = RHI_SAMPLE_COUNT_1_BIT;
        gbufferMetalicRoughAttachDesc.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        gbufferMetalicRoughAttachDesc.storeOp        = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        gbufferMetalicRoughAttachDesc.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        gbufferMetalicRoughAttachDesc.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        gbufferMetalicRoughAttachDesc.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        gbufferMetalicRoughAttachDesc.finalLayout    = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentDescription& gbufferAlbedoAttachDesc = attachments[MAIN_CAMERA_PASS_GBUFFER_C];
        gbufferAlbedoAttachDesc.format         = mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_C].format;
        gbufferAlbedoAttachDesc.samples        = RHI_SAMPLE_COUNT_1_BIT;
        gbufferAlbedoAttachDesc.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        gbufferAlbedoAttachDesc.storeOp        = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        gbufferAlbedoAttachDesc.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        gbufferAlbedoAttachDesc.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        gbufferAlbedoAttachDesc.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        gbufferAlbedoAttachDesc.finalLayout    = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentDescription& backupOddColorAttachDesc = attachments[MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD];
        backupOddColorAttachDesc.format         = mFrameBuffer.attachments[MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD].format;
        backupOddColorAttachDesc.samples        = RHI_SAMPLE_COUNT_1_BIT;
        backupOddColorAttachDesc.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        backupOddColorAttachDesc.storeOp        = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        backupOddColorAttachDesc.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        backupOddColorAttachDesc.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        backupOddColorAttachDesc.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        backupOddColorAttachDesc.finalLayout    = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentDescription& backupEvenColorAttachDesc = attachments[MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN];
        backupEvenColorAttachDesc.format         = mFrameBuffer.attachments[MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN].format;
        backupEvenColorAttachDesc.samples        = RHI_SAMPLE_COUNT_1_BIT;
        backupEvenColorAttachDesc.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        backupEvenColorAttachDesc.storeOp        = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        backupEvenColorAttachDesc.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        backupEvenColorAttachDesc.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        backupEvenColorAttachDesc.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        backupEvenColorAttachDesc.finalLayout    = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentDescription& postProcOddColorAttachDesc = attachments[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_ODD];
        postProcOddColorAttachDesc.format         = mFrameBuffer.attachments[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_ODD].format;
        postProcOddColorAttachDesc.samples        = RHI_SAMPLE_COUNT_1_BIT;
        postProcOddColorAttachDesc.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        postProcOddColorAttachDesc.storeOp        = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        postProcOddColorAttachDesc.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        postProcOddColorAttachDesc.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        postProcOddColorAttachDesc.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        postProcOddColorAttachDesc.finalLayout    = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentDescription& postProcEvenColorAttachDesc = attachments[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_EVEN];
        postProcEvenColorAttachDesc.format         = mFrameBuffer.attachments[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_EVEN].format;
        postProcEvenColorAttachDesc.samples        = RHI_SAMPLE_COUNT_1_BIT;
        postProcEvenColorAttachDesc.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        postProcEvenColorAttachDesc.storeOp        = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        postProcEvenColorAttachDesc.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        postProcEvenColorAttachDesc.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        postProcEvenColorAttachDesc.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        postProcEvenColorAttachDesc.finalLayout    = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentDescription& depthAttachDesc = attachments[MAIN_CAMERA_PASS_DEPTH];
        depthAttachDesc.format         = mRHI->GetDepthImageInfo().depthImageFormat;
        depthAttachDesc.samples        = RHI_SAMPLE_COUNT_1_BIT;
        depthAttachDesc.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachDesc.storeOp        = RHI_ATTACHMENT_STORE_OP_STORE;
        depthAttachDesc.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachDesc.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachDesc.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        depthAttachDesc.finalLayout    = RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        RHIAttachmentDescription& swapChainAttachDesc = attachments[MAIN_CAMERA_PASS_SWAP_CHAIN_IMAGE];
        swapChainAttachDesc.format         = mRHI->GetSwapChainInfo().imageFormat;
        swapChainAttachDesc.samples        = RHI_SAMPLE_COUNT_1_BIT;
        swapChainAttachDesc.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        swapChainAttachDesc.storeOp        = RHI_ATTACHMENT_STORE_OP_STORE;
        swapChainAttachDesc.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        swapChainAttachDesc.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        swapChainAttachDesc.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        swapChainAttachDesc.finalLayout    = RHI_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        RHISubpassDescription subpasses[MAIN_CAMERA_SUBPASS_COUNT] {};

        RHIAttachmentReference basePassAttachRef[3] {};
        basePassAttachRef[0].attachment = &gbufferNormalAttacDesc - attachments;
        basePassAttachRef[0].layout     = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        basePassAttachRef[1].attachment = &gbufferMetalicRoughAttachDesc - attachments;
        basePassAttachRef[1].layout     = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        basePassAttachRef[2].attachment = &gbufferAlbedoAttachDesc - attachments;
        basePassAttachRef[2].layout     = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        RHIAttachmentReference basePassDepthAttachRef {};
        basePassDepthAttachRef.attachment = &depthAttachDesc - attachments;
        basePassDepthAttachRef.layout     = RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        RHISubpassDescription& base_pass = subpasses[MAIN_CAMERA_SUBPASS_BASEPASS];
        base_pass.pipelineBindPoint      = RHI_PIPELINE_BIND_POINT_GRAPHICS;
        base_pass.colorAttachmentCount   = sizeof(basePassAttachRef) / sizeof(basePassAttachRef[0]);
        base_pass.pColorAttachments       = &basePassAttachRef[0];
        base_pass.pDepthStencilAttachment = &basePassDepthAttachRef;
        base_pass.preserveAttachmentCount = 0;
        base_pass.pPreserveAttachments    = nullptr;

        RHIAttachmentReference deferredPassInputAttachRef[4] = {};
        deferredPassInputAttachRef[0].attachment = &gbufferNormalAttacDesc - attachments;
        deferredPassInputAttachRef[0].layout     = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        deferredPassInputAttachRef[1].attachment = &gbufferMetalicRoughAttachDesc - attachments;
        deferredPassInputAttachRef[1].layout     = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        deferredPassInputAttachRef[2].attachment = &gbufferAlbedoAttachDesc - attachments;
        deferredPassInputAttachRef[2].layout     = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        deferredPassInputAttachRef[3].attachment = &depthAttachDesc - attachments;
        deferredPassInputAttachRef[3].layout     = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentReference deferredPassColorAttach[1] = {};
        deferredPassColorAttach[0].attachment = &backupOddColorAttachDesc - attachments;
        deferredPassColorAttach[0].layout = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        RHISubpassDescription& deferredPassDesc  = subpasses[MAIN_CAMERA_SUBPASS_DEFERRED_LIGHTING];
        deferredPassDesc.pipelineBindPoint       = RHI_PIPELINE_BIND_POINT_GRAPHICS;
        deferredPassDesc.inputAttachmentCount    = sizeof(deferredPassInputAttachRef) / sizeof(deferredPassInputAttachRef[0]);
        deferredPassDesc.pInputAttachments       = &deferredPassInputAttachRef[0];
        deferredPassDesc.colorAttachmentCount    = sizeof(deferredPassColorAttach) / sizeof(deferredPassColorAttach[0]);
        deferredPassDesc.pColorAttachments       = &deferredPassColorAttach[0];
        deferredPassDesc.pDepthStencilAttachment = nullptr;
        deferredPassDesc.preserveAttachmentCount = 0;
        deferredPassDesc.pPreserveAttachments    = nullptr;

        RHIAttachmentReference forwardColorAttachRef[1] = {};
        forwardColorAttachRef[0].attachment = &backupOddColorAttachDesc - attachments;
        forwardColorAttachRef[0].layout = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        RHIAttachmentReference forwardPassDepthAttachRef {};
        forwardPassDepthAttachRef.attachment = &depthAttachDesc - attachments;
        forwardPassDepthAttachRef.layout     = RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        RHISubpassDescription& forwardPass  = subpasses[MAIN_CAMERA_SUBPASS_FORWARD_LIGHTING];
        forwardPass.pipelineBindPoint       = RHI_PIPELINE_BIND_POINT_GRAPHICS;
        forwardPass.inputAttachmentCount    = 0;
        forwardPass.pInputAttachments       = nullptr;
        forwardPass.colorAttachmentCount    = sizeof(forwardColorAttachRef) / sizeof(forwardColorAttachRef[0]);
        forwardPass.pColorAttachments       = &forwardColorAttachRef[0];
        forwardPass.pDepthStencilAttachment = &forwardPassDepthAttachRef;
        forwardPass.preserveAttachmentCount = 0;
        forwardPass.pPreserveAttachments    = nullptr;

        RHIAttachmentReference tonemappingPassInputAttachRef {};
        tonemappingPassInputAttachRef.attachment = &backupOddColorAttachDesc - attachments;
        tonemappingPassInputAttachRef.layout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentReference tonemappingColorAttacheRef {};
        tonemappingColorAttacheRef.attachment = &backupEvenColorAttachDesc - attachments;
        tonemappingColorAttacheRef.layout = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        RHISubpassDescription& tonemappingPass  = subpasses[MAIN_CAMERA_SUBPASS_TONE_MAPPING];
        tonemappingPass.pipelineBindPoint       = RHI_PIPELINE_BIND_POINT_GRAPHICS;
        tonemappingPass.inputAttachmentCount    = 1;
        tonemappingPass.pInputAttachments       = &tonemappingPassInputAttachRef;
        tonemappingPass.colorAttachmentCount    = 1;
        tonemappingPass.pColorAttachments       = &tonemappingColorAttacheRef;
        tonemappingPass.pDepthStencilAttachment = nullptr;
        tonemappingPass.preserveAttachmentCount = 0;
        tonemappingPass.pPreserveAttachments    = nullptr;

        RHIAttachmentReference colorGradientInputAttachRef {};
        colorGradientInputAttachRef.attachment = &backupEvenColorAttachDesc - attachments;
        colorGradientInputAttachRef.layout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentReference colorGradientColorAttachRef {};
        colorGradientColorAttachRef.attachment = &backupOddColorAttachDesc - attachments;
        colorGradientColorAttachRef.layout = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        RHISubpassDescription& colorGradientPass  = subpasses[MAIN_CAMERA_SUBPASS_COLOR_GRADIENT];
        colorGradientPass.pipelineBindPoint       = RHI_PIPELINE_BIND_POINT_GRAPHICS;
        colorGradientPass.inputAttachmentCount    = 1;
        colorGradientPass.pInputAttachments       = &colorGradientInputAttachRef;
        colorGradientPass.colorAttachmentCount    = 1;
        colorGradientPass.pColorAttachments       = &colorGradientColorAttachRef;
        colorGradientPass.pDepthStencilAttachment = nullptr;
        colorGradientPass.preserveAttachmentCount = 0;
        colorGradientPass.pPreserveAttachments    = nullptr;

        RHIAttachmentReference uiPassColorAttachRef {};
        uiPassColorAttachRef.attachment = &backupEvenColorAttachDesc - attachments;
        uiPassColorAttachRef.layout     = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        uint32_t uiPassPreserveAttach = &backupOddColorAttachDesc - attachments;

        RHISubpassDescription& uiPass  = subpasses[MAIN_CAMERA_SUBPASS_UI];
        uiPass.pipelineBindPoint       = RHI_PIPELINE_BIND_POINT_GRAPHICS;
        uiPass.inputAttachmentCount    = 0;
        uiPass.pInputAttachments       = nullptr;
        uiPass.colorAttachmentCount    = 1;
        uiPass.pColorAttachments       = &uiPassColorAttachRef;
        uiPass.pDepthStencilAttachment = nullptr;
        uiPass.preserveAttachmentCount = 1;
        uiPass.pPreserveAttachments    = &uiPassPreserveAttach;

        RHIAttachmentReference combineUIPassInputAttachRef[2] = {};
        combineUIPassInputAttachRef[0].attachment = &backupOddColorAttachDesc - attachments;
        combineUIPassInputAttachRef[0].layout     = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        combineUIPassInputAttachRef[1].attachment = &backupEvenColorAttachDesc - attachments;
        combineUIPassInputAttachRef[1].layout     = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentReference combineUIPassColorAttachRef {};
        combineUIPassColorAttachRef.attachment = &swapChainAttachDesc - attachments;
        combineUIPassColorAttachRef.layout     = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        RHISubpassDescription& combineUIPass  = subpasses[MAIN_CAMERA_SUBPASS_COMBINE_UI];
        combineUIPass.pipelineBindPoint       = RHI_PIPELINE_BIND_POINT_GRAPHICS;
        combineUIPass.inputAttachmentCount    = sizeof(combineUIPassInputAttachRef) / sizeof(combineUIPassInputAttachRef[0]);
        combineUIPass.pInputAttachments       = combineUIPassInputAttachRef;
        combineUIPass.colorAttachmentCount    = 1;
        combineUIPass.pColorAttachments       = &combineUIPassColorAttachRef;
        combineUIPass.pDepthStencilAttachment = nullptr;
        combineUIPass.preserveAttachmentCount = 0;
        combineUIPass.pPreserveAttachments    = nullptr;

        RHISubpassDependency dependencies[8] = {};

        RHISubpassDependency& deferredPassDependOnShadowMapPass = dependencies[0];
        deferredPassDependOnShadowMapPass.srcSubpass      = RHI_SUBPASS_EXTERNAL;
        deferredPassDependOnShadowMapPass.dstSubpass      = MAIN_CAMERA_SUBPASS_DEFERRED_LIGHTING;
        deferredPassDependOnShadowMapPass.srcStageMask    = RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deferredPassDependOnShadowMapPass.dstStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deferredPassDependOnShadowMapPass.srcAccessMask   = RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deferredPassDependOnShadowMapPass.dstAccessMask   = RHI_ACCESS_SHADER_READ_BIT;
        deferredPassDependOnShadowMapPass.dependencyFlags = 0; // NOT BY REGION

        RHISubpassDependency& deferredPassDependOnBasePass = dependencies[1];
        deferredPassDependOnBasePass.srcSubpass      = MAIN_CAMERA_SUBPASS_BASEPASS;
        deferredPassDependOnBasePass.dstSubpass      = MAIN_CAMERA_SUBPASS_DEFERRED_LIGHTING;
        deferredPassDependOnBasePass.srcStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deferredPassDependOnBasePass.dstStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deferredPassDependOnBasePass.srcAccessMask   = RHI_ACCESS_SHADER_WRITE_BIT | RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deferredPassDependOnBasePass.dstAccessMask   = RHI_ACCESS_SHADER_READ_BIT | RHI_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        deferredPassDependOnBasePass.dependencyFlags = RHI_DEPENDENCY_BY_REGION_BIT;

        RHISubpassDependency& forwadPassDependOnDeferredPass = dependencies[2];
        forwadPassDependOnDeferredPass.srcSubpass      = MAIN_CAMERA_SUBPASS_DEFERRED_LIGHTING;
        forwadPassDependOnDeferredPass.dstSubpass      = MAIN_CAMERA_SUBPASS_FORWARD_LIGHTING;
        forwadPassDependOnDeferredPass.srcStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        forwadPassDependOnDeferredPass.dstStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        forwadPassDependOnDeferredPass.srcAccessMask   = RHI_ACCESS_SHADER_WRITE_BIT | RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        forwadPassDependOnDeferredPass.dstAccessMask   = RHI_ACCESS_SHADER_READ_BIT | RHI_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        forwadPassDependOnDeferredPass.dependencyFlags = RHI_DEPENDENCY_BY_REGION_BIT;

        RHISubpassDependency& toneMappingDependOnLightingPass = dependencies[3];
        toneMappingDependOnLightingPass.srcSubpass      = MAIN_CAMERA_SUBPASS_FORWARD_LIGHTING;
        toneMappingDependOnLightingPass.dstSubpass      = MAIN_CAMERA_SUBPASS_TONE_MAPPING;
        toneMappingDependOnLightingPass.srcStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        toneMappingDependOnLightingPass.dstStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        toneMappingDependOnLightingPass.srcAccessMask   = RHI_ACCESS_SHADER_WRITE_BIT | RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        toneMappingDependOnLightingPass.dstAccessMask   = RHI_ACCESS_SHADER_READ_BIT | RHI_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        toneMappingDependOnLightingPass.dependencyFlags = RHI_DEPENDENCY_BY_REGION_BIT;

        RHISubpassDependency& colorGradientDependOnToneMappingPass = dependencies[4];
        colorGradientDependOnToneMappingPass.srcSubpass      = MAIN_CAMERA_SUBPASS_TONE_MAPPING;
        colorGradientDependOnToneMappingPass.dstSubpass      = MAIN_CAMERA_SUBPASS_COLOR_GRADIENT;
        colorGradientDependOnToneMappingPass.srcStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorGradientDependOnToneMappingPass.dstStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorGradientDependOnToneMappingPass.srcAccessMask   = RHI_ACCESS_SHADER_WRITE_BIT | RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        colorGradientDependOnToneMappingPass.dstAccessMask   = RHI_ACCESS_SHADER_READ_BIT | RHI_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        colorGradientDependOnToneMappingPass.dependencyFlags = RHI_DEPENDENCY_BY_REGION_BIT;

        RHISubpassDependency& uiPassDependOnColorGradient = dependencies[6];
        uiPassDependOnColorGradient.srcSubpass      = MAIN_CAMERA_SUBPASS_COLOR_GRADIENT;
        uiPassDependOnColorGradient.dstSubpass      = MAIN_CAMERA_SUBPASS_UI;
        uiPassDependOnColorGradient.srcStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        uiPassDependOnColorGradient.dstStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        uiPassDependOnColorGradient.srcAccessMask   = RHI_ACCESS_SHADER_WRITE_BIT | RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        uiPassDependOnColorGradient.dstAccessMask   = RHI_ACCESS_SHADER_READ_BIT | RHI_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        uiPassDependOnColorGradient.dependencyFlags = RHI_DEPENDENCY_BY_REGION_BIT;

        RHISubpassDependency& combineUIDependOnUIPass = dependencies[7];
        combineUIDependOnUIPass.srcSubpass      = MAIN_CAMERA_SUBPASS_UI;
        combineUIDependOnUIPass.dstSubpass      = MAIN_CAMERA_SUBPASS_COMBINE_UI;
        combineUIDependOnUIPass.srcStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        combineUIDependOnUIPass.dstStageMask    = RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        combineUIDependOnUIPass.srcAccessMask   = RHI_ACCESS_SHADER_WRITE_BIT | RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        combineUIDependOnUIPass.dstAccessMask   = RHI_ACCESS_SHADER_READ_BIT | RHI_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        combineUIDependOnUIPass.dependencyFlags = RHI_DEPENDENCY_BY_REGION_BIT;

        RHIRenderPassCreateInfo renderPassCI {};
        renderPassCI.sType           = RHI_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCI.attachmentCount = (sizeof(attachments) / sizeof(attachments[0]));
        renderPassCI.pAttachments    = attachments;
        renderPassCI.subpassCount    = (sizeof(subpasses) / sizeof(subpasses[0]));
        renderPassCI.pSubpasses      = subpasses;
        renderPassCI.dependencyCount = (sizeof(dependencies) / sizeof(dependencies[0]));
        renderPassCI.pDependencies   = dependencies;

        if (mRHI->CreateRenderPass(&renderPassCI, mFrameBuffer.renderPass) != RHI_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass");
        }
    }

    void MainCameraPass::setupDescriptorSetLayout()
    {
    }

    void MainCameraPass::setupPipelines()
    {
    }

    void MainCameraPass::setupDescriptorSet()
    {
        setupModelGlobalDescriptorSet();
        setupSkyboxDescriptorSet();
        setupAxisDescriptorSet();
        setupGBufferLightingDescriptorSet();
    }

    void MainCameraPass::setupFrameBufferDescriptorSet()
    {
    }

    void MainCameraPass::setupSwapChainFrameBuffers()
    {
    }

    void MainCameraPass::setupModelGlobalDescriptorSet()
    {
    }

    void MainCameraPass::setupSkyboxDescriptorSet()
    {
    }

    void MainCameraPass::setupAxisDescriptorSet()
    {
    }

    void MainCameraPass::setupGBufferLightingDescriptorSet()
    {
    }

    void MainCameraPass::drawMeshGBuffer()
    {
    }

    void MainCameraPass::drawDeferredLighting()
    {
    }

    void MainCameraPass::drawMeshLighting()
    {
    }

    void MainCameraPass::drawSkybox()
    {
    }

    void MainCameraPass::drawAxis()
    {
    }

} // namespace MiniEngine