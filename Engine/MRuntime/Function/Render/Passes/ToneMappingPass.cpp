#include "ToneMappingPass.hpp"

#include "MRuntime/Function/Render/Interface/Vulkan/VulkanRHI.hpp"
#include "MRuntime/Function/Render/Interface/Vulkan/VulkanUtil.hpp"

#include <Tonemapping_frag.h>
#include <PostProcess_vert.h>

#include <stdexcept>

namespace MiniEngine
{
    void ToneMappingPass::Initialize(const RenderPassInitInfo *initInfo)
    {
        RenderPass::Initialize(initInfo);

        const ToneMappingPassInitInfo* tmInitInfo = static_cast<const ToneMappingPassInitInfo*>(initInfo);
        mFrameBuffer.renderPass = tmInitInfo->mRenderPass;

        setupDescriptorSetLayout();
        setupPipelines();
        setupDescriptorSet();
        UpdateAfterFramebufferRecreate(tmInitInfo->mInputAttachment);
    }

    void ToneMappingPass::Draw()
    {
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        mRHI->PushEvent(mRHI->GetCurrentCommandBuffer(), "Tone Mapping", color);

        mRHI->CmdBindPipelinePFN(mRHI->GetCurrentCommandBuffer(), RHI_PIPELINE_BIND_POINT_GRAPHICS, mRenderPipelines[0].pipeline);
        mRHI->CmdSetViewportPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().viewport);
        mRHI->CmdSetScissorPFN(mRHI->GetCurrentCommandBuffer(), 0, 1, mRHI->GetSwapChainInfo().scissor);
        mRHI->CmdBindDescriptorSetsPFN(
            mRHI->GetCurrentCommandBuffer(),
            RHI_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderPipelines[0].layout,
            0, 1,
            &mDescInfos[0].descriptorSet,
            0, nullptr);
        mRHI->CmdDraw(mRHI->GetCurrentCommandBuffer(), 3, 1, 0, 0);
        mRHI->PopEvent(mRHI->GetCurrentCommandBuffer());
    }

    void ToneMappingPass::UpdateAfterFramebufferRecreate(RHIImageView *inputAttachment)
    {
        RHIDescriptorImageInfo postProcessInputAttachImage {};
        postProcessInputAttachImage.sampler     = mRHI->GetOrCreateDefaultSampler(Default_Sampler_Nearest);
        postProcessInputAttachImage.imageView   = inputAttachment;
        postProcessInputAttachImage.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIWriteDescriptorSet postProcessDescWriteInfo[1];

        RHIWriteDescriptorSet& postProcessDescSet = postProcessDescWriteInfo[0];
        postProcessDescSet.sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        postProcessDescSet.pNext           = nullptr;
        postProcessDescSet.dstSet          = mDescInfos[0].descriptorSet;
        postProcessDescSet.dstBinding      = 0;
        postProcessDescSet.dstArrayElement = 0;
        postProcessDescSet.descriptorType  = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        postProcessDescSet.descriptorCount = 1;
        postProcessDescSet.pImageInfo      = &postProcessInputAttachImage;

        mRHI->UpdateDescriptorSets(
            sizeof(postProcessDescWriteInfo) / sizeof(postProcessDescWriteInfo[0]),
            postProcessDescWriteInfo,
            0,
            nullptr);
    }

    void ToneMappingPass::setupDescriptorSetLayout()
    {
        mDescInfos.resize(1);

        RHIDescriptorSetLayoutBinding postProcessGlobalDescSetLayoutBinding {};
        postProcessGlobalDescSetLayoutBinding.binding         = 0;
        postProcessGlobalDescSetLayoutBinding.descriptorType  = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        postProcessGlobalDescSetLayoutBinding.descriptorCount = 1;
        postProcessGlobalDescSetLayoutBinding.stageFlags      = RHI_SHADER_STAGE_FRAGMENT_BIT;

        RHIDescriptorSetLayoutCreateInfo postProcessDescSetLayoutCI {};
        postProcessDescSetLayoutCI.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        postProcessDescSetLayoutCI.pNext = nullptr;
        postProcessDescSetLayoutCI.flags = 0;
        postProcessDescSetLayoutCI.bindingCount = 1;
        postProcessDescSetLayoutCI.pBindings = &postProcessGlobalDescSetLayoutBinding;

        if (RHI_SUCCESS != mRHI->CreateDescriptorSetLayout(&postProcessDescSetLayoutCI, mDescInfos[0].layout))
        {
            throw std::runtime_error("create post process global layout");
        }
    }

    void ToneMappingPass::setupPipelines()
    {
        mRenderPipelines.resize(1);

        RHIDescriptorSetLayout*      descriptorset_layouts[1] = {mDescInfos[0].layout};
        RHIPipelineLayoutCreateInfo pipelienLayoutCI {};
        pipelienLayoutCI.sType          = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelienLayoutCI.setLayoutCount = 1;
        pipelienLayoutCI.pSetLayouts    = descriptorset_layouts;

        if (mRHI->CreatePipelineLayout(&pipelienLayoutCI, mRenderPipelines[0].layout) != RHI_SUCCESS)
        {
            throw std::runtime_error("create post process pipeline layout");
        }

        RHIShader* vsShader = mRHI->CreateShaderModule(POSTPROCESS_VERT);
        RHIShader* psShader = mRHI->CreateShaderModule(TONEMAPPING_FRAG);

        RHIPipelineShaderStageCreateInfo vsStageCI {};
        vsStageCI.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vsStageCI.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
        vsStageCI.module = vsShader;
        vsStageCI.pName  = "main";

        RHIPipelineShaderStageCreateInfo psStageCI {};
        psStageCI.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        psStageCI.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
        psStageCI.module = psShader;
        psStageCI.pName  = "main";

        RHIPipelineShaderStageCreateInfo shader_stages[] = {vsStageCI, psStageCI};

        RHIPipelineVertexInputStateCreateInfo viStateCI {};
        viStateCI.sType = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        viStateCI.vertexBindingDescriptionCount   = 0;
        viStateCI.pVertexBindingDescriptions      = nullptr;
        viStateCI.vertexAttributeDescriptionCount = 0;
        viStateCI.pVertexAttributeDescriptions    = nullptr;

        RHIPipelineInputAssemblyStateCreateInfo iaStateCI {};
        iaStateCI.sType                  = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        iaStateCI.topology               = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        iaStateCI.primitiveRestartEnable = RHI_FALSE;

        RHIPipelineViewportStateCreateInfo vpStateCI {};
        vpStateCI.sType         = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vpStateCI.viewportCount = 1;
        vpStateCI.pViewports    = mRHI->GetSwapChainInfo().viewport;
        vpStateCI.scissorCount  = 1;
        vpStateCI.pScissors     = mRHI->GetSwapChainInfo().scissor;

        RHIPipelineRasterizationStateCreateInfo rsStateCI {};
        rsStateCI.sType                   = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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
        msStateCI.sType                = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        msStateCI.sampleShadingEnable  = RHI_FALSE;
        msStateCI.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

        RHIPipelineColorBlendAttachmentState cbAttachState {};
        cbAttachState.colorWriteMask      = RHI_COLOR_COMPONENT_R_BIT |
                                            RHI_COLOR_COMPONENT_G_BIT |
                                            RHI_COLOR_COMPONENT_B_BIT |
                                            RHI_COLOR_COMPONENT_A_BIT;
        cbAttachState.blendEnable         = RHI_FALSE;
        cbAttachState.srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
        cbAttachState.dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
        cbAttachState.colorBlendOp        = RHI_BLEND_OP_ADD;
        cbAttachState.srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
        cbAttachState.dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
        cbAttachState.alphaBlendOp        = RHI_BLEND_OP_ADD;

        RHIPipelineColorBlendStateCreateInfo cbStateCI {};
        cbStateCI.sType             = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cbStateCI.logicOpEnable     = RHI_FALSE;
        cbStateCI.logicOp           = RHI_LOGIC_OP_COPY;
        cbStateCI.attachmentCount   = 1;
        cbStateCI.pAttachments      = &cbAttachState;
        cbStateCI.blendConstants[0] = 0.0f;
        cbStateCI.blendConstants[1] = 0.0f;
        cbStateCI.blendConstants[2] = 0.0f;
        cbStateCI.blendConstants[3] = 0.0f;

        RHIPipelineDepthStencilStateCreateInfo dsStateCI {};
        dsStateCI.sType                 = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        dsStateCI.depthTestEnable       = RHI_TRUE;
        dsStateCI.depthWriteEnable      = RHI_TRUE;
        dsStateCI.depthCompareOp        = RHI_COMPARE_OP_LESS;
        dsStateCI.depthBoundsTestEnable = RHI_FALSE;
        dsStateCI.stencilTestEnable     = RHI_FALSE;

        RHIDynamicState dynamic_states[] = {RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR};

        RHIPipelineDynamicStateCreateInfo dyStateCI {};
        dyStateCI.sType             = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dyStateCI.dynamicStateCount = 2;
        dyStateCI.pDynamicStates    = dynamic_states;

        RHIGraphicsPipelineCreateInfo pipelineInfo {};
        pipelineInfo.sType               = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = 2;
        pipelineInfo.pStages             = shader_stages;
        pipelineInfo.pVertexInputState   = &viStateCI;
        pipelineInfo.pInputAssemblyState = &iaStateCI;
        pipelineInfo.pViewportState      = &vpStateCI;
        pipelineInfo.pRasterizationState = &rsStateCI;
        pipelineInfo.pMultisampleState   = &msStateCI;
        pipelineInfo.pColorBlendState    = &cbStateCI;
        pipelineInfo.pDepthStencilState  = &dsStateCI;
        pipelineInfo.layout              = mRenderPipelines[0].layout;
        pipelineInfo.renderPass          = mFrameBuffer.renderPass;
        pipelineInfo.subpass             = MAIN_CAMERA_SUBPASS_TONE_MAPPING;
        pipelineInfo.basePipelineHandle  = RHI_NULL_HANDLE;
        pipelineInfo.pDynamicState       = &dyStateCI;

        if (RHI_SUCCESS != mRHI->CreateGraphicsPipelines(RHI_NULL_HANDLE, 1, &pipelineInfo, mRenderPipelines[0].pipeline))
        {
            throw std::runtime_error("create post process graphics pipeline");
        }

        mRHI->DestroyShaderModule(vsShader);
        mRHI->DestroyShaderModule(psShader);
    }

    void ToneMappingPass::setupDescriptorSet()
    {
        RHIDescriptorSetAllocateInfo postProcessDescSetAI {};
        postProcessDescSetAI.sType          = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        postProcessDescSetAI.pNext          = nullptr;
        postProcessDescSetAI.descriptorPool = mRHI->GetDescriptorPool();
        postProcessDescSetAI.descriptorSetCount = 1;
        postProcessDescSetAI.pSetLayouts        = &mDescInfos[0].layout;

        if (RHI_SUCCESS != mRHI->AllocateDescriptorSets(&postProcessDescSetAI, mDescInfos[0].descriptorSet))
        {
            throw std::runtime_error("allocate post process global descriptor set");
        }
    }

} // namespace MiniEngine