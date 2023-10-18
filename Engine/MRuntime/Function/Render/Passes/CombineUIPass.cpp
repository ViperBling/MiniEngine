#include "CombineUIPass.hpp"

#include "MRuntime/Function/Render/Interface/Vulkan/VulkanRHI.hpp"
#include "MRuntime/Function/Render/Interface/Vulkan/VulkanUtil.hpp"

#include <CombineUI_frag.h>
#include <PostProcess_vert.h>

#include <stdexcept>

namespace MiniEngine
{
    void CombineUIPass::Initialize(const RenderPassInitInfo *initInfo)
    {
        RenderPass::Initialize(nullptr);

        const CombineUIPassInitInfo* combineUIInitInfo = static_cast<const CombineUIPassInitInfo*>(initInfo);
        mFrameBuffer.renderPass = combineUIInitInfo->mRenderPass;

        setupDescriptorSetLayout();
        setupPipelines();
        setupDescriptorSet();
        UpdateAfterFramebufferRecreate(combineUIInitInfo->mSceneInputAttachment, combineUIInitInfo->mUIInputAttachment);
    }

    void CombineUIPass::Draw()
    {
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        mRHI->PushEvent(mRHI->GetCurrentCommandBuffer(), "Combine UI", color);

        RHIViewport viewport = {
            0.0, 0.0,
            static_cast<float>(mRHI->GetSwapChainInfo().extent.width),
            static_cast<float>(mRHI->GetSwapChainInfo().extent.height),
            0.0, 1.0
        };
        RHIRect2D scissor = {
            0, 0,
            mRHI->GetSwapChainInfo().extent.width,
            mRHI->GetSwapChainInfo().extent.height
        };
        mRHI->CmdBindPipelinePFN(mRHI->GetCurrentCommandBuffer(), RHI_PIPELINE_BIND_POINT_GRAPHICS, mRenderPipelines[0].pipeline);
        mRHI->CmdSetViewportPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, &viewport);
        mRHI->CmdSetScissorPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, &scissor);
        mRHI->CmdBindDescriptorSetsPFN(
            mRHI->GetCurrentCommandBuffer(),
            RHI_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderPipelines[0].layout,
            0, 1,
            &mDescInfos[0].descriptorSet, 
            0, nullptr
        );
        mRHI->CmdDraw(mRHI->GetCurrentCommandBuffer(), 3, 1, 0, 0);

        mRHI->PopEvent(mRHI->GetCurrentCommandBuffer());
    }

    void CombineUIPass::UpdateAfterFramebufferRecreate(RHIImageView *sceneInputAttachment, RHIImageView *uiInputAttachment)
    {
        RHIDescriptorImageInfo perFrameSceneInputAttachInfo {};
        perFrameSceneInputAttachInfo.sampler = mRHI->GetOrCreateDefaultSampler(Default_Sampler_Nearest);
        perFrameSceneInputAttachInfo.imageView   = sceneInputAttachment;
        perFrameSceneInputAttachInfo.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo perFrameUIInputAttachInfo {};
        perFrameUIInputAttachInfo.sampler     = mRHI->GetOrCreateDefaultSampler(Default_Sampler_Nearest);
        perFrameUIInputAttachInfo.imageView   = uiInputAttachment;
        perFrameUIInputAttachInfo.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        std::array<RHIWriteDescriptorSet, 2> ppDescWriteInfo;

        RHIWriteDescriptorSet& perFrameSceneInputAttachWriteInfo = ppDescWriteInfo[0];
        perFrameSceneInputAttachWriteInfo.pNext                 = nullptr;
        perFrameSceneInputAttachWriteInfo.dstSet                = mDescInfos[0].descriptorSet;
        perFrameSceneInputAttachWriteInfo.dstBinding            = 0;
        perFrameSceneInputAttachWriteInfo.dstArrayElement       = 0;
        perFrameSceneInputAttachWriteInfo.descriptorType        = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        perFrameSceneInputAttachWriteInfo.descriptorCount       = 1;
        perFrameSceneInputAttachWriteInfo.pImageInfo            = &perFrameSceneInputAttachInfo;

        RHIWriteDescriptorSet& perFrameUIInputAttachWriteInfo   = ppDescWriteInfo[1];
        perFrameUIInputAttachWriteInfo.sType                    = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        perFrameUIInputAttachWriteInfo.pNext                    = nullptr;
        perFrameUIInputAttachWriteInfo.dstSet                   = mDescInfos[0].descriptorSet;
        perFrameUIInputAttachWriteInfo.dstBinding               = 1;
        perFrameUIInputAttachWriteInfo.dstArrayElement          = 0;
        perFrameUIInputAttachWriteInfo.descriptorType           = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        perFrameUIInputAttachWriteInfo.descriptorCount          = 1;
        perFrameUIInputAttachWriteInfo.pImageInfo               = &perFrameUIInputAttachInfo;

        mRHI->UpdateDescriptorSets(
            static_cast<uint32_t>(ppDescWriteInfo.size()),
            ppDescWriteInfo.data(),
            0,
            nullptr);
    }

    void CombineUIPass::setupDescriptorSetLayout()
    {
        mDescInfos.resize(1);
        std::array<RHIDescriptorSetLayoutBinding, 2> ppDescLayoutBindings;

        for (uint32_t i = 0; i < 2; i++)
        {
            ppDescLayoutBindings[i].binding = i;
            ppDescLayoutBindings[i].descriptorType = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            ppDescLayoutBindings[i].descriptorCount = 1;
            ppDescLayoutBindings[i].stageFlags = RHI_SHADER_STAGE_FRAGMENT_BIT;
        }

        RHIDescriptorSetLayoutCreateInfo ppGlobalDescLayoutCI;
        ppGlobalDescLayoutCI.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ppGlobalDescLayoutCI.pNext = nullptr;
        ppGlobalDescLayoutCI.flags = 0;
        ppGlobalDescLayoutCI.bindingCount = static_cast<uint32_t>(ppDescLayoutBindings.size());
        ppGlobalDescLayoutCI.pBindings = ppDescLayoutBindings.data();

        if (RHI_SUCCESS != mRHI->CreateDescriptorSetLayout(&ppGlobalDescLayoutCI, mDescInfos[0].layout))
        {
            throw std::runtime_error("Create Combine UI Global Layout");
        }
    }

    void CombineUIPass::setupPipelines()
    {
        mRenderPipelines.resize(1);

        std::vector<RHIDescriptorSetLayout*> descSetLayout = { mDescInfos[0].layout };
        RHIPipelineLayoutCreateInfo pipelineLayoutCI {};
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts    = descSetLayout.data();
        if (RHI_SUCCESS != mRHI->CreatePipelineLayout(&pipelineLayoutCI, mRenderPipelines[0].layout))
        {
            throw std::runtime_error("Create Combine UI Pipeline Layout");
        }

        RHIShader* vsShader = mRHI->CreateShaderModule(POSTPROCESS_VERT);
        RHIShader* psShader = mRHI->CreateShaderModule(COMBINEUI_FRAG);
        RHIPipelineShaderStageCreateInfo vsStageCI {};
        vsStageCI.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
        vsStageCI.module = vsShader;
        vsStageCI.pName  = "main";
        RHIPipelineShaderStageCreateInfo psStageCI {};
        psStageCI.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
        psStageCI.module = psShader;
        psStageCI.pName  = "main";
        RHIPipelineShaderStageCreateInfo ssStage[] = { vsStageCI, psStageCI };

        RHIPipelineVertexInputStateCreateInfo viStateCI {};
        viStateCI.vertexBindingDescriptionCount   = 0;
        viStateCI.pVertexBindingDescriptions      = nullptr;
        viStateCI.vertexAttributeDescriptionCount = 0;
        viStateCI.pVertexAttributeDescriptions    = nullptr;

        RHIPipelineInputAssemblyStateCreateInfo iaStateCI {};
        iaStateCI.topology               = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        iaStateCI.primitiveRestartEnable = RHI_FALSE;

        RHIPipelineViewportStateCreateInfo vpStateCI {};
        vpStateCI.viewportCount = 1;
        vpStateCI.pViewports    = mRHI->GetSwapChainInfo().viewport;
        vpStateCI.scissorCount  = 1;
        vpStateCI.pScissors     = mRHI->GetSwapChainInfo().scissor;

        RHIPipelineRasterizationStateCreateInfo rsStateCI {};
        rsStateCI.depthClampEnable        = RHI_FALSE;
        rsStateCI.rasterizerDiscardEnable = RHI_FALSE;
        rsStateCI.polygonMode             = RHI_POLYGON_MODE_FILL;
        rsStateCI.lineWidth               = 1.0f;
        rsStateCI.cullMode                = RHI_CULL_MODE_BACK_BIT;
        rsStateCI.frontFace               = RHI_FRONT_FACE_CLOCKWISE;
        rsStateCI.depthBiasEnable         = RHI_FALSE;
        rsStateCI.depthBiasConstantFactor = 0.0f;
        rsStateCI.depthBiasClamp          = 0.0f;
        rsStateCI.depthBiasSlopeFactor    = 0.0f;

        RHIPipelineMultisampleStateCreateInfo msStateCI {};
        msStateCI.sampleShadingEnable  = RHI_FALSE;
        msStateCI.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

        RHIPipelineColorBlendAttachmentState cbAttachState {};
        cbAttachState.colorWriteMask      = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT | RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
        cbAttachState.blendEnable         = RHI_FALSE;
        cbAttachState.srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
        cbAttachState.dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
        cbAttachState.colorBlendOp        = RHI_BLEND_OP_ADD;
        cbAttachState.srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
        cbAttachState.dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
        cbAttachState.alphaBlendOp        = RHI_BLEND_OP_ADD;

        RHIPipelineColorBlendStateCreateInfo cbStateCI {};
        cbStateCI.logicOpEnable     = RHI_FALSE;
        cbStateCI.logicOp           = RHI_LOGIC_OP_COPY;
        cbStateCI.attachmentCount   = 1;
        cbStateCI.pAttachments      = &cbAttachState;
        cbStateCI.blendConstants[0] = 0.0f;
        cbStateCI.blendConstants[1] = 0.0f;
        cbStateCI.blendConstants[2] = 0.0f;
        cbStateCI.blendConstants[3] = 0.0f;

        RHIPipelineDepthStencilStateCreateInfo dsStateCI {};
        dsStateCI.depthTestEnable       = RHI_TRUE;
        dsStateCI.depthWriteEnable      = RHI_TRUE;
        dsStateCI.depthCompareOp        = RHI_COMPARE_OP_LESS;
        dsStateCI.depthBoundsTestEnable = RHI_FALSE;
        dsStateCI.stencilTestEnable     = RHI_FALSE;

        RHIDynamicState dyState[] = {RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR};
        RHIPipelineDynamicStateCreateInfo dyStateCI {};
        dyStateCI.dynamicStateCount = 2;
        dyStateCI.pDynamicStates    = dyState;

        RHIGraphicsPipelineCreateInfo pipelineCI {};
        pipelineCI.stageCount          = 2;
        pipelineCI.pStages             = ssStage;
        pipelineCI.pVertexInputState   = &viStateCI;
        pipelineCI.pInputAssemblyState = &iaStateCI;
        pipelineCI.pViewportState      = &vpStateCI;
        pipelineCI.pRasterizationState = &rsStateCI;
        pipelineCI.pMultisampleState   = &msStateCI;
        pipelineCI.pColorBlendState    = &cbStateCI;
        pipelineCI.pDepthStencilState  = &dsStateCI;
        pipelineCI.layout              = mRenderPipelines[0].layout;
        pipelineCI.renderPass          = mFrameBuffer.renderPass;
        pipelineCI.subpass             = MAIN_CAMERA_SUBPASS_COMBINE_UI;
        pipelineCI.basePipelineHandle  = RHI_NULL_HANDLE;
        pipelineCI.pDynamicState       = &dyStateCI;

        if (RHI_SUCCESS != mRHI->CreateGraphicsPipelines(RHI_NULL_HANDLE, 1, &pipelineCI, mRenderPipelines[0].pipeline))
        {
            throw std::runtime_error("create post process graphics pipeline");
        }

        mRHI->DestroyShaderModule(vsShader);
        mRHI->DestroyShaderModule(psShader);
    }

    void CombineUIPass::setupDescriptorSet()
    {
        RHIDescriptorSetAllocateInfo ppGlobalDescSetAI {};
        ppGlobalDescSetAI.pNext              = nullptr;
        ppGlobalDescSetAI.descriptorPool     = mRHI->GetDescriptorPool();
        ppGlobalDescSetAI.descriptorSetCount = 1;
        ppGlobalDescSetAI.pSetLayouts        = &mDescInfos[0].layout;

        if (RHI_SUCCESS != mRHI->AllocateDescriptorSets(&ppGlobalDescSetAI, mDescInfos[0].descriptorSet))
        {
            throw std::runtime_error("allocate post process global descriptor set");
        }
    }

} // namespace MiniEngine