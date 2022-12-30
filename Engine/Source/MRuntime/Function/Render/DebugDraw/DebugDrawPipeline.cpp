#include <DebugDraw_frag.h>
#include <DebugDraw_vert.h>

#include "DebugDrawPipeline.h"
#include "Core/Base/Marco.h"
#include "Function/Global/GlobalContext.h"

namespace MiniEngine
{

    void DebugDrawPipeline::Initialize() {

        mRHI = gRuntimeGlobalContext.mRenderSystem->GetRHI();

        SetupRenderPass();
        SetupPipelines();
        SetupFrameBuffers();
    }

    void DebugDrawPipeline::SetupRenderPass() {

        // discribe the color attachment
        RHIAttachmentDescription colorAttachmentDesc {};

        colorAttachmentDesc.format  = mRHI->GetSwapChainInfo().imageFormat;    // should be equal to swapchain
        colorAttachmentDesc.samples = RHI_SAMPLE_COUNT_1_BIT;                 // now just need one sample
        // before render pass, clear the data(color/depth) in the buffer
        colorAttachmentDesc.loadOp = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        // after render pass, store the data from buffer to the memory
        colorAttachmentDesc.storeOp        = RHI_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDesc.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE; // not using the stencil
        colorAttachmentDesc.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        // not care about the image initally(before render pass)
        colorAttachmentDesc.initialLayout = RHI_IMAGE_LAYOUT_UNDEFINED;
        // present the image finally(after render pass)
        colorAttachmentDesc.finalLayout = RHI_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // set the subpass attachment reference
        RHIAttachmentReference colorAttachmentReference {};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout     = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // optimize the conversion

        // describe one of the render pass's subpass behavior
        RHISubpassDescription subpass {};
        subpass.pipelineBindPoint = RHI_PIPELINE_BIND_POINT_GRAPHICS; // explicitly claim that this is graphics subpass
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorAttachmentReference;

        // specify the subpass dependency
        RHISubpassDependency dependencies[1] = {};
        RHISubpassDependency& debugDrawDependency = dependencies[0];
        debugDrawDependency.srcSubpass            = RHI_SUBPASS_EXTERNAL; // before render pass
        debugDrawDependency.dstSubpass            = 0;
        debugDrawDependency.srcStageMask          = RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        debugDrawDependency.dstStageMask          = RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        debugDrawDependency.srcAccessMask         = 0;
        debugDrawDependency.dstAccessMask         = RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        debugDrawDependency.dependencyFlags       = 0; // NOT BY REGION

        // create the render pass
        RHIRenderPassCreateInfo renderPassCreateInfo {};
        renderPassCreateInfo.sType           = RHI_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments    = &colorAttachmentDesc;
        renderPassCreateInfo.subpassCount    = 1;
        renderPassCreateInfo.pSubpasses      = &subpass;
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies   = dependencies;

        if (mRHI->CreateRenderPass(&renderPassCreateInfo, mFrameBuffer.renderPass) != RHI_SUCCESS)
            LOG_ERROR("RHI failed to create RenderPass!");
    }

    void DebugDrawPipeline::SetupPipelines() {

        // using glsl shader
        RHIShader* VSModule = mRHI->CreateShaderModule(DEBUGDRAW_VERT);
        RHIShader* PSModule = mRHI->CreateShaderModule(DEBUGDRAW_FRAG);

        // create vertex stage pipeline info
        RHIPipelineShaderStageCreateInfo VSPipelineShaderStageCreateInfo {};
        VSPipelineShaderStageCreateInfo.sType = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VSPipelineShaderStageCreateInfo.stage = RHI_SHADER_STAGE_VERTEX_BIT;    // using in vertex stage
        VSPipelineShaderStageCreateInfo.module = VSModule;                      // set module(SPIR-V code)
        VSPipelineShaderStageCreateInfo.pName = "main";                         // SPIR-V program entry

        // create frag stage pipeline info
        RHIPipelineShaderStageCreateInfo PSPipelineShaderStageCreateInfo {};
        PSPipelineShaderStageCreateInfo.sType = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        PSPipelineShaderStageCreateInfo.stage = RHI_SHADER_STAGE_FRAGMENT_BIT;    // using in vertex stage
        PSPipelineShaderStageCreateInfo.module = PSModule;                      // set module(SPIR-V code)
        PSPipelineShaderStageCreateInfo.pName = "main";                         // SPIR-V program entry

        RHIPipelineShaderStageCreateInfo shaderStages[] = {
            VSPipelineShaderStageCreateInfo,
            PSPipelineShaderStageCreateInfo
        };

        // set vertex input information
        RHIPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};
        vertexInputStateCreateInfo.sType = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
        vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

        // set vertex input assembly rule
        RHIPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // 在画第一个三角形时，直接选这个选项
        inputAssembly.primitiveRestartEnable = RHI_FALSE; // 不进行图元重启（就画个三角形没必要）

        // set viewport & scissor change stage information
        RHIPipelineViewportStateCreateInfo viewportStateCreateInfo {};
        viewportStateCreateInfo.sType         = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports    = mRHI->GetSwapChainInfo().viewport;
        viewportStateCreateInfo.scissorCount  = 1;
        viewportStateCreateInfo.pScissors     = mRHI->GetSwapChainInfo().scissor;

