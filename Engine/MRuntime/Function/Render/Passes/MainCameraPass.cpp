#include "MainCameraPass.hpp"
#include "MRuntime/Function/Render/RenderHelper.hpp"
#include "MRuntime/Function/Render/RenderMesh.hpp"
#include "MRuntime/Function/Render/RenderResource.hpp"

#include "MRuntime/Function/Render/Interface/Vulkan/VulkanRHI.hpp"
#include "MRuntime/Function/Render/Interface/Vulkan/VulkanUtil.hpp"

#include <map>
#include <stdexcept>

#include <Axis_frag.h>
#include <Axis_vert.h>
#include <DeferredLighting_frag.h>
#include <DeferredLighting_vert.h>
#include <MeshGBuffer_frag.h>
#include <BasePass_frag.h>
#include <BasePass_vert.h>
#include <Skybox_frag.h>
#include <Skybox_vert.h>

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
        mDescInfos.resize(LayoutType_Count);
        {
            RHIDescriptorSetLayoutBinding meshDescSetLayoutBinding;
            meshDescSetLayoutBinding.binding                       = 0;
            meshDescSetLayoutBinding.descriptorType                = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            meshDescSetLayoutBinding.descriptorCount               = 1;
            meshDescSetLayoutBinding.stageFlags                    = RHI_SHADER_STAGE_VERTEX_BIT;
            meshDescSetLayoutBinding.pImmutableSamplers            = nullptr;

            RHIDescriptorSetLayoutCreateInfo meshDescSetLayoutCI {};
            meshDescSetLayoutCI.sType        = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            meshDescSetLayoutCI.bindingCount = 1;
            meshDescSetLayoutCI.pBindings    = &meshDescSetLayoutBinding;

            if (mRHI->CreateDescriptorSetLayout(&meshDescSetLayoutCI, mDescInfos[LayoutType_PerMesh].layout) != RHI_SUCCESS)
            {
                throw std::runtime_error("create mesh mesh layout");
            }
        }
        {
            RHIDescriptorSetLayoutBinding meshGlobalDescSetLayoutBinding[7];

            RHIDescriptorSetLayoutBinding& meshGlobalDescSetLayoutPerFrameBinding = meshGlobalDescSetLayoutBinding[0];
            meshGlobalDescSetLayoutPerFrameBinding.binding            = 0;
            meshGlobalDescSetLayoutPerFrameBinding.descriptorType     = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            meshGlobalDescSetLayoutPerFrameBinding.descriptorCount    = 1;
            meshGlobalDescSetLayoutPerFrameBinding.stageFlags         = RHI_SHADER_STAGE_VERTEX_BIT | RHI_SHADER_STAGE_FRAGMENT_BIT;
            meshGlobalDescSetLayoutPerFrameBinding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutBinding& meshGlobalDescSetLayoutPerDrawCallBinding = meshGlobalDescSetLayoutBinding[1];
            meshGlobalDescSetLayoutPerDrawCallBinding.binding            = 1;
            meshGlobalDescSetLayoutPerDrawCallBinding.descriptorType     = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            meshGlobalDescSetLayoutPerDrawCallBinding.descriptorCount    = 1;
            meshGlobalDescSetLayoutPerDrawCallBinding.stageFlags         = RHI_SHADER_STAGE_VERTEX_BIT;
            meshGlobalDescSetLayoutPerDrawCallBinding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutBinding& meshGlobalDescSetLayoutPerDrawCallVertexBlendingBinding = meshGlobalDescSetLayoutBinding[2];
            meshGlobalDescSetLayoutPerDrawCallVertexBlendingBinding.binding            = 2;
            meshGlobalDescSetLayoutPerDrawCallVertexBlendingBinding.descriptorType     = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            meshGlobalDescSetLayoutPerDrawCallVertexBlendingBinding.descriptorCount    = 1;
            meshGlobalDescSetLayoutPerDrawCallVertexBlendingBinding.stageFlags         = RHI_SHADER_STAGE_VERTEX_BIT;
            meshGlobalDescSetLayoutPerDrawCallVertexBlendingBinding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutBinding& meshGlobalDescSetLayoutBrdfLutBinding = meshGlobalDescSetLayoutBinding[3];
            meshGlobalDescSetLayoutBrdfLutBinding.binding            = 3;
            meshGlobalDescSetLayoutBrdfLutBinding.descriptorType     = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            meshGlobalDescSetLayoutBrdfLutBinding.descriptorCount    = 1;
            meshGlobalDescSetLayoutBrdfLutBinding.stageFlags         = RHI_SHADER_STAGE_FRAGMENT_BIT;
            meshGlobalDescSetLayoutBrdfLutBinding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutBinding& meshGlobalDescSetLayoutIrradianceBinding = meshGlobalDescSetLayoutBinding[4];
            meshGlobalDescSetLayoutIrradianceBinding         = meshGlobalDescSetLayoutBrdfLutBinding;
            meshGlobalDescSetLayoutIrradianceBinding.binding = 4;

            RHIDescriptorSetLayoutBinding& meshGlobalDescSetLayoutSpecularBinding = meshGlobalDescSetLayoutBinding[5];
            meshGlobalDescSetLayoutSpecularBinding         = meshGlobalDescSetLayoutBrdfLutBinding;
            meshGlobalDescSetLayoutSpecularBinding.binding = 5;

            RHIDescriptorSetLayoutBinding& meshGlobalDescSetLayoutDirectLightShadowBinding = meshGlobalDescSetLayoutBinding[6];
            meshGlobalDescSetLayoutDirectLightShadowBinding         = meshGlobalDescSetLayoutBrdfLutBinding;
            meshGlobalDescSetLayoutDirectLightShadowBinding.binding = 6;

            RHIDescriptorSetLayoutCreateInfo meshGlobalDescSetLayoutCI {};
            meshGlobalDescSetLayoutCI.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            meshGlobalDescSetLayoutCI.pNext = nullptr;
            meshGlobalDescSetLayoutCI.flags = 0;
            meshGlobalDescSetLayoutCI.bindingCount = (sizeof(meshGlobalDescSetLayoutBinding) / sizeof(meshGlobalDescSetLayoutBinding[0]));
            meshGlobalDescSetLayoutCI.pBindings = meshGlobalDescSetLayoutBinding;

            if (RHI_SUCCESS != mRHI->CreateDescriptorSetLayout(&meshGlobalDescSetLayoutCI, mDescInfos[LayoutType_MeshGlobal].layout))
            {
                throw std::runtime_error("create mesh global layout");
            }
        }
        {
            RHIDescriptorSetLayoutBinding meshMatDescSetLayoutBinding[6];

            // (set = 2, binding = 0 in fragment shader)
            RHIDescriptorSetLayoutBinding& meshMatDescSetLayoutUniformBinding = meshMatDescSetLayoutBinding[0];
            meshMatDescSetLayoutUniformBinding.binding            = 0;
            meshMatDescSetLayoutUniformBinding.descriptorType     = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            meshMatDescSetLayoutUniformBinding.descriptorCount    = 1;
            meshMatDescSetLayoutUniformBinding.stageFlags         = RHI_SHADER_STAGE_FRAGMENT_BIT;
            meshMatDescSetLayoutUniformBinding.pImmutableSamplers = nullptr;

            // (set = 2, binding = 1 in fragment shader)
            RHIDescriptorSetLayoutBinding& meshMatDescSetLayoutBaseColorBinding = meshMatDescSetLayoutBinding[1];
            meshMatDescSetLayoutBaseColorBinding.binding         = 1;
            meshMatDescSetLayoutBaseColorBinding.descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            meshMatDescSetLayoutBaseColorBinding.descriptorCount = 1;
            meshMatDescSetLayoutBaseColorBinding.stageFlags      = RHI_SHADER_STAGE_FRAGMENT_BIT;
            meshMatDescSetLayoutBaseColorBinding.pImmutableSamplers = nullptr;

            // (set = 2, binding = 2 in fragment shader)
            RHIDescriptorSetLayoutBinding& meshMatDescSetLayoutMetalicRoughBinding = meshMatDescSetLayoutBinding[2];
            meshMatDescSetLayoutMetalicRoughBinding = meshMatDescSetLayoutBaseColorBinding;
            meshMatDescSetLayoutMetalicRoughBinding.binding = 2;

            // (set = 2, binding = 3 in fragment shader)
            RHIDescriptorSetLayoutBinding& meshMatDescSetLayoutNormalRoughBinding = meshMatDescSetLayoutBinding[3];
            meshMatDescSetLayoutNormalRoughBinding = meshMatDescSetLayoutBaseColorBinding;
            meshMatDescSetLayoutNormalRoughBinding.binding = 3;

            // (set = 2, binding = 4 in fragment shader)
            RHIDescriptorSetLayoutBinding& meshMatDescSetLayoutOcclusionBinding = meshMatDescSetLayoutBinding[4];
            meshMatDescSetLayoutOcclusionBinding         = meshMatDescSetLayoutBaseColorBinding;
            meshMatDescSetLayoutOcclusionBinding.binding = 4;

            // (set = 2, binding = 5 in fragment shader)
            RHIDescriptorSetLayoutBinding& meshMatDescSetLayoutEmissiveBinding = meshMatDescSetLayoutBinding[5];
            meshMatDescSetLayoutEmissiveBinding         = meshMatDescSetLayoutBaseColorBinding;
            meshMatDescSetLayoutEmissiveBinding.binding = 5;

            RHIDescriptorSetLayoutCreateInfo meshMatDescSetLayoutCI {};
            meshMatDescSetLayoutCI.sType        = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            meshMatDescSetLayoutCI.pNext        = nullptr;
            meshMatDescSetLayoutCI.flags        = 0;
            meshMatDescSetLayoutCI.bindingCount = 6;
            meshMatDescSetLayoutCI.pBindings    = meshMatDescSetLayoutBinding;

            if (mRHI->CreateDescriptorSetLayout(&meshMatDescSetLayoutCI, mDescInfos[LayoutType_MeshPerMaterial].layout) != RHI_SUCCESS)
            {
                throw std::runtime_error("create mesh material layout");
            }
        }

        {
            RHIDescriptorSetLayoutBinding skyboxDescSetLayoutBinding[2];

            RHIDescriptorSetLayoutBinding& skyboxDescSetLayoutPerFrameBinding = skyboxDescSetLayoutBinding[0];
            skyboxDescSetLayoutPerFrameBinding.binding            = 0;
            skyboxDescSetLayoutPerFrameBinding.descriptorType     = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            skyboxDescSetLayoutPerFrameBinding.descriptorCount    = 1;
            skyboxDescSetLayoutPerFrameBinding.stageFlags         = RHI_SHADER_STAGE_VERTEX_BIT;
            skyboxDescSetLayoutPerFrameBinding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutBinding& skyboxDescSetLayoutSpecularBinding = skyboxDescSetLayoutBinding[1];
            skyboxDescSetLayoutSpecularBinding.binding            = 1;
            skyboxDescSetLayoutSpecularBinding.descriptorType     = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            skyboxDescSetLayoutSpecularBinding.descriptorCount    = 1;
            skyboxDescSetLayoutSpecularBinding.stageFlags         = RHI_SHADER_STAGE_FRAGMENT_BIT;
            skyboxDescSetLayoutSpecularBinding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutCreateInfo skyboxDescSetLayoutCI {};
            skyboxDescSetLayoutCI.sType        = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            skyboxDescSetLayoutCI.bindingCount = 2;
            skyboxDescSetLayoutCI.pBindings    = skyboxDescSetLayoutBinding;

            if (RHI_SUCCESS != mRHI->CreateDescriptorSetLayout(&skyboxDescSetLayoutCI, mDescInfos[LayoutType_Skybox].layout))
            {
                throw std::runtime_error("create skybox layout");
            }
        }
        {
            RHIDescriptorSetLayoutBinding axisDescSetLayoutBinding[2];

            RHIDescriptorSetLayoutBinding& axisDescSetLayoutPerFrameBinding = axisDescSetLayoutBinding[0];
            axisDescSetLayoutPerFrameBinding.binding            = 0;
            axisDescSetLayoutPerFrameBinding.descriptorType     = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            axisDescSetLayoutPerFrameBinding.descriptorCount    = 1;
            axisDescSetLayoutPerFrameBinding.stageFlags         = RHI_SHADER_STAGE_VERTEX_BIT;
            axisDescSetLayoutPerFrameBinding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutBinding& axisDescSetLayoutBufferBinding = axisDescSetLayoutBinding[1];
            axisDescSetLayoutBufferBinding.binding            = 1;
            axisDescSetLayoutBufferBinding.descriptorType     = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            axisDescSetLayoutBufferBinding.descriptorCount    = 1;
            axisDescSetLayoutBufferBinding.stageFlags         = RHI_SHADER_STAGE_VERTEX_BIT;
            axisDescSetLayoutBufferBinding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutCreateInfo axisDescSetLayoutCI {};
            axisDescSetLayoutCI.sType        = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            axisDescSetLayoutCI.bindingCount = 2;
            axisDescSetLayoutCI.pBindings    = axisDescSetLayoutBinding;

            if (RHI_SUCCESS != mRHI->CreateDescriptorSetLayout(&axisDescSetLayoutCI, mDescInfos[LayoutType_Axis].layout))
            {
                throw std::runtime_error("create axis layout");
            }
        }
        {
            RHIDescriptorSetLayoutBinding gbufferLightingDescSetLayoutBinding[4];

            RHIDescriptorSetLayoutBinding& gbufferNormalDescSetLayoutInputAttachBinding = gbufferLightingDescSetLayoutBinding[0];
            gbufferNormalDescSetLayoutInputAttachBinding.binding         = 0;
            gbufferNormalDescSetLayoutInputAttachBinding.descriptorType  = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            gbufferNormalDescSetLayoutInputAttachBinding.descriptorCount = 1;
            gbufferNormalDescSetLayoutInputAttachBinding.stageFlags      = RHI_SHADER_STAGE_FRAGMENT_BIT;

            RHIDescriptorSetLayoutBinding& gbufferMetalRoughDescSetLayoutInputAttachBinding = gbufferLightingDescSetLayoutBinding[1];
            gbufferMetalRoughDescSetLayoutInputAttachBinding.binding = 1;
            gbufferMetalRoughDescSetLayoutInputAttachBinding.descriptorType = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            gbufferMetalRoughDescSetLayoutInputAttachBinding.descriptorCount = 1;
            gbufferMetalRoughDescSetLayoutInputAttachBinding.stageFlags = RHI_SHADER_STAGE_FRAGMENT_BIT;

            RHIDescriptorSetLayoutBinding& gbufferAlbedoDescSetLayoutInputAttachBinding = gbufferLightingDescSetLayoutBinding[2];
            gbufferAlbedoDescSetLayoutInputAttachBinding.binding         = 2;
            gbufferAlbedoDescSetLayoutInputAttachBinding.descriptorType  = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            gbufferAlbedoDescSetLayoutInputAttachBinding.descriptorCount = 1;
            gbufferAlbedoDescSetLayoutInputAttachBinding.stageFlags      = RHI_SHADER_STAGE_FRAGMENT_BIT;

            RHIDescriptorSetLayoutBinding& gbufferDepthDescSetLayoutInputAttachBinding = gbufferLightingDescSetLayoutBinding[3];
            gbufferDepthDescSetLayoutInputAttachBinding.binding         = 3;
            gbufferDepthDescSetLayoutInputAttachBinding.descriptorType  = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            gbufferDepthDescSetLayoutInputAttachBinding.descriptorCount = 1;
            gbufferDepthDescSetLayoutInputAttachBinding.stageFlags      = RHI_SHADER_STAGE_FRAGMENT_BIT;

            RHIDescriptorSetLayoutCreateInfo gbufferLightingDescSetLayoutCI {};
            gbufferLightingDescSetLayoutCI.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            gbufferLightingDescSetLayoutCI.pNext = nullptr;
            gbufferLightingDescSetLayoutCI.flags = 0;
            gbufferLightingDescSetLayoutCI.bindingCount = sizeof(gbufferLightingDescSetLayoutBinding) / sizeof(gbufferLightingDescSetLayoutBinding[0]);
            gbufferLightingDescSetLayoutCI.pBindings = gbufferLightingDescSetLayoutBinding;

            if (RHI_SUCCESS != mRHI->CreateDescriptorSetLayout(&gbufferLightingDescSetLayoutCI, mDescInfos[LayoutType_DeferredLighting].layout))
            {
                throw std::runtime_error("create deferred lighting global layout");
            }
        }
    }

    void MainCameraPass::setupPipelines()
    {
        mRenderPipelines.resize(RenderPipelineType_Count);

        // mesh gbuffer
        {
            RHIDescriptorSetLayout*      descriptorset_layouts[3] = {mDescInfos[LayoutType_MeshGlobal].layout,
                                                              mDescInfos[LayoutType_PerMesh].layout,
                                                              mDescInfos[LayoutType_MeshPerMaterial].layout};
            RHIPipelineLayoutCreateInfo pipeline_layout_create_info {};
            pipeline_layout_create_info.sType          = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = 3;
            pipeline_layout_create_info.pSetLayouts    = descriptorset_layouts;

            if (mRHI->CreatePipelineLayout(&pipeline_layout_create_info, mRenderPipelines[RenderPipelineType_MeshGBuffer].layout) != RHI_SUCCESS)
            {
                throw std::runtime_error("create mesh gbuffer pipeline layout");
            }

            RHIShader* vert_shader_module = mRHI->CreateShaderModule(BASEPASS_VERT);
            RHIShader* frag_shader_module = mRHI->CreateShaderModule(MESHGBUFFER_FRAG);

            RHIPipelineShaderStageCreateInfo vert_pipeline_shader_stage_create_info {};
            vert_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
            vert_pipeline_shader_stage_create_info.module = vert_shader_module;
            vert_pipeline_shader_stage_create_info.pName  = "main";

            RHIPipelineShaderStageCreateInfo frag_pipeline_shader_stage_create_info {};
            frag_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
            frag_pipeline_shader_stage_create_info.module = frag_shader_module;
            frag_pipeline_shader_stage_create_info.pName  = "main";

            RHIPipelineShaderStageCreateInfo shader_stages[] = {vert_pipeline_shader_stage_create_info,
                                                               frag_pipeline_shader_stage_create_info};

            auto vertex_binding_descriptions   = MeshVertex::GetBindingDescriptions();
            auto vertex_attribute_descriptions = MeshVertex::GetAttributeDescriptions();
            RHIPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
            vertex_input_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_create_info.vertexBindingDescriptionCount   = vertex_binding_descriptions.size();
            vertex_input_state_create_info.pVertexBindingDescriptions      = &vertex_binding_descriptions[0];
            vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_attribute_descriptions.size();
            vertex_input_state_create_info.pVertexAttributeDescriptions    = &vertex_attribute_descriptions[0];

            RHIPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
            input_assembly_create_info.sType    = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_create_info.topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly_create_info.primitiveRestartEnable = RHI_FALSE;

            RHIPipelineViewportStateCreateInfo viewport_state_create_info {};
            viewport_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_create_info.viewportCount = 1;
            viewport_state_create_info.pViewports    = mRHI->GetSwapChainInfo().viewport;
            viewport_state_create_info.scissorCount  = 1;
            viewport_state_create_info.pScissors     = mRHI->GetSwapChainInfo().scissor;

            RHIPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
            rasterization_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterization_state_create_info.depthClampEnable        = RHI_FALSE;
            rasterization_state_create_info.rasterizerDiscardEnable = RHI_FALSE;
            rasterization_state_create_info.polygonMode             = RHI_POLYGON_MODE_FILL;
            rasterization_state_create_info.lineWidth               = 1.0f;
            rasterization_state_create_info.cullMode                = RHI_CULL_MODE_BACK_BIT;
            rasterization_state_create_info.frontFace               = RHI_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterization_state_create_info.depthBiasEnable         = RHI_FALSE;
            rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
            rasterization_state_create_info.depthBiasClamp          = 0.0f;
            rasterization_state_create_info.depthBiasSlopeFactor    = 0.0f;

            RHIPipelineMultisampleStateCreateInfo multisample_state_create_info {};
            multisample_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisample_state_create_info.sampleShadingEnable  = RHI_FALSE;
            multisample_state_create_info.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

            RHIPipelineColorBlendAttachmentState color_blend_attachments[3] = {};
            color_blend_attachments[0].colorWriteMask = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT |
                                                        RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
            color_blend_attachments[0].blendEnable         = RHI_FALSE;
            color_blend_attachments[0].srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachments[0].colorBlendOp        = RHI_BLEND_OP_ADD;
            color_blend_attachments[0].srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachments[0].alphaBlendOp        = RHI_BLEND_OP_ADD;
            color_blend_attachments[1].colorWriteMask      = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT |
                                                             RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
            color_blend_attachments[1].blendEnable         = RHI_FALSE;
            color_blend_attachments[1].srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[1].dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachments[1].colorBlendOp        = RHI_BLEND_OP_ADD;
            color_blend_attachments[1].srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[1].dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachments[1].alphaBlendOp        = RHI_BLEND_OP_ADD;
            color_blend_attachments[2].colorWriteMask      = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT |
                                                             RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
            color_blend_attachments[2].blendEnable         = RHI_FALSE;
            color_blend_attachments[2].srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[2].dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachments[2].colorBlendOp        = RHI_BLEND_OP_ADD;
            color_blend_attachments[2].srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[2].dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachments[2].alphaBlendOp        = RHI_BLEND_OP_ADD;

            RHIPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
            color_blend_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_state_create_info.logicOpEnable = RHI_FALSE;
            color_blend_state_create_info.logicOp       = RHI_LOGIC_OP_COPY;
            color_blend_state_create_info.attachmentCount =
                sizeof(color_blend_attachments) / sizeof(color_blend_attachments[0]);
            color_blend_state_create_info.pAttachments      = &color_blend_attachments[0];
            color_blend_state_create_info.blendConstants[0] = 0.0f;
            color_blend_state_create_info.blendConstants[1] = 0.0f;
            color_blend_state_create_info.blendConstants[2] = 0.0f;
            color_blend_state_create_info.blendConstants[3] = 0.0f;

            RHIPipelineDepthStencilStateCreateInfo depth_stencil_create_info {};
            depth_stencil_create_info.sType            = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_create_info.depthTestEnable  = RHI_TRUE;
            depth_stencil_create_info.depthWriteEnable = RHI_TRUE;
            depth_stencil_create_info.depthCompareOp   = RHI_COMPARE_OP_LESS;
            depth_stencil_create_info.depthBoundsTestEnable = RHI_FALSE;
            depth_stencil_create_info.stencilTestEnable     = RHI_FALSE;

            RHIDynamicState                   dynamic_states[] = {RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR};
            RHIPipelineDynamicStateCreateInfo dynamic_state_create_info {};
            dynamic_state_create_info.sType             = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state_create_info.dynamicStateCount = 2;
            dynamic_state_create_info.pDynamicStates    = dynamic_states;

            RHIGraphicsPipelineCreateInfo pipelineInfo {};
            pipelineInfo.sType               = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount          = 2;
            pipelineInfo.pStages             = shader_stages;
            pipelineInfo.pVertexInputState   = &vertex_input_state_create_info;
            pipelineInfo.pInputAssemblyState = &input_assembly_create_info;
            pipelineInfo.pViewportState      = &viewport_state_create_info;
            pipelineInfo.pRasterizationState = &rasterization_state_create_info;
            pipelineInfo.pMultisampleState   = &multisample_state_create_info;
            pipelineInfo.pColorBlendState    = &color_blend_state_create_info;
            pipelineInfo.pDepthStencilState  = &depth_stencil_create_info;
            pipelineInfo.layout              = mRenderPipelines[RenderPipelineType_MeshGBuffer].layout;
            pipelineInfo.renderPass          = mFrameBuffer.renderPass;
            pipelineInfo.subpass             = MAIN_CAMERA_SUBPASS_BASEPASS;
            pipelineInfo.basePipelineHandle  = RHI_NULL_HANDLE;
            pipelineInfo.pDynamicState       = &dynamic_state_create_info;

            if (RHI_SUCCESS != mRHI->CreateGraphicsPipelines(
                RHI_NULL_HANDLE,
                1,
                &pipelineInfo,
                mRenderPipelines[RenderPipelineType_MeshGBuffer].pipeline))
            {
                throw std::runtime_error("create mesh gbuffer graphics pipeline");
            }

            mRHI->DestroyShaderModule(vert_shader_module);
            mRHI->DestroyShaderModule(frag_shader_module);
        }

        // deferred lighting
        {
            RHIDescriptorSetLayout* descriptorset_layouts[3] = {
                mDescInfos[LayoutType_MeshGlobal].layout,
                mDescInfos[LayoutType_DeferredLighting].layout,
                mDescInfos[LayoutType_Skybox].layout};
            RHIPipelineLayoutCreateInfo pipeline_layout_create_info {};
            pipeline_layout_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount =
                sizeof(descriptorset_layouts) / sizeof(descriptorset_layouts[0]);
            pipeline_layout_create_info.pSetLayouts = descriptorset_layouts;

            if (RHI_SUCCESS != mRHI->CreatePipelineLayout(&pipeline_layout_create_info, mRenderPipelines[RenderPipelineType_DeferredLighting].layout))
            {
                throw std::runtime_error("create deferred lighting pipeline layout");
            }

            RHIShader* vert_shader_module = mRHI->CreateShaderModule(DEFERREDLIGHTING_VERT);
            RHIShader* frag_shader_module = mRHI->CreateShaderModule(DEFERREDLIGHTING_FRAG);

            RHIPipelineShaderStageCreateInfo vert_pipeline_shader_stage_create_info {};
            vert_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
            vert_pipeline_shader_stage_create_info.module = vert_shader_module;
            vert_pipeline_shader_stage_create_info.pName  = "main";
            // vert_pipeline_shader_stage_create_info.pSpecializationInfo

            RHIPipelineShaderStageCreateInfo frag_pipeline_shader_stage_create_info {};
            frag_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
            frag_pipeline_shader_stage_create_info.module = frag_shader_module;
            frag_pipeline_shader_stage_create_info.pName  = "main";

            RHIPipelineShaderStageCreateInfo shader_stages[] = {
                vert_pipeline_shader_stage_create_info,
                frag_pipeline_shader_stage_create_info};

            auto vertex_binding_descriptions   = MeshVertex::GetBindingDescriptions();
            auto vertex_attribute_descriptions = MeshVertex::GetAttributeDescriptions();
            RHIPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
            vertex_input_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
            vertex_input_state_create_info.pVertexBindingDescriptions    = nullptr;
            vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
            vertex_input_state_create_info.pVertexAttributeDescriptions  = nullptr;

            RHIPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
            input_assembly_create_info.sType    = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_create_info.topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly_create_info.primitiveRestartEnable = RHI_FALSE;

            RHIPipelineViewportStateCreateInfo viewport_state_create_info {};
            viewport_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_create_info.viewportCount = 1;
            viewport_state_create_info.pViewports    = mRHI->GetSwapChainInfo().viewport;
            viewport_state_create_info.scissorCount  = 1;
            viewport_state_create_info.pScissors     = mRHI->GetSwapChainInfo().scissor;

            RHIPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
            rasterization_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterization_state_create_info.depthClampEnable        = RHI_FALSE;
            rasterization_state_create_info.rasterizerDiscardEnable = RHI_FALSE;
            rasterization_state_create_info.polygonMode             = RHI_POLYGON_MODE_FILL;
            rasterization_state_create_info.lineWidth               = 1.0f;
            rasterization_state_create_info.cullMode                = RHI_CULL_MODE_BACK_BIT;
            rasterization_state_create_info.frontFace               = RHI_FRONT_FACE_CLOCKWISE;
            rasterization_state_create_info.depthBiasEnable         = RHI_FALSE;
            rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
            rasterization_state_create_info.depthBiasClamp          = 0.0f;
            rasterization_state_create_info.depthBiasSlopeFactor    = 0.0f;

            RHIPipelineMultisampleStateCreateInfo multisample_state_create_info {};
            multisample_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisample_state_create_info.sampleShadingEnable  = RHI_FALSE;
            multisample_state_create_info.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

            RHIPipelineColorBlendAttachmentState color_blend_attachments[1] = {};
            color_blend_attachments[0].colorWriteMask = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT |
                                                        RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
            color_blend_attachments[0].blendEnable         = RHI_FALSE;
            color_blend_attachments[0].srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].dstColorBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].colorBlendOp        = RHI_BLEND_OP_ADD;
            color_blend_attachments[0].srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].dstAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].alphaBlendOp        = RHI_BLEND_OP_ADD;

            RHIPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
            color_blend_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_state_create_info.logicOpEnable = RHI_FALSE;
            color_blend_state_create_info.logicOp       = RHI_LOGIC_OP_COPY;
            color_blend_state_create_info.attachmentCount =
                sizeof(color_blend_attachments) / sizeof(color_blend_attachments[0]);
            color_blend_state_create_info.pAttachments      = &color_blend_attachments[0];
            color_blend_state_create_info.blendConstants[0] = 0.0f;
            color_blend_state_create_info.blendConstants[1] = 0.0f;
            color_blend_state_create_info.blendConstants[2] = 0.0f;
            color_blend_state_create_info.blendConstants[3] = 0.0f;

            RHIPipelineDepthStencilStateCreateInfo depth_stencil_create_info {};
            depth_stencil_create_info.sType            = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_create_info.depthTestEnable  = RHI_FALSE;
            depth_stencil_create_info.depthWriteEnable = RHI_FALSE;
            depth_stencil_create_info.depthCompareOp   = RHI_COMPARE_OP_ALWAYS;
            depth_stencil_create_info.depthBoundsTestEnable = RHI_FALSE;
            depth_stencil_create_info.stencilTestEnable     = RHI_FALSE;

            RHIDynamicState                   dynamic_states[] = {RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR};
            RHIPipelineDynamicStateCreateInfo dynamic_state_create_info {};
            dynamic_state_create_info.sType             = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state_create_info.dynamicStateCount = 2;
            dynamic_state_create_info.pDynamicStates    = dynamic_states;

            RHIGraphicsPipelineCreateInfo pipelineInfo {};
            pipelineInfo.sType               = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount          = 2;
            pipelineInfo.pStages             = shader_stages;
            pipelineInfo.pVertexInputState   = &vertex_input_state_create_info;
            pipelineInfo.pInputAssemblyState = &input_assembly_create_info;
            pipelineInfo.pViewportState      = &viewport_state_create_info;
            pipelineInfo.pRasterizationState = &rasterization_state_create_info;
            pipelineInfo.pMultisampleState   = &multisample_state_create_info;
            pipelineInfo.pColorBlendState    = &color_blend_state_create_info;
            pipelineInfo.pDepthStencilState  = &depth_stencil_create_info;
            pipelineInfo.layout              = mRenderPipelines[RenderPipelineType_DeferredLighting].layout;
            pipelineInfo.renderPass          = mFrameBuffer.renderPass;
            pipelineInfo.subpass             = MAIN_CAMERA_SUBPASS_DEFERRED_LIGHTING;
            pipelineInfo.basePipelineHandle  = RHI_NULL_HANDLE;
            pipelineInfo.pDynamicState       = &dynamic_state_create_info;

            if (RHI_SUCCESS != mRHI->CreateGraphicsPipelines(
                RHI_NULL_HANDLE,
                1,
                &pipelineInfo,
                mRenderPipelines[RenderPipelineType_DeferredLighting].pipeline))
            {
                throw std::runtime_error("create deferred lighting graphics pipeline");
            }

            mRHI->DestroyShaderModule(vert_shader_module);
            mRHI->DestroyShaderModule(frag_shader_module);
        }

        // mesh lighting
        {
            RHIDescriptorSetLayout*      descriptorset_layouts[3] = {mDescInfos[LayoutType_MeshGlobal].layout,
                                                                     mDescInfos[LayoutType_PerMesh].layout,
                                                                     mDescInfos[LayoutType_MeshPerMaterial].layout};
            RHIPipelineLayoutCreateInfo pipeline_layout_create_info {};
            pipeline_layout_create_info.sType          = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = 3;
            pipeline_layout_create_info.pSetLayouts    = descriptorset_layouts;

            if (mRHI->CreatePipelineLayout(&pipeline_layout_create_info, mRenderPipelines[RenderPipelineType_MeshLighting].layout) != RHI_SUCCESS)
            {
                throw std::runtime_error("create mesh lighting pipeline layout");
            }

            RHIShader* vert_shader_module = mRHI->CreateShaderModule(BASEPASS_VERT);
            RHIShader* frag_shader_module = mRHI->CreateShaderModule(BASEPASS_FRAG);

            RHIPipelineShaderStageCreateInfo vert_pipeline_shader_stage_create_info {};
            vert_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
            vert_pipeline_shader_stage_create_info.module = vert_shader_module;
            vert_pipeline_shader_stage_create_info.pName  = "main";

            RHIPipelineShaderStageCreateInfo frag_pipeline_shader_stage_create_info {};
            frag_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
            frag_pipeline_shader_stage_create_info.module = frag_shader_module;
            frag_pipeline_shader_stage_create_info.pName  = "main";

            RHIPipelineShaderStageCreateInfo shader_stages[] = {vert_pipeline_shader_stage_create_info,
                                                               frag_pipeline_shader_stage_create_info};

            auto vertex_binding_descriptions   = MeshVertex::GetBindingDescriptions();
            auto vertex_attribute_descriptions = MeshVertex::GetAttributeDescriptions();
            RHIPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
            vertex_input_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_create_info.vertexBindingDescriptionCount   = vertex_binding_descriptions.size();
            vertex_input_state_create_info.pVertexBindingDescriptions      = &vertex_binding_descriptions[0];
            vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_attribute_descriptions.size();
            vertex_input_state_create_info.pVertexAttributeDescriptions    = &vertex_attribute_descriptions[0];

            RHIPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
            input_assembly_create_info.sType    = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_create_info.topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly_create_info.primitiveRestartEnable = RHI_FALSE;

            RHIPipelineViewportStateCreateInfo viewport_state_create_info {};
            viewport_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_create_info.viewportCount = 1;
            viewport_state_create_info.pViewports    = mRHI->GetSwapChainInfo().viewport;
            viewport_state_create_info.scissorCount  = 1;
            viewport_state_create_info.pScissors     = mRHI->GetSwapChainInfo().scissor;

            RHIPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
            rasterization_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterization_state_create_info.depthClampEnable        = RHI_FALSE;
            rasterization_state_create_info.rasterizerDiscardEnable = RHI_FALSE;
            rasterization_state_create_info.polygonMode             = RHI_POLYGON_MODE_FILL;
            rasterization_state_create_info.lineWidth               = 1.0f;
            rasterization_state_create_info.cullMode                = RHI_CULL_MODE_BACK_BIT;
            rasterization_state_create_info.frontFace               = RHI_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterization_state_create_info.depthBiasEnable         = RHI_FALSE;
            rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
            rasterization_state_create_info.depthBiasClamp          = 0.0f;
            rasterization_state_create_info.depthBiasSlopeFactor    = 0.0f;

            RHIPipelineMultisampleStateCreateInfo multisample_state_create_info {};
            multisample_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisample_state_create_info.sampleShadingEnable  = RHI_FALSE;
            multisample_state_create_info.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

            RHIPipelineColorBlendAttachmentState color_blend_attachments[1] = {};
            color_blend_attachments[0].colorWriteMask = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT |
                                                        RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
            color_blend_attachments[0].blendEnable         = RHI_FALSE;
            color_blend_attachments[0].srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].dstColorBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].colorBlendOp        = RHI_BLEND_OP_ADD;
            color_blend_attachments[0].srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].dstAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].alphaBlendOp        = RHI_BLEND_OP_ADD;

            RHIPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
            color_blend_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_state_create_info.logicOpEnable = RHI_FALSE;
            color_blend_state_create_info.logicOp       = RHI_LOGIC_OP_COPY;
            color_blend_state_create_info.attachmentCount =
                sizeof(color_blend_attachments) / sizeof(color_blend_attachments[0]);
            color_blend_state_create_info.pAttachments      = &color_blend_attachments[0];
            color_blend_state_create_info.blendConstants[0] = 0.0f;
            color_blend_state_create_info.blendConstants[1] = 0.0f;
            color_blend_state_create_info.blendConstants[2] = 0.0f;
            color_blend_state_create_info.blendConstants[3] = 0.0f;

            RHIPipelineDepthStencilStateCreateInfo depth_stencil_create_info {};
            depth_stencil_create_info.sType            = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_create_info.depthTestEnable  = RHI_TRUE;
            depth_stencil_create_info.depthWriteEnable = RHI_TRUE;
            depth_stencil_create_info.depthCompareOp   = RHI_COMPARE_OP_LESS;
            depth_stencil_create_info.depthBoundsTestEnable = RHI_FALSE;
            depth_stencil_create_info.stencilTestEnable     = RHI_FALSE;

            RHIDynamicState                   dynamic_states[] = {RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR};
            RHIPipelineDynamicStateCreateInfo dynamic_state_create_info {};
            dynamic_state_create_info.sType             = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state_create_info.dynamicStateCount = 2;
            dynamic_state_create_info.pDynamicStates    = dynamic_states;

            RHIGraphicsPipelineCreateInfo pipelineInfo {};
            pipelineInfo.sType               = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount          = 2;
            pipelineInfo.pStages             = shader_stages;
            pipelineInfo.pVertexInputState   = &vertex_input_state_create_info;
            pipelineInfo.pInputAssemblyState = &input_assembly_create_info;
            pipelineInfo.pViewportState      = &viewport_state_create_info;
            pipelineInfo.pRasterizationState = &rasterization_state_create_info;
            pipelineInfo.pMultisampleState   = &multisample_state_create_info;
            pipelineInfo.pColorBlendState    = &color_blend_state_create_info;
            pipelineInfo.pDepthStencilState  = &depth_stencil_create_info;
            pipelineInfo.layout              = mRenderPipelines[RenderPipelineType_MeshLighting].layout;
            pipelineInfo.renderPass          = mFrameBuffer.renderPass;
            pipelineInfo.subpass             = MAIN_CAMERA_SUBPASS_FORWARD_LIGHTING;
            pipelineInfo.basePipelineHandle  = RHI_NULL_HANDLE;
            pipelineInfo.pDynamicState       = &dynamic_state_create_info;

            if (mRHI->CreateGraphicsPipelines(RHI_NULL_HANDLE,
                1,
                &pipelineInfo,
                mRenderPipelines[RenderPipelineType_MeshLighting].pipeline) !=
                RHI_SUCCESS)
            {
                throw std::runtime_error("create mesh lighting graphics pipeline");
            }

            mRHI->DestroyShaderModule(vert_shader_module);
            mRHI->DestroyShaderModule(frag_shader_module);
        }

        // skybox
        {
            RHIDescriptorSetLayout*      descriptorset_layouts[1] = {mDescInfos[LayoutType_Skybox].layout};
            RHIPipelineLayoutCreateInfo pipeline_layout_create_info {};
            pipeline_layout_create_info.sType          = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = 1;
            pipeline_layout_create_info.pSetLayouts    = descriptorset_layouts;

            if (mRHI->CreatePipelineLayout(&pipeline_layout_create_info, mRenderPipelines[RenderPipelineType_Skybox].layout) != RHI_SUCCESS)
            {
                throw std::runtime_error("create skybox pipeline layout");
            }

            RHIShader* vert_shader_module = mRHI->CreateShaderModule(SKYBOX_VERT);
            RHIShader* frag_shader_module = mRHI->CreateShaderModule(SKYBOX_FRAG);

            RHIPipelineShaderStageCreateInfo vert_pipeline_shader_stage_create_info {};
            vert_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
            vert_pipeline_shader_stage_create_info.module = vert_shader_module;
            vert_pipeline_shader_stage_create_info.pName  = "main";
            // vert_pipeline_shader_stage_create_info.pSpecializationInfo

            RHIPipelineShaderStageCreateInfo frag_pipeline_shader_stage_create_info {};
            frag_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
            frag_pipeline_shader_stage_create_info.module = frag_shader_module;
            frag_pipeline_shader_stage_create_info.pName  = "main";

            RHIPipelineShaderStageCreateInfo shader_stages[] = {vert_pipeline_shader_stage_create_info,
                                                               frag_pipeline_shader_stage_create_info};

            auto vertex_binding_descriptions   = MeshVertex::GetBindingDescriptions();
            auto vertex_attribute_descriptions = MeshVertex::GetAttributeDescriptions();
            RHIPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
            vertex_input_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_create_info.vertexBindingDescriptionCount   = 0;
            vertex_input_state_create_info.pVertexBindingDescriptions      = nullptr;
            vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
            vertex_input_state_create_info.pVertexAttributeDescriptions    = nullptr;

            RHIPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
            input_assembly_create_info.sType    = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_create_info.topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly_create_info.primitiveRestartEnable = RHI_FALSE;

            RHIPipelineViewportStateCreateInfo viewport_state_create_info {};
            viewport_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_create_info.viewportCount = 1;
            viewport_state_create_info.pViewports    = mRHI->GetSwapChainInfo().viewport;
            viewport_state_create_info.scissorCount  = 1;
            viewport_state_create_info.pScissors     = mRHI->GetSwapChainInfo().scissor;

            RHIPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
            rasterization_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterization_state_create_info.depthClampEnable        = RHI_FALSE;
            rasterization_state_create_info.rasterizerDiscardEnable = RHI_FALSE;
            rasterization_state_create_info.polygonMode             = RHI_POLYGON_MODE_FILL;
            rasterization_state_create_info.lineWidth               = 1.0f;
            rasterization_state_create_info.cullMode                = RHI_CULL_MODE_BACK_BIT;
            rasterization_state_create_info.frontFace               = RHI_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterization_state_create_info.depthBiasEnable         = RHI_FALSE;
            rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
            rasterization_state_create_info.depthBiasClamp          = 0.0f;
            rasterization_state_create_info.depthBiasSlopeFactor    = 0.0f;

            RHIPipelineMultisampleStateCreateInfo multisample_state_create_info {};
            multisample_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisample_state_create_info.sampleShadingEnable  = RHI_FALSE;
            multisample_state_create_info.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

            RHIPipelineColorBlendAttachmentState color_blend_attachments[1] = {};
            color_blend_attachments[0].colorWriteMask = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT |
                                                        RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
            color_blend_attachments[0].blendEnable         = RHI_FALSE;
            color_blend_attachments[0].srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachments[0].colorBlendOp        = RHI_BLEND_OP_ADD;
            color_blend_attachments[0].srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachments[0].alphaBlendOp        = RHI_BLEND_OP_ADD;

            RHIPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
            color_blend_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_state_create_info.logicOpEnable = RHI_FALSE;
            color_blend_state_create_info.logicOp       = RHI_LOGIC_OP_COPY;
            color_blend_state_create_info.attachmentCount =
                sizeof(color_blend_attachments) / sizeof(color_blend_attachments[0]);
            color_blend_state_create_info.pAttachments      = &color_blend_attachments[0];
            color_blend_state_create_info.blendConstants[0] = 0.0f;
            color_blend_state_create_info.blendConstants[1] = 0.0f;
            color_blend_state_create_info.blendConstants[2] = 0.0f;
            color_blend_state_create_info.blendConstants[3] = 0.0f;

            RHIPipelineDepthStencilStateCreateInfo depth_stencil_create_info {};
            depth_stencil_create_info.sType            = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_create_info.depthTestEnable  = RHI_TRUE;
            depth_stencil_create_info.depthWriteEnable = RHI_TRUE;
            depth_stencil_create_info.depthCompareOp   = RHI_COMPARE_OP_LESS;
            depth_stencil_create_info.depthBoundsTestEnable = RHI_FALSE;
            depth_stencil_create_info.stencilTestEnable     = RHI_FALSE;

            RHIDynamicState                   dynamic_states[] = {RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR};
            RHIPipelineDynamicStateCreateInfo dynamic_state_create_info {};
            dynamic_state_create_info.sType             = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state_create_info.dynamicStateCount = 2;
            dynamic_state_create_info.pDynamicStates    = dynamic_states;

            RHIGraphicsPipelineCreateInfo pipelineInfo {};
            pipelineInfo.sType               = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount          = 2;
            pipelineInfo.pStages             = shader_stages;
            pipelineInfo.pVertexInputState   = &vertex_input_state_create_info;
            pipelineInfo.pInputAssemblyState = &input_assembly_create_info;
            pipelineInfo.pViewportState      = &viewport_state_create_info;
            pipelineInfo.pRasterizationState = &rasterization_state_create_info;
            pipelineInfo.pMultisampleState   = &multisample_state_create_info;
            pipelineInfo.pColorBlendState    = &color_blend_state_create_info;
            pipelineInfo.pDepthStencilState  = &depth_stencil_create_info;
            pipelineInfo.layout              = mRenderPipelines[RenderPipelineType_Skybox].layout;
            pipelineInfo.renderPass          = mFrameBuffer.renderPass;
            pipelineInfo.subpass             = MAIN_CAMERA_SUBPASS_FORWARD_LIGHTING;
            pipelineInfo.basePipelineHandle  = RHI_NULL_HANDLE;
            pipelineInfo.pDynamicState       = &dynamic_state_create_info;

            if (RHI_SUCCESS != mRHI->CreateGraphicsPipelines(RHI_NULL_HANDLE,
                                                              1,
                                                              &pipelineInfo,
                                                              mRenderPipelines[RenderPipelineType_Skybox].pipeline))
            {
                throw std::runtime_error("create skybox graphics pipeline");
            }

            mRHI->DestroyShaderModule(vert_shader_module);
            mRHI->DestroyShaderModule(frag_shader_module);
        }

        // draw axis
        {
            RHIDescriptorSetLayout*     descriptorset_layouts[1] = {mDescInfos[LayoutType_Axis].layout};
            RHIPipelineLayoutCreateInfo pipeline_layout_create_info {};
            pipeline_layout_create_info.sType          = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = 1;
            pipeline_layout_create_info.pSetLayouts    = descriptorset_layouts;

            if (mRHI->CreatePipelineLayout(&pipeline_layout_create_info, mRenderPipelines[RenderPipelineType_Axis].layout) != RHI_SUCCESS)
            {
                throw std::runtime_error("create axis pipeline layout");
            }

            RHIShader* vert_shader_module = mRHI->CreateShaderModule(AXIS_VERT);
            RHIShader* frag_shader_module = mRHI->CreateShaderModule(AXIS_FRAG);

            RHIPipelineShaderStageCreateInfo vert_pipeline_shader_stage_create_info {};
            vert_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
            vert_pipeline_shader_stage_create_info.module = vert_shader_module;
            vert_pipeline_shader_stage_create_info.pName  = "main";
            // vert_pipeline_shader_stage_create_info.pSpecializationInfo

            RHIPipelineShaderStageCreateInfo frag_pipeline_shader_stage_create_info {};
            frag_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
            frag_pipeline_shader_stage_create_info.module = frag_shader_module;
            frag_pipeline_shader_stage_create_info.pName  = "main";

            RHIPipelineShaderStageCreateInfo shader_stages[] = {vert_pipeline_shader_stage_create_info,
                                                               frag_pipeline_shader_stage_create_info};

            auto vertex_binding_descriptions   = MeshVertex::GetBindingDescriptions();
            auto vertex_attribute_descriptions = MeshVertex::GetAttributeDescriptions();
            RHIPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
            vertex_input_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_create_info.vertexBindingDescriptionCount   = vertex_binding_descriptions.size();
            vertex_input_state_create_info.pVertexBindingDescriptions      = &vertex_binding_descriptions[0];
            vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_attribute_descriptions.size();
            vertex_input_state_create_info.pVertexAttributeDescriptions    = &vertex_attribute_descriptions[0];

            RHIPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
            input_assembly_create_info.sType    = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_create_info.topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly_create_info.primitiveRestartEnable = RHI_FALSE;

            RHIPipelineViewportStateCreateInfo viewport_state_create_info {};
            viewport_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_create_info.viewportCount = 1;
            viewport_state_create_info.pViewports    = mRHI->GetSwapChainInfo().viewport;
            viewport_state_create_info.scissorCount  = 1;
            viewport_state_create_info.pScissors     = mRHI->GetSwapChainInfo().scissor;

            RHIPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
            rasterization_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterization_state_create_info.depthClampEnable        = RHI_FALSE;
            rasterization_state_create_info.rasterizerDiscardEnable = RHI_FALSE;
            rasterization_state_create_info.polygonMode             = RHI_POLYGON_MODE_FILL;
            rasterization_state_create_info.lineWidth               = 1.0f;
            rasterization_state_create_info.cullMode                = RHI_CULL_MODE_NONE;
            rasterization_state_create_info.frontFace               = RHI_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterization_state_create_info.depthBiasEnable         = RHI_FALSE;
            rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
            rasterization_state_create_info.depthBiasClamp          = 0.0f;
            rasterization_state_create_info.depthBiasSlopeFactor    = 0.0f;

            RHIPipelineMultisampleStateCreateInfo multisample_state_create_info {};
            multisample_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisample_state_create_info.sampleShadingEnable  = RHI_FALSE;
            multisample_state_create_info.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

            RHIPipelineColorBlendAttachmentState color_blend_attachment_state {};
            color_blend_attachment_state.colorWriteMask = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT |
                                                          RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
            color_blend_attachment_state.blendEnable         = RHI_FALSE;
            color_blend_attachment_state.srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachment_state.dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachment_state.colorBlendOp        = RHI_BLEND_OP_ADD;
            color_blend_attachment_state.srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachment_state.dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachment_state.alphaBlendOp        = RHI_BLEND_OP_ADD;

            RHIPipelineColorBlendStateCreateInfo color_blend_state_create_info {};
            color_blend_state_create_info.sType             = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_state_create_info.logicOpEnable     = RHI_FALSE;
            color_blend_state_create_info.logicOp           = RHI_LOGIC_OP_COPY;
            color_blend_state_create_info.attachmentCount   = 1;
            color_blend_state_create_info.pAttachments      = &color_blend_attachment_state;
            color_blend_state_create_info.blendConstants[0] = 0.0f;
            color_blend_state_create_info.blendConstants[1] = 0.0f;
            color_blend_state_create_info.blendConstants[2] = 0.0f;
            color_blend_state_create_info.blendConstants[3] = 0.0f;

            RHIPipelineDepthStencilStateCreateInfo depth_stencil_create_info {};
            depth_stencil_create_info.sType            = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_create_info.depthTestEnable  = RHI_FALSE;
            depth_stencil_create_info.depthWriteEnable = RHI_FALSE;
            depth_stencil_create_info.depthCompareOp   = RHI_COMPARE_OP_LESS;
            depth_stencil_create_info.depthBoundsTestEnable = RHI_FALSE;
            depth_stencil_create_info.stencilTestEnable     = RHI_FALSE;

            RHIDynamicState                   dynamic_states[] = {RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR};
            RHIPipelineDynamicStateCreateInfo dynamic_state_create_info {};
            dynamic_state_create_info.sType             = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state_create_info.dynamicStateCount = 2;
            dynamic_state_create_info.pDynamicStates    = dynamic_states;

            RHIGraphicsPipelineCreateInfo pipelineInfo {};
            pipelineInfo.sType               = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount          = 2;
            pipelineInfo.pStages             = shader_stages;
            pipelineInfo.pVertexInputState   = &vertex_input_state_create_info;
            pipelineInfo.pInputAssemblyState = &input_assembly_create_info;
            pipelineInfo.pViewportState      = &viewport_state_create_info;
            pipelineInfo.pRasterizationState = &rasterization_state_create_info;
            pipelineInfo.pMultisampleState   = &multisample_state_create_info;
            pipelineInfo.pColorBlendState    = &color_blend_state_create_info;
            pipelineInfo.pDepthStencilState  = &depth_stencil_create_info;
            pipelineInfo.layout              = mRenderPipelines[RenderPipelineType_Skybox].layout;
            pipelineInfo.renderPass          = mFrameBuffer.renderPass;
            pipelineInfo.subpass             = MAIN_CAMERA_SUBPASS_UI;
            pipelineInfo.basePipelineHandle  = RHI_NULL_HANDLE;
            pipelineInfo.pDynamicState       = &dynamic_state_create_info;

            if (RHI_SUCCESS != mRHI->CreateGraphicsPipelines(RHI_NULL_HANDLE,
                                                              1,
                                                              &pipelineInfo,
                                                              mRenderPipelines[RenderPipelineType_Skybox].pipeline))
            {
                throw std::runtime_error("create axis graphics pipeline");
            }

            mRHI->DestroyShaderModule(vert_shader_module);
            mRHI->DestroyShaderModule(frag_shader_module);
        }
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
        RHIDescriptorImageInfo gbuffer_normal_input_attachment_info = {};
        gbuffer_normal_input_attachment_info.sampler = mRHI->GetOrCreateDefaultSampler(Default_Sampler_Nearest);
        gbuffer_normal_input_attachment_info.imageView   = mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_A].view;
        gbuffer_normal_input_attachment_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo gbuffer_metallic_roughness_shadingmodeid_input_attachment_info = {};
        gbuffer_metallic_roughness_shadingmodeid_input_attachment_info.sampler = mRHI->GetOrCreateDefaultSampler(Default_Sampler_Nearest);
        gbuffer_metallic_roughness_shadingmodeid_input_attachment_info.imageView =
            mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_B].view;
        gbuffer_metallic_roughness_shadingmodeid_input_attachment_info.imageLayout =
            RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo gbuffer_albedo_input_attachment_info = {};
        gbuffer_albedo_input_attachment_info.sampler = mRHI->GetOrCreateDefaultSampler(Default_Sampler_Nearest);
        gbuffer_albedo_input_attachment_info.imageView   = mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_C].view;
        gbuffer_albedo_input_attachment_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo depth_input_attachment_info = {};
        depth_input_attachment_info.sampler = mRHI->GetOrCreateDefaultSampler(Default_Sampler_Nearest);
        depth_input_attachment_info.imageView   = mRHI->GetDepthImageInfo().depthImageView;
        depth_input_attachment_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIWriteDescriptorSet deferred_lighting_descriptor_writes_info[4];

        RHIWriteDescriptorSet& gbuffer_normal_descriptor_input_attachment_write_info =
            deferred_lighting_descriptor_writes_info[0];
        gbuffer_normal_descriptor_input_attachment_write_info.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gbuffer_normal_descriptor_input_attachment_write_info.pNext = nullptr;
        gbuffer_normal_descriptor_input_attachment_write_info.dstSet =
            mDescInfos[LayoutType_DeferredLighting].descriptorSet;
        gbuffer_normal_descriptor_input_attachment_write_info.dstBinding      = 0;
        gbuffer_normal_descriptor_input_attachment_write_info.dstArrayElement = 0;
        gbuffer_normal_descriptor_input_attachment_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        gbuffer_normal_descriptor_input_attachment_write_info.descriptorCount = 1;
        gbuffer_normal_descriptor_input_attachment_write_info.pImageInfo      = &gbuffer_normal_input_attachment_info;

        RHIWriteDescriptorSet& gbuffer_metallic_roughness_shadingmodeid_descriptor_input_attachment_write_info =
            deferred_lighting_descriptor_writes_info[1];
        gbuffer_metallic_roughness_shadingmodeid_descriptor_input_attachment_write_info.sType =
            RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gbuffer_metallic_roughness_shadingmodeid_descriptor_input_attachment_write_info.pNext = nullptr;
        gbuffer_metallic_roughness_shadingmodeid_descriptor_input_attachment_write_info.dstSet =
            mDescInfos[LayoutType_DeferredLighting].descriptorSet;
        gbuffer_metallic_roughness_shadingmodeid_descriptor_input_attachment_write_info.dstBinding      = 1;
        gbuffer_metallic_roughness_shadingmodeid_descriptor_input_attachment_write_info.dstArrayElement = 0;
        gbuffer_metallic_roughness_shadingmodeid_descriptor_input_attachment_write_info.descriptorType =
            RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        gbuffer_metallic_roughness_shadingmodeid_descriptor_input_attachment_write_info.descriptorCount = 1;
        gbuffer_metallic_roughness_shadingmodeid_descriptor_input_attachment_write_info.pImageInfo =
            &gbuffer_metallic_roughness_shadingmodeid_input_attachment_info;

        RHIWriteDescriptorSet& gbuffer_albedo_descriptor_input_attachment_write_info =
            deferred_lighting_descriptor_writes_info[2];
        gbuffer_albedo_descriptor_input_attachment_write_info.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gbuffer_albedo_descriptor_input_attachment_write_info.pNext = nullptr;
        gbuffer_albedo_descriptor_input_attachment_write_info.dstSet =
            mDescInfos[LayoutType_DeferredLighting].descriptorSet;
        gbuffer_albedo_descriptor_input_attachment_write_info.dstBinding      = 2;
        gbuffer_albedo_descriptor_input_attachment_write_info.dstArrayElement = 0;
        gbuffer_albedo_descriptor_input_attachment_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        gbuffer_albedo_descriptor_input_attachment_write_info.descriptorCount = 1;
        gbuffer_albedo_descriptor_input_attachment_write_info.pImageInfo      = &gbuffer_albedo_input_attachment_info;

        RHIWriteDescriptorSet& depth_descriptor_input_attachment_write_info =
            deferred_lighting_descriptor_writes_info[3];
        depth_descriptor_input_attachment_write_info.sType      = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        depth_descriptor_input_attachment_write_info.pNext      = nullptr;
        depth_descriptor_input_attachment_write_info.dstSet     = mDescInfos[LayoutType_DeferredLighting].descriptorSet;
        depth_descriptor_input_attachment_write_info.dstBinding = 3;
        depth_descriptor_input_attachment_write_info.dstArrayElement = 0;
        depth_descriptor_input_attachment_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        depth_descriptor_input_attachment_write_info.descriptorCount = 1;
        depth_descriptor_input_attachment_write_info.pImageInfo      = &depth_input_attachment_info;

        mRHI->UpdateDescriptorSets(sizeof(deferred_lighting_descriptor_writes_info) /
                                    sizeof(deferred_lighting_descriptor_writes_info[0]),
                                    deferred_lighting_descriptor_writes_info,
                                    0,
                                    nullptr);
    }

    void MainCameraPass::setupSwapChainFrameBuffers()
    {
        mSwapChainFrameBuffers.resize(mRHI->GetSwapChainInfo().imageViews.size());

        // create frame buffer for every imageview
        for (size_t i = 0; i < mRHI->GetSwapChainInfo().imageViews.size(); i++)
        {
            RHIImageView* framebuffer_attachments_for_image_view[MAIN_CAMERA_PASS_ATTACHMENT_COUNT] = {
                mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_A].view,
                mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_B].view,
                mFrameBuffer.attachments[MAIN_CAMERA_PASS_GBUFFER_C].view,
                mFrameBuffer.attachments[MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD].view,
                mFrameBuffer.attachments[MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN].view,
                mFrameBuffer.attachments[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_ODD].view,
                mFrameBuffer.attachments[MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_EVEN].view,
                mRHI->GetDepthImageInfo().depthImageView,
                mRHI->GetSwapChainInfo().imageViews[i] };

            RHIFramebufferCreateInfo framebuffer_create_info {};
            framebuffer_create_info.sType      = RHI_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.flags      = 0U;
            framebuffer_create_info.renderPass = mFrameBuffer.renderPass;
            framebuffer_create_info.attachmentCount =
                (sizeof(framebuffer_attachments_for_image_view) / sizeof(framebuffer_attachments_for_image_view[0]));
            framebuffer_create_info.pAttachments = framebuffer_attachments_for_image_view;
            framebuffer_create_info.width        = mRHI->GetSwapChainInfo().extent.width;
            framebuffer_create_info.height       = mRHI->GetSwapChainInfo().extent.height;
            framebuffer_create_info.layers       = 1;

            mSwapChainFrameBuffers[i] = new VulkanFrameBuffer();
            if (RHI_SUCCESS != mRHI->CreateFrameBuffer(&framebuffer_create_info, mSwapChainFrameBuffers[i]))
            {
                throw std::runtime_error("create main camera framebuffer");
            }
        }
    }

    void MainCameraPass::setupModelGlobalDescriptorSet()
    {
        // update common model's global descriptor set
        RHIDescriptorSetAllocateInfo mesh_global_descriptor_set_alloc_info;
        mesh_global_descriptor_set_alloc_info.sType              = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        mesh_global_descriptor_set_alloc_info.pNext              = nullptr;
        mesh_global_descriptor_set_alloc_info.descriptorPool     = mRHI->GetDescriptorPool();
        mesh_global_descriptor_set_alloc_info.descriptorSetCount = 1;
        mesh_global_descriptor_set_alloc_info.pSetLayouts        = &mDescInfos[LayoutType_MeshGlobal].layout;

        if (RHI_SUCCESS != mRHI->AllocateDescriptorSets(&mesh_global_descriptor_set_alloc_info, mDescInfos[LayoutType_MeshGlobal].descriptorSet))
        {
            throw std::runtime_error("allocate mesh global descriptor set");
        }

        RHIDescriptorBufferInfo mesh_perframe_storage_buffer_info = {};
        // this offset plus dynamic_offset should not be greater than the size of the buffer
        mesh_perframe_storage_buffer_info.offset = 0;
        // the range means the size actually used by the shader per draw call
        mesh_perframe_storage_buffer_info.range  = sizeof(MeshPerframeStorageBufferObject);
        mesh_perframe_storage_buffer_info.buffer = mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbuffer;
        assert(mesh_perframe_storage_buffer_info.range <
               mGlobalRenderResource->mStorageBuffer.mMaxStorageBufferRange);

        RHIDescriptorBufferInfo mesh_perdrawcall_storage_buffer_info = {};
        mesh_perdrawcall_storage_buffer_info.offset                 = 0;
        mesh_perdrawcall_storage_buffer_info.range                  = sizeof(MeshPerdrawcallStorageBufferObject);
        mesh_perdrawcall_storage_buffer_info.buffer =
            mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbuffer;
        assert(mesh_perdrawcall_storage_buffer_info.range <
               mGlobalRenderResource->mStorageBuffer.mMaxStorageBufferRange);

        RHIDescriptorBufferInfo mesh_per_drawcall_vertex_blending_storage_buffer_info = {};
        mesh_per_drawcall_vertex_blending_storage_buffer_info.offset                 = 0;
        mesh_per_drawcall_vertex_blending_storage_buffer_info.range =
            sizeof(MeshPerdrawcallVertexBlendingStorageBufferObject);
        mesh_per_drawcall_vertex_blending_storage_buffer_info.buffer =
            mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbuffer;
        assert(mesh_per_drawcall_vertex_blending_storage_buffer_info.range <
               mGlobalRenderResource->mStorageBuffer.mMaxStorageBufferRange);

        RHIDescriptorImageInfo brdf_texture_image_info = {};
        brdf_texture_image_info.sampler     = mGlobalRenderResource->mIBLResource.mBrdfLutTextureSampler;
        brdf_texture_image_info.imageView   = mGlobalRenderResource->mIBLResource.mBrdfLutTextureImageView;
        brdf_texture_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo irradiance_texture_image_info = {};
        irradiance_texture_image_info.sampler = mGlobalRenderResource->mIBLResource.mIrradianceTextureSampler;
        irradiance_texture_image_info.imageView =
            mGlobalRenderResource->mIBLResource.mIrradianceTextureImageView;
        irradiance_texture_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo specular_texture_image_info {};
        specular_texture_image_info.sampler     = mGlobalRenderResource->mIBLResource.mSpecularTextureSampler;
        specular_texture_image_info.imageView   = mGlobalRenderResource->mIBLResource.mSpecularTextureImageView;
        specular_texture_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo directional_light_shadow_texture_image_info{};
        directional_light_shadow_texture_image_info.sampler =
            mRHI->GetOrCreateDefaultSampler(Default_Sampler_Nearest);
        directional_light_shadow_texture_image_info.imageView = mDirectionalLightShadowColorImageView;
        directional_light_shadow_texture_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIWriteDescriptorSet mesh_descriptor_writes_info[8];

        mesh_descriptor_writes_info[0].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mesh_descriptor_writes_info[0].pNext           = nullptr;
        mesh_descriptor_writes_info[0].dstSet          = mDescInfos[LayoutType_MeshGlobal].descriptorSet;
        mesh_descriptor_writes_info[0].dstBinding      = 0;
        mesh_descriptor_writes_info[0].dstArrayElement = 0;
        mesh_descriptor_writes_info[0].descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_descriptor_writes_info[0].descriptorCount = 1;
        mesh_descriptor_writes_info[0].pBufferInfo     = &mesh_perframe_storage_buffer_info;

        mesh_descriptor_writes_info[1].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mesh_descriptor_writes_info[1].pNext           = nullptr;
        mesh_descriptor_writes_info[1].dstSet          = mDescInfos[LayoutType_MeshGlobal].descriptorSet;
        mesh_descriptor_writes_info[1].dstBinding      = 1;
        mesh_descriptor_writes_info[1].dstArrayElement = 0;
        mesh_descriptor_writes_info[1].descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_descriptor_writes_info[1].descriptorCount = 1;
        mesh_descriptor_writes_info[1].pBufferInfo     = &mesh_perdrawcall_storage_buffer_info;

        mesh_descriptor_writes_info[2].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mesh_descriptor_writes_info[2].pNext           = nullptr;
        mesh_descriptor_writes_info[2].dstSet          = mDescInfos[LayoutType_MeshGlobal].descriptorSet;
        mesh_descriptor_writes_info[2].dstBinding      = 2;
        mesh_descriptor_writes_info[2].dstArrayElement = 0;
        mesh_descriptor_writes_info[2].descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_descriptor_writes_info[2].descriptorCount = 1;
        mesh_descriptor_writes_info[2].pBufferInfo     = &mesh_per_drawcall_vertex_blending_storage_buffer_info;

        mesh_descriptor_writes_info[3].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mesh_descriptor_writes_info[3].pNext           = nullptr;
        mesh_descriptor_writes_info[3].dstSet          = mDescInfos[LayoutType_MeshGlobal].descriptorSet;
        mesh_descriptor_writes_info[3].dstBinding      = 3;
        mesh_descriptor_writes_info[3].dstArrayElement = 0;
        mesh_descriptor_writes_info[3].descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        mesh_descriptor_writes_info[3].descriptorCount = 1;
        mesh_descriptor_writes_info[3].pImageInfo      = &brdf_texture_image_info;

        mesh_descriptor_writes_info[4]            = mesh_descriptor_writes_info[3];
        mesh_descriptor_writes_info[4].dstBinding = 4;
        mesh_descriptor_writes_info[4].pImageInfo = &irradiance_texture_image_info;

        mesh_descriptor_writes_info[5]            = mesh_descriptor_writes_info[3];
        mesh_descriptor_writes_info[5].dstBinding = 5;
        mesh_descriptor_writes_info[5].pImageInfo = &specular_texture_image_info;

        mesh_descriptor_writes_info[6]            = mesh_descriptor_writes_info[3];
        mesh_descriptor_writes_info[6].dstBinding = 6;
        mesh_descriptor_writes_info[6].pImageInfo = &directional_light_shadow_texture_image_info;

        mRHI->UpdateDescriptorSets(sizeof(mesh_descriptor_writes_info) / sizeof(mesh_descriptor_writes_info[0]),
                                    mesh_descriptor_writes_info,
                                    0,
                                    nullptr);
    }

    void MainCameraPass::setupSkyboxDescriptorSet()
    {
        RHIDescriptorSetAllocateInfo skybox_descriptor_set_alloc_info;
        skybox_descriptor_set_alloc_info.sType              = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        skybox_descriptor_set_alloc_info.pNext              = nullptr;
        skybox_descriptor_set_alloc_info.descriptorPool     = mRHI->GetDescriptorPool();
        skybox_descriptor_set_alloc_info.descriptorSetCount = 1;
        skybox_descriptor_set_alloc_info.pSetLayouts        = &mDescInfos[LayoutType_Skybox].layout;

        if (RHI_SUCCESS != mRHI->AllocateDescriptorSets(&skybox_descriptor_set_alloc_info, mDescInfos[LayoutType_Skybox].descriptorSet))
        {
            throw std::runtime_error("allocate skybox descriptor set");
        }

        RHIDescriptorBufferInfo mesh_perframe_storage_buffer_info = {};
        mesh_perframe_storage_buffer_info.offset                 = 0;
        mesh_perframe_storage_buffer_info.range                  = sizeof(MeshPerframeStorageBufferObject);
        mesh_perframe_storage_buffer_info.buffer = mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbuffer;
        assert(mesh_perframe_storage_buffer_info.range <
               mGlobalRenderResource->mStorageBuffer.mMaxStorageBufferRange);

        RHIDescriptorImageInfo specular_texture_image_info = {};
        specular_texture_image_info.sampler     = mGlobalRenderResource->mIBLResource.mSpecularTextureSampler;
        specular_texture_image_info.imageView   = mGlobalRenderResource->mIBLResource.mSpecularTextureImageView;
        specular_texture_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIWriteDescriptorSet skybox_descriptor_writes_info[2];

        skybox_descriptor_writes_info[0].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skybox_descriptor_writes_info[0].pNext           = nullptr;
        skybox_descriptor_writes_info[0].dstSet          = mDescInfos[LayoutType_Skybox].descriptorSet;
        skybox_descriptor_writes_info[0].dstBinding      = 0;
        skybox_descriptor_writes_info[0].dstArrayElement = 0;
        skybox_descriptor_writes_info[0].descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        skybox_descriptor_writes_info[0].descriptorCount = 1;
        skybox_descriptor_writes_info[0].pBufferInfo     = &mesh_perframe_storage_buffer_info;

        skybox_descriptor_writes_info[1].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skybox_descriptor_writes_info[1].pNext           = nullptr;
        skybox_descriptor_writes_info[1].dstSet          = mDescInfos[LayoutType_Skybox].descriptorSet;
        skybox_descriptor_writes_info[1].dstBinding      = 1;
        skybox_descriptor_writes_info[1].dstArrayElement = 0;
        skybox_descriptor_writes_info[1].descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skybox_descriptor_writes_info[1].descriptorCount = 1;
        skybox_descriptor_writes_info[1].pImageInfo      = &specular_texture_image_info;

        mRHI->UpdateDescriptorSets(2, skybox_descriptor_writes_info, 0, nullptr);
    }

    void MainCameraPass::setupAxisDescriptorSet()
    {
        RHIDescriptorSetAllocateInfo axis_descriptor_set_alloc_info;
        axis_descriptor_set_alloc_info.sType              = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        axis_descriptor_set_alloc_info.pNext              = nullptr;
        axis_descriptor_set_alloc_info.descriptorPool     = mRHI->GetDescriptorPool();
        axis_descriptor_set_alloc_info.descriptorSetCount = 1;
        axis_descriptor_set_alloc_info.pSetLayouts        = &mDescInfos[LayoutType_Axis].layout;

        if (RHI_SUCCESS != mRHI->AllocateDescriptorSets(&axis_descriptor_set_alloc_info, mDescInfos[LayoutType_Axis].descriptorSet))
        {
            throw std::runtime_error("allocate axis descriptor set");
        }

        RHIDescriptorBufferInfo mesh_perframe_storage_buffer_info = {};
        mesh_perframe_storage_buffer_info.offset                 = 0;
        mesh_perframe_storage_buffer_info.range                  = sizeof(MeshPerframeStorageBufferObject);
        mesh_perframe_storage_buffer_info.buffer = mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbuffer;
        assert(mesh_perframe_storage_buffer_info.range <
               mGlobalRenderResource->mStorageBuffer.mMaxStorageBufferRange);

        RHIDescriptorBufferInfo axis_storage_buffer_info = {};
        axis_storage_buffer_info.offset                 = 0;
        axis_storage_buffer_info.range                  = sizeof(AxisStorageBufferObject);
        axis_storage_buffer_info.buffer = mGlobalRenderResource->mStorageBuffer.mAxisInefficientStorageBuffer;

        RHIWriteDescriptorSet axis_descriptor_writes_info[2];

        axis_descriptor_writes_info[0].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        axis_descriptor_writes_info[0].pNext           = nullptr;
        axis_descriptor_writes_info[0].dstSet          = mDescInfos[LayoutType_Axis].descriptorSet;
        axis_descriptor_writes_info[0].dstBinding      = 0;
        axis_descriptor_writes_info[0].dstArrayElement = 0;
        axis_descriptor_writes_info[0].descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        axis_descriptor_writes_info[0].descriptorCount = 1;
        axis_descriptor_writes_info[0].pBufferInfo     = &mesh_perframe_storage_buffer_info;

        axis_descriptor_writes_info[1].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        axis_descriptor_writes_info[1].pNext           = nullptr;
        axis_descriptor_writes_info[1].dstSet          = mDescInfos[LayoutType_Axis].descriptorSet;
        axis_descriptor_writes_info[1].dstBinding      = 1;
        axis_descriptor_writes_info[1].dstArrayElement = 0;
        axis_descriptor_writes_info[1].descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        axis_descriptor_writes_info[1].descriptorCount = 1;
        axis_descriptor_writes_info[1].pBufferInfo     = &axis_storage_buffer_info;

        mRHI->UpdateDescriptorSets((uint32_t)(sizeof(axis_descriptor_writes_info) / sizeof(axis_descriptor_writes_info[0])),
                                    axis_descriptor_writes_info,
                                    0,
                                    nullptr);
    }

    void MainCameraPass::setupGBufferLightingDescriptorSet()
    {
        RHIDescriptorSetAllocateInfo gbuffer_light_global_descriptor_set_alloc_info;
        gbuffer_light_global_descriptor_set_alloc_info.sType          = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        gbuffer_light_global_descriptor_set_alloc_info.pNext          = nullptr;
        gbuffer_light_global_descriptor_set_alloc_info.descriptorPool = mRHI->GetDescriptorPool();
        gbuffer_light_global_descriptor_set_alloc_info.descriptorSetCount = 1;
        gbuffer_light_global_descriptor_set_alloc_info.pSetLayouts = &mDescInfos[LayoutType_DeferredLighting].layout;

        if (RHI_SUCCESS != mRHI->AllocateDescriptorSets(&gbuffer_light_global_descriptor_set_alloc_info, mDescInfos[LayoutType_DeferredLighting].descriptorSet))
        {
            throw std::runtime_error("allocate gbuffer light global descriptor set");
        }
    }

    void MainCameraPass::drawMeshGBuffer()
    {
        struct MeshNode
        {
            const Matrix4x4* model_matrix {nullptr};
            const Matrix4x4* joint_matrices {nullptr};
            uint32_t         joint_count {0};
        };

        std::map<VulkanPBRMaterial*, std::map<VulkanMesh*, std::vector<MeshNode>>> main_camera_mesh_drawcall_batch;

        // reorganize mesh
        for (RenderMeshNode& node : *(mVisibleNodes.mMainCameraVisibleMeshNodes))
        {
            auto& mesh_instanced = main_camera_mesh_drawcall_batch[node.ref_material];
            auto& mesh_nodes     = mesh_instanced[node.ref_mesh];

            MeshNode temp;
            temp.model_matrix = node.model_matrix;
            if (node.enable_vertex_blending)
            {
                temp.joint_matrices = node.joint_matrices;
                temp.joint_count    = node.joint_count;
            }

            mesh_nodes.push_back(temp);
        }

        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        mRHI->PushEvent(mRHI->GetCurrentCommandBuffer(), "Mesh GBuffer", color);

        mRHI->CmdBindPipelinePFN(mRHI->GetCurrentCommandBuffer(),
                                  RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                  mRenderPipelines[RenderPipelineType_MeshGBuffer].pipeline);
        mRHI->CmdSetViewportPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().viewport);
        mRHI->CmdSetScissorPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().scissor);

        // perframe storage buffer
        uint32_t perframe_dynamic_offset =
            RoundUp(mGlobalRenderResource->mStorageBuffer
                        .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()],
                    mGlobalRenderResource->mStorageBuffer.mMinStorageBufferOffsetAlignment);

        mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] =
            perframe_dynamic_offset + sizeof(MeshPerframeStorageBufferObject);
        assert(mGlobalRenderResource->mStorageBuffer
                   .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] <=
               (mGlobalRenderResource->mStorageBuffer
                    .mGlobalUploadRingbuffersBegin[mRHI->GetCurrentFrameIndex()] +
                mGlobalRenderResource->mStorageBuffer
                    .mGlobalUploadRingbuffersSize[mRHI->GetCurrentFrameIndex()]));

        (*reinterpret_cast<MeshPerframeStorageBufferObject*>(
            reinterpret_cast<uintptr_t>(
                mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbufferMemoryPointer) +
            perframe_dynamic_offset)) = mPerFrameStorageBufferObject;

        for (auto& pair1 : main_camera_mesh_drawcall_batch)
        {
            VulkanPBRMaterial& material       = (*pair1.first);
            auto&              mesh_instanced = pair1.second;

            // bind per material
            mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                                            RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                            mRenderPipelines[RenderPipelineType_MeshGBuffer].layout,
                                            2,
                                            1,
                                            &material.material_descriptor_set,
                                            0,
                                            nullptr);

            // TODO: render from near to far

            for (auto& pair2 : mesh_instanced)
            {
                VulkanMesh& mesh       = (*pair2.first);
                auto&       mesh_nodes = pair2.second;

                uint32_t total_instance_count = static_cast<uint32_t>(mesh_nodes.size());
                if (total_instance_count > 0)
                {
                    // bind per mesh
                    mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                                                    RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                                    mRenderPipelines[RenderPipelineType_MeshGBuffer].layout,
                                                    1,
                                                    1,
                                                    &mesh.mesh_vertex_blending_descriptor_set,
                                                    0,
                                                    nullptr);


                    RHIBuffer* vertex_buffers[] = {mesh.mesh_vertex_position_buffer,
                                                 mesh.mesh_vertex_varying_enable_blending_buffer,
                                                 mesh.mesh_vertex_varying_buffer};
                    RHIDeviceSize offsets[]        = {0, 0, 0};
                    mRHI->CmdBindVertexBuffersPFN(mRHI->GetCurrentCommandBuffer(),
                                                   0,
                                                   (sizeof(vertex_buffers) / sizeof(vertex_buffers[0])),
                                                   vertex_buffers,
                                                   offsets);
                    mRHI->CmdBindIndexBufferPFN(mRHI->GetCurrentCommandBuffer(), mesh.mesh_index_buffer, 0, RHI_INDEX_TYPE_UINT16);

                    uint32_t drawcall_max_instance_count =
                        (sizeof(MeshPerdrawcallStorageBufferObject::mesh_instances) /
                         sizeof(MeshPerdrawcallStorageBufferObject::mesh_instances[0]));
                    uint32_t drawcall_count =
                        RoundUp(total_instance_count, drawcall_max_instance_count) / drawcall_max_instance_count;

                    for (uint32_t drawcall_index = 0; drawcall_index < drawcall_count; ++drawcall_index)
                    {
                        uint32_t current_instance_count =
                            ((total_instance_count - drawcall_max_instance_count * drawcall_index) <
                             drawcall_max_instance_count) ?
                                (total_instance_count - drawcall_max_instance_count * drawcall_index) :
                                drawcall_max_instance_count;

                        // per drawcall storage buffer
                        uint32_t perdrawcall_dynamic_offset =
                            RoundUp(mGlobalRenderResource->mStorageBuffer
                                        .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()],
                                    mGlobalRenderResource->mStorageBuffer.mMinStorageBufferOffsetAlignment);
                        mGlobalRenderResource->mStorageBuffer
                            .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] =
                            perdrawcall_dynamic_offset + sizeof(MeshPerdrawcallStorageBufferObject);
                        assert(mGlobalRenderResource->mStorageBuffer
                                   .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] <=
                               (mGlobalRenderResource->mStorageBuffer
                                    .mGlobalUploadRingbuffersBegin[mRHI->GetCurrentFrameIndex()] +
                                mGlobalRenderResource->mStorageBuffer
                                    .mGlobalUploadRingbuffersSize[mRHI->GetCurrentFrameIndex()]));

                        MeshPerdrawcallStorageBufferObject& perdrawcall_storage_buffer_object =
                            (*reinterpret_cast<MeshPerdrawcallStorageBufferObject*>(
                                reinterpret_cast<uintptr_t>(mGlobalRenderResource->mStorageBuffer
                                                                .mGlobalUploadRingbufferMemoryPointer) +
                                perdrawcall_dynamic_offset));
                        for (uint32_t i = 0; i < current_instance_count; ++i)
                        {
                            perdrawcall_storage_buffer_object.mesh_instances[i].model_matrix =
                                *mesh_nodes[drawcall_max_instance_count * drawcall_index + i].model_matrix;
                            perdrawcall_storage_buffer_object.mesh_instances[i].enable_vertex_blending =
                                mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_matrices ? 1.0 :
                                                                                                              -1.0;
                        }

                        // per drawcall vertex blending storage buffer
                        uint32_t per_drawcall_vertex_blending_dynamic_offset;
                        bool     least_one_enable_vertex_blending = true;
                        for (uint32_t i = 0; i < current_instance_count; ++i)
                        {
                            if (!mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_matrices)
                            {
                                least_one_enable_vertex_blending = false;
                                break;
                            }
                        }
                        if (least_one_enable_vertex_blending)
                        {
                            per_drawcall_vertex_blending_dynamic_offset =
                                RoundUp(mGlobalRenderResource->mStorageBuffer
                                            .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()],
                                        mGlobalRenderResource->mStorageBuffer.mMinStorageBufferOffsetAlignment);
                            mGlobalRenderResource->mStorageBuffer
                                .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] =
                                per_drawcall_vertex_blending_dynamic_offset +
                                sizeof(MeshPerdrawcallVertexBlendingStorageBufferObject);
                            assert(mGlobalRenderResource->mStorageBuffer
                                       .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] <=
                                   (mGlobalRenderResource->mStorageBuffer
                                        .mGlobalUploadRingbuffersBegin[mRHI->GetCurrentFrameIndex()] +
                                    mGlobalRenderResource->mStorageBuffer
                                        .mGlobalUploadRingbuffersSize[mRHI->GetCurrentFrameIndex()]));

                            MeshPerdrawcallVertexBlendingStorageBufferObject&
                                per_drawcall_vertex_blending_storage_buffer_object =
                                    (*reinterpret_cast<MeshPerdrawcallVertexBlendingStorageBufferObject*>(
                                        reinterpret_cast<uintptr_t>(mGlobalRenderResource->mStorageBuffer
                                                                        .mGlobalUploadRingbufferMemoryPointer) +
                                        per_drawcall_vertex_blending_dynamic_offset));
                            for (uint32_t i = 0; i < current_instance_count; ++i)
                            {
                                if (mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_matrices)
                                {
                                    for (uint32_t j = 0;
                                         j < mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_count;
                                         ++j)
                                    {
                                        per_drawcall_vertex_blending_storage_buffer_object
                                            .joint_matrices[s_mesh_vertex_blending_max_joint_count * i + j] =
                                            mesh_nodes[drawcall_max_instance_count * drawcall_index + i]
                                                .joint_matrices[j];
                                    }
                                }
                            }
                        }
                        else
                        {
                            per_drawcall_vertex_blending_dynamic_offset = 0;
                        }

                        // bind perdrawcall
                        uint32_t dynamic_offsets[3] = {perframe_dynamic_offset,
                                                       perdrawcall_dynamic_offset,
                                                       per_drawcall_vertex_blending_dynamic_offset};
                        mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                                                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                                        mRenderPipelines[RenderPipelineType_MeshGBuffer].layout,
                                                        0,
                                                        1,
                                                        &mDescInfos[LayoutType_MeshGlobal].descriptorSet,
                                                        3,
                                                        dynamic_offsets);

                        mRHI->CmdDrawIndexed(mRHI->GetCurrentCommandBuffer(),
                                                 mesh.mesh_index_count,
                                                 current_instance_count,
                                                 0,
                                                 0,
                                                 0);
                    }
                }
            }
        }

        mRHI->PopEvent(mRHI->GetCurrentCommandBuffer());
    }

    void MainCameraPass::drawDeferredLighting()
    {
        mRHI->CmdBindPipelinePFN(mRHI->GetCurrentCommandBuffer(),
            RHI_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderPipelines[RenderPipelineType_DeferredLighting].pipeline);

        mRHI->CmdSetViewportPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().viewport);
        mRHI->CmdSetScissorPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().scissor);

        uint32_t perframe_dynamic_offset =
            RoundUp(mGlobalRenderResource->mStorageBuffer
                        .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()],
                    mGlobalRenderResource->mStorageBuffer.mMinStorageBufferOffsetAlignment);

        mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] =
            perframe_dynamic_offset + sizeof(MeshPerframeStorageBufferObject);
        assert(mGlobalRenderResource->mStorageBuffer
                   .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] <=
               (mGlobalRenderResource->mStorageBuffer
                    .mGlobalUploadRingbuffersBegin[mRHI->GetCurrentFrameIndex()] +
                mGlobalRenderResource->mStorageBuffer
                    .mGlobalUploadRingbuffersSize[mRHI->GetCurrentFrameIndex()]));

        (*reinterpret_cast<MeshPerframeStorageBufferObject*>(
            reinterpret_cast<uintptr_t>(
                mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbufferMemoryPointer) +
            perframe_dynamic_offset)) = mPerFrameStorageBufferObject;

        RHIDescriptorSet* descriptor_sets[3] = {mDescInfos[LayoutType_MeshGlobal].descriptorSet,
                                              mDescInfos[LayoutType_DeferredLighting].descriptorSet,
                                              mDescInfos[LayoutType_Skybox].descriptorSet};
        uint32_t        dynamic_offsets[4] = {perframe_dynamic_offset, perframe_dynamic_offset, 0, 0};
        mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                        mRenderPipelines[RenderPipelineType_DeferredLighting].layout,
                                        0,
                                        3,
                                        descriptor_sets,
                                        4,
                                        dynamic_offsets);

        mRHI->CmdDraw(mRHI->GetCurrentCommandBuffer(), 3, 1, 0, 0);
    }

    void MainCameraPass::drawMeshLighting()
    {
        struct MeshNode
        {
            const Matrix4x4* model_matrix {nullptr};
            const Matrix4x4* joint_matrices {nullptr};
            uint32_t         joint_count {0};
        };

        std::map<VulkanPBRMaterial*, std::map<VulkanMesh*, std::vector<MeshNode>>> main_camera_mesh_drawcall_batch;

        // reorganize mesh
        for (RenderMeshNode& node : *(mVisibleNodes.mMainCameraVisibleMeshNodes))
        {
            auto& mesh_instanced = main_camera_mesh_drawcall_batch[node.ref_material];
            auto& mesh_nodes     = mesh_instanced[node.ref_mesh];

            MeshNode temp;
            temp.model_matrix = node.model_matrix;
            if (node.enable_vertex_blending)
            {
                temp.joint_matrices = node.joint_matrices;
                temp.joint_count    = node.joint_count;
            }

            mesh_nodes.push_back(temp);
        }

        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        mRHI->PushEvent(mRHI->GetCurrentCommandBuffer(), "Model", color);

        mRHI->CmdBindPipelinePFN(mRHI->GetCurrentCommandBuffer(),
                                  RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                  mRenderPipelines[RenderPipelineType_MeshLighting].pipeline);
        mRHI->CmdSetViewportPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().viewport);
        mRHI->CmdSetScissorPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().scissor);

        // perframe storage buffer
        uint32_t perframe_dynamic_offset =
            RoundUp(mGlobalRenderResource->mStorageBuffer
                        .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()],
                    mGlobalRenderResource->mStorageBuffer.mMinStorageBufferOffsetAlignment);

        mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] =
            perframe_dynamic_offset + sizeof(MeshPerframeStorageBufferObject);
        assert(mGlobalRenderResource->mStorageBuffer
                   .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] <=
               (mGlobalRenderResource->mStorageBuffer
                    .mGlobalUploadRingbuffersBegin[mRHI->GetCurrentFrameIndex()] +
                mGlobalRenderResource->mStorageBuffer
                    .mGlobalUploadRingbuffersSize[mRHI->GetCurrentFrameIndex()]));

        (*reinterpret_cast<MeshPerframeStorageBufferObject*>(
            reinterpret_cast<uintptr_t>(
                mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbufferMemoryPointer) +
            perframe_dynamic_offset)) = mPerFrameStorageBufferObject;

        for (auto& pair1 : main_camera_mesh_drawcall_batch)
        {
            VulkanPBRMaterial& material       = (*pair1.first);
            auto&              mesh_instanced = pair1.second;

            // bind per material
            mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                                            RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                            mRenderPipelines[RenderPipelineType_MeshLighting].layout,
                                            2,
                                            1,
                                            &material.material_descriptor_set,
                                            0,
                                            nullptr);

            // TODO: render from near to far

            for (auto& pair2 : mesh_instanced)
            {
                VulkanMesh& mesh       = (*pair2.first);
                auto&       mesh_nodes = pair2.second;

                uint32_t total_instance_count = static_cast<uint32_t>(mesh_nodes.size());
                if (total_instance_count > 0)
                {
                    // bind per mesh
                    mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                                                    RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                                    mRenderPipelines[RenderPipelineType_MeshLighting].layout,
                                                    1,
                                                    1,
                                                    &mesh.mesh_vertex_blending_descriptor_set,
                                                    0,
                                                    nullptr);

                    RHIBuffer*     vertex_buffers[3] = {mesh.mesh_vertex_position_buffer,
                                                 mesh.mesh_vertex_varying_enable_blending_buffer,
                                                 mesh.mesh_vertex_varying_buffer};
                    RHIDeviceSize offsets[]        = {0, 0, 0};
                    mRHI->CmdBindVertexBuffersPFN(mRHI->GetCurrentCommandBuffer(),
                                                   0,
                                                   (sizeof(vertex_buffers) / sizeof(vertex_buffers[0])),
                                                   vertex_buffers,
                                                   offsets);
                    mRHI->CmdBindIndexBufferPFN(mRHI->GetCurrentCommandBuffer(), mesh.mesh_index_buffer, 0, RHI_INDEX_TYPE_UINT16);

                    uint32_t drawcall_max_instance_count =
                        (sizeof(MeshPerdrawcallStorageBufferObject::mesh_instances) /
                         sizeof(MeshPerdrawcallStorageBufferObject::mesh_instances[0]));
                    uint32_t drawcall_count =
                        RoundUp(total_instance_count, drawcall_max_instance_count) / drawcall_max_instance_count;

                    for (uint32_t drawcall_index = 0; drawcall_index < drawcall_count; ++drawcall_index)
                    {
                        uint32_t current_instance_count =
                            ((total_instance_count - drawcall_max_instance_count * drawcall_index) <
                             drawcall_max_instance_count) ?
                                (total_instance_count - drawcall_max_instance_count * drawcall_index) :
                                drawcall_max_instance_count;

                        // per drawcall storage buffer
                        uint32_t perdrawcall_dynamic_offset =
                            RoundUp(mGlobalRenderResource->mStorageBuffer
                                        .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()],
                                    mGlobalRenderResource->mStorageBuffer.mMinStorageBufferOffsetAlignment);
                        mGlobalRenderResource->mStorageBuffer
                            .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] =
                            perdrawcall_dynamic_offset + sizeof(MeshPerdrawcallStorageBufferObject);
                        assert(mGlobalRenderResource->mStorageBuffer
                                   .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] <=
                               (mGlobalRenderResource->mStorageBuffer
                                    .mGlobalUploadRingbuffersBegin[mRHI->GetCurrentFrameIndex()] +
                                mGlobalRenderResource->mStorageBuffer
                                    .mGlobalUploadRingbuffersSize[mRHI->GetCurrentFrameIndex()]));

                        MeshPerdrawcallStorageBufferObject& perdrawcall_storage_buffer_object =
                            (*reinterpret_cast<MeshPerdrawcallStorageBufferObject*>(
                                reinterpret_cast<uintptr_t>(mGlobalRenderResource->mStorageBuffer
                                                                .mGlobalUploadRingbufferMemoryPointer) +
                                perdrawcall_dynamic_offset));
                        for (uint32_t i = 0; i < current_instance_count; ++i)
                        {
                            perdrawcall_storage_buffer_object.mesh_instances[i].model_matrix =
                                *mesh_nodes[drawcall_max_instance_count * drawcall_index + i].model_matrix;
                            perdrawcall_storage_buffer_object.mesh_instances[i].enable_vertex_blending =
                                mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_matrices ? 1.0 :
                                                                                                              -1.0;
                        }

                        // per drawcall vertex blending storage buffer
                        uint32_t per_drawcall_vertex_blending_dynamic_offset;
                        bool     least_one_enable_vertex_blending = true;
                        for (uint32_t i = 0; i < current_instance_count; ++i)
                        {
                            if (!mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_matrices)
                            {
                                least_one_enable_vertex_blending = false;
                                break;
                            }
                        }
                        if (least_one_enable_vertex_blending)
                        {
                            per_drawcall_vertex_blending_dynamic_offset =
                                RoundUp(mGlobalRenderResource->mStorageBuffer
                                            .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()],
                                        mGlobalRenderResource->mStorageBuffer.mMinStorageBufferOffsetAlignment);
                            mGlobalRenderResource->mStorageBuffer
                                .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] =
                                per_drawcall_vertex_blending_dynamic_offset +
                                sizeof(MeshPerdrawcallVertexBlendingStorageBufferObject);
                            assert(mGlobalRenderResource->mStorageBuffer
                                       .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] <=
                                   (mGlobalRenderResource->mStorageBuffer
                                        .mGlobalUploadRingbuffersBegin[mRHI->GetCurrentFrameIndex()] +
                                    mGlobalRenderResource->mStorageBuffer
                                        .mGlobalUploadRingbuffersSize[mRHI->GetCurrentFrameIndex()]));

                            MeshPerdrawcallVertexBlendingStorageBufferObject&
                                per_drawcall_vertex_blending_storage_buffer_object =
                                    (*reinterpret_cast<MeshPerdrawcallVertexBlendingStorageBufferObject*>(
                                        reinterpret_cast<uintptr_t>(mGlobalRenderResource->mStorageBuffer
                                                                        .mGlobalUploadRingbufferMemoryPointer) +
                                        per_drawcall_vertex_blending_dynamic_offset));
                            for (uint32_t i = 0; i < current_instance_count; ++i)
                            {
                                if (mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_matrices)
                                {
                                    for (uint32_t j = 0;
                                         j < mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_count;
                                         ++j)
                                    {
                                        per_drawcall_vertex_blending_storage_buffer_object
                                            .joint_matrices[s_mesh_vertex_blending_max_joint_count * i + j] =
                                            mesh_nodes[drawcall_max_instance_count * drawcall_index + i]
                                                .joint_matrices[j];
                                    }
                                }
                            }
                        }
                        else
                        {
                            per_drawcall_vertex_blending_dynamic_offset = 0;
                        }

                        // bind perdrawcall
                        uint32_t dynamic_offsets[3] = {perframe_dynamic_offset,
                                                       perdrawcall_dynamic_offset,
                                                       per_drawcall_vertex_blending_dynamic_offset};
                        mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                                                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                                        mRenderPipelines[RenderPipelineType_MeshLighting].layout,
                                                        0,
                                                        1,
                                                        &mDescInfos[LayoutType_MeshGlobal].descriptorSet,
                                                        3,
                                                        dynamic_offsets);

                        mRHI->CmdDrawIndexed(mRHI->GetCurrentCommandBuffer(),
                                                 mesh.mesh_index_count,
                                                 current_instance_count,
                                                 0,
                                                 0,
                                                 0);
                    }
                }
            }
        }

        mRHI->PopEvent(mRHI->GetCurrentCommandBuffer());
    }

    void MainCameraPass::drawSkybox()
    {
        uint32_t perframe_dynamic_offset =
            RoundUp(mGlobalRenderResource->mStorageBuffer
                        .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()],
                    mGlobalRenderResource->mStorageBuffer.mMinStorageBufferOffsetAlignment);

        mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] =
            perframe_dynamic_offset + sizeof(MeshPerframeStorageBufferObject);
        assert(mGlobalRenderResource->mStorageBuffer
                   .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] <=
               (mGlobalRenderResource->mStorageBuffer
                    .mGlobalUploadRingbuffersBegin[mRHI->GetCurrentFrameIndex()] +
                mGlobalRenderResource->mStorageBuffer
                    .mGlobalUploadRingbuffersSize[mRHI->GetCurrentFrameIndex()]));

        (*reinterpret_cast<MeshPerframeStorageBufferObject*>(
            reinterpret_cast<uintptr_t>(
                mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbufferMemoryPointer) +
            perframe_dynamic_offset)) = mPerFrameStorageBufferObject;

        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        mRHI->PushEvent(mRHI->GetCurrentCommandBuffer(), "Skybox", color);

        mRHI->CmdBindPipelinePFN(mRHI->GetCurrentCommandBuffer(),
                                  RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                  mRenderPipelines[RenderPipelineType_Skybox].pipeline);
        mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                        mRenderPipelines[RenderPipelineType_Skybox].layout,
                                        0,
                                        1,
                                        &mDescInfos[LayoutType_Skybox].descriptorSet,
                                        1,
                                        &perframe_dynamic_offset);
        mRHI->CmdDraw(mRHI->GetCurrentCommandBuffer(), 36, 1, 0, 0); // 2 triangles(6 vertex) each face, 6 faces

        mRHI->PopEvent(mRHI->GetCurrentCommandBuffer());
    }

    void MainCameraPass::drawAxis()
    {
        if (!mbIsShowAxis)
            return;

        uint32_t perframe_dynamic_offset =
            RoundUp(mGlobalRenderResource->mStorageBuffer
                        .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()],
                    mGlobalRenderResource->mStorageBuffer.mMinStorageBufferOffsetAlignment);

        mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] =
            perframe_dynamic_offset + sizeof(MeshPerframeStorageBufferObject);
        assert(mGlobalRenderResource->mStorageBuffer
                   .mGlobalUploadRingbuffersEnd[mRHI->GetCurrentFrameIndex()] <=
               (mGlobalRenderResource->mStorageBuffer
                    .mGlobalUploadRingbuffersBegin[mRHI->GetCurrentFrameIndex()] +
                mGlobalRenderResource->mStorageBuffer
                    .mGlobalUploadRingbuffersSize[mRHI->GetCurrentFrameIndex()]));

        (*reinterpret_cast<MeshPerframeStorageBufferObject*>(
            reinterpret_cast<uintptr_t>(
                mGlobalRenderResource->mStorageBuffer.mGlobalUploadRingbufferMemoryPointer) +
            perframe_dynamic_offset)) = mPerFrameStorageBufferObject;

        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        mRHI->PushEvent(mRHI->GetCurrentCommandBuffer(), "Axis", color);

        mRHI->CmdBindPipelinePFN(mRHI->GetCurrentCommandBuffer(),
                                  RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                  mRenderPipelines[RenderPipelineType_Axis].pipeline);
        mRHI->CmdSetViewportPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().viewport);
        mRHI->CmdSetScissorPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().scissor);
        mRHI->CmdBindDescriptorSetsPFN(mRHI->GetCurrentCommandBuffer(),
                                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                        mRenderPipelines[RenderPipelineType_Axis].layout,
                                        0,
                                        1,
                                        &mDescInfos[LayoutType_Axis].descriptorSet,
                                        1,
                                        &perframe_dynamic_offset);

        mAxisStorageBufferObject.selected_axis = mSelectedAxis;
        mAxisStorageBufferObject.model_matrix  = mVisibleNodes.mAxisNode->model_matrix;

        RHIBuffer*     vertex_buffers[3] = {mVisibleNodes.mAxisNode->ref_mesh->mesh_vertex_position_buffer,
                                     mVisibleNodes.mAxisNode->ref_mesh->mesh_vertex_varying_enable_blending_buffer,
                                     mVisibleNodes.mAxisNode->ref_mesh->mesh_vertex_varying_buffer};
        RHIDeviceSize offsets[3]        = {0, 0, 0};
        mRHI->CmdBindVertexBuffersPFN(mRHI->GetCurrentCommandBuffer(),
                                       0,
                                       (sizeof(vertex_buffers) / sizeof(vertex_buffers[0])),
                                       vertex_buffers,
                                       offsets);
        mRHI->CmdBindIndexBufferPFN(mRHI->GetCurrentCommandBuffer(),
                                     mVisibleNodes.mAxisNode->ref_mesh->mesh_index_buffer,
                                     0,
                                     RHI_INDEX_TYPE_UINT16);
        (*reinterpret_cast<AxisStorageBufferObject*>(reinterpret_cast<uintptr_t>(
            mGlobalRenderResource->mStorageBuffer.mAxisInefficientStorageBufferMemoryPointer))) =
            mAxisStorageBufferObject;

        mRHI->CmdDrawIndexed(mRHI->GetCurrentCommandBuffer(),
                                 mVisibleNodes.mAxisNode->ref_mesh->mesh_index_count,
                                 1,
                                 0,
                                 0,
                                 0);

        mRHI->PopEvent(mRHI->GetCurrentCommandBuffer());
    }

} // namespace MiniEngine