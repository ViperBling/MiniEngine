#include <DebugDraw_frag.h>
#include <DebugDraw_vert.h>

#include "DebugDrawPipeline.h"
#include "Core/Base/Marco.h"
#include "Function/Global/GlobalContext.h"

namespace MiniEngine
{

    void DebugDrawPipeline::Initialize() {

        mRHI = gRuntimeGlobalContext.mRenderSystem->GetRHI();
        SetupPipelines();
    }

    void DebugDrawPipeline::SetupRenderPass() {

        RHIAttachmentDescription colorAttachmentDesc {};
        colorAttachmentDesc.format = mRHI->GetSwapChainInfo().imageFormat;
    }

    void DebugDrawPipeline::SetupPipelines() {

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
        PSPipelineShaderStageCreateInfo.module = VSModule;                      // set module(SPIR-V code)
        PSPipelineShaderStageCreateInfo.pName = "main";                         // SPIR-V program entry

        RHIPipelineShaderStageCreateInfo shaderStages[] = {
            VSPipelineShaderStageCreateInfo,
            PSPipelineShaderStageCreateInfo
        };

        // set vertex input information
        VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        // set vertex input assembly rule
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // 在花第一个三角形时，直接选这个选项
        inputAssembly.primitiveRestartEnable = VK_FALSE; // 不进行图元重启（就画个三角形没必要）

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
    }
}