        // set rasterization stage
        RHIPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {};
        rasterizationStateCreateInfo.sType            = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.depthClampEnable = RHI_FALSE;        // discard the far & near fragment
        rasterizationStateCreateInfo.rasterizerDiscardEnable = RHI_FALSE; // not discard the rasterization stage
        rasterizationStateCreateInfo.polygonMode             = RHI_POLYGON_MODE_FILL; // fill the vertices to polygon
        rasterizationStateCreateInfo.lineWidth = 1.0f; // when the line width is greater than 1, GPU will be used
        rasterizationStateCreateInfo.cullMode  = RHI_CULL_MODE_NONE;       // cull nothing
        rasterizationStateCreateInfo.frontFace = RHI_FRONT_FACE_CLOCKWISE; // specify the vertices join order
        rasterizationStateCreateInfo.depthBiasEnable         = RHI_FALSE;
        rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationStateCreateInfo.depthBiasClamp          = 0.0f;
        rasterizationStateCreateInfo.depthBiasSlopeFactor    = 0.0f;

        // set MSAA information
        RHIPipelineMultisampleStateCreateInfo msStateCreateInfo {};
        msStateCreateInfo.sType                = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        msStateCreateInfo.sampleShadingEnable  = RHI_FALSE;
        msStateCreateInfo.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

         // set depth & stencil test information
         // RHIPipelineDepthStencilStateCreateInfo depthStencilCreateInfo {};
         // depthStencilCreateInfo.sType                 = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
         // depthStencilCreateInfo.depthTestEnable       = RHI_TRUE;
         // depthStencilCreateInfo.depthWriteEnable      = RHI_TRUE;
         // depthStencilCreateInfo.depthCompareOp        = RHI_COMPARE_OP_LESS;
         // depthStencilCreateInfo.depthBoundsTestEnable = RHI_FALSE;
         // depthStencilCreateInfo.stencilTestEnable     = RHI_FALSE;

        // set color blend rule in every buffer frame
        RHIPipelineColorBlendAttachmentState colorBlendAttachmentState {}; // used in every buffer frame
        colorBlendAttachmentState.colorWriteMask =
            RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT |
            RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.blendEnable         = RHI_FALSE; // will be true later
        colorBlendAttachmentState.srcColorBlendFactor = RHI_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachmentState.dstColorBlendFactor = RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachmentState.colorBlendOp        = RHI_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.alphaBlendOp        = RHI_BLEND_OP_ADD;

        // set global color blend methods
        RHIPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
        colorBlendStateCreateInfo.sType             = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable     = RHI_FALSE; // will be true maybe later
        colorBlendStateCreateInfo.logicOp           = RHI_LOGIC_OP_COPY;
        colorBlendStateCreateInfo.attachmentCount   = 1;
        colorBlendStateCreateInfo.pAttachments      = &colorBlendAttachmentState;
        colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
        colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
        colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
        colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

        // some settings can be dynamic(it will not rebulild the pipeline however)
        RHIDynamicState dynamicStates[] = {RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR};
        RHIPipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
        dynamicStateCreateInfo.sType             = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = 2;
        dynamicStateCreateInfo.pDynamicStates    = dynamicStates;

        // some glsl uniform will be specified in the Pipeline layout
        RHIPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.sType                  = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount         = 0; // TODO: layout descriptor
        pipelineLayoutCreateInfo.pSetLayouts            = nullptr;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;

        mRenderPipelines.resize(1);
        if (mRHI->CreatePiplineLayout(&pipelineLayoutCreateInfo, mRenderPipelines[0].layout) != RHI_SUCCESS) {
            LOG_ERROR("Failed to create RHI pipeline layout");
        }

        RHIGraphicsPipelineCreateInfo pipelineCreateInfo {};
        pipelineCreateInfo.sType = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        // fill the shader stages
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages    = shaderStages;
        // fill the fixed function parts
        pipelineCreateInfo.pVertexInputState   = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState   = &msStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState  = nullptr;
        pipelineCreateInfo.pColorBlendState    = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDynamicState       = &dynamicStateCreateInfo;
        // fill the pipeline layout
        pipelineCreateInfo.layout = mRenderPipelines[0].layout;
        // fill the render pass
        pipelineCreateInfo.renderPass = mFrameBuffer.renderPass;
        pipelineCreateInfo.subpass    = 0; // subpass index
        // set the derivative pipeline reference
        pipelineCreateInfo.basePipelineHandle = RHI_NULL_HANDLE; // not used
        pipelineCreateInfo.basePipelineIndex  = -1;              // illegal index

        // select pipeline type
        if (mPipelineType == DebugDrawPipelineType::triangle)
            inputAssembly.topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // create the pipeline
        if (mRHI->CreateGraphicsPipeline(RHI_NULL_HANDLE, 1, &pipelineCreateInfo, mRenderPipelines[0].pipeline) !=
            RHI_SUCCESS)
            LOG_ERROR("Failed to create debug draw graphics pipeline");
    }

    void DebugDrawPipeline::SetupFrameBuffers() {

        // every image view has its own framebuffer
        const std::vector<RHIImageView*>&& imageViews = mRHI->GetSwapChainInfo().imageViews;
        // store framebuffer
        mFrameBuffer.framebuffers.resize(imageViews.size());

        for (size_t i = 0; i < mFrameBuffer.framebuffers.size(); i++) {
            RHIImageView* attachments[] = {imageViews[i]};

            RHIFramebufferCreateInfo framebufferCreateInfo {};
            framebufferCreateInfo.sType = RHI_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            // specify which render pass to should be compatible with the frame buffer
            framebufferCreateInfo.renderPass      = mFrameBuffer.renderPass;
            framebufferCreateInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
            framebufferCreateInfo.pAttachments    = attachments;
            framebufferCreateInfo.width           = mRHI->GetSwapChainInfo().extent.width;
            framebufferCreateInfo.height          = mRHI->GetSwapChainInfo().extent.height;
            framebufferCreateInfo.layers          = 1;

            if (mRHI->CreateFrameBuffer(&framebufferCreateInfo, mFrameBuffer.framebuffers[i]) != RHI_SUCCESS)
                LOG_ERROR("create inefficient pick framebuffer");
        }
    }
}
