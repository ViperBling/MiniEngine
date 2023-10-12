#pragma once

#include "MRuntime/Function/Render/RenderCommon.hpp"
#include "MRuntime/Function/Render/RenderPassBase.hpp"
#include "MRuntime/Function/Render/RenderResource.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace MiniEngine
{
    class VulkanRHI;

    enum
    {
        MAIN_CAMERA_PASS_GBUFFER_A                     = 0,
        MAIN_CAMERA_PASS_GBUFFER_B                     = 1,
        MAIN_CAMERA_PASS_GBUFFER_C                     = 2,
        MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD             = 3,
        MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN            = 4,
        MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_ODD       = 5,
        MAIN_CAMERA_PASS_POST_PROCESS_BUFFER_EVEN      = 6,
        MAIN_CAMERA_PASS_DEPTH                         = 7,
        MAIN_CAMERA_PASS_SWAP_CHAIN_IMAGE              = 8,
        MAIN_CAMERA_PASS_CUSTOM_ATTACHMENT_COUNT       = 5,
        MAIN_CAMERA_PASS_POST_PROCESS_ATTACHMENT_COUNT = 2,
        MAIN_CAMERA_PASS_ATTACHMENT_COUNT              = 9,
    };

    enum
    {
        MAIN_CAMERA_SUBPASS_basepass = 0,
        MAIN_CAMERA_SUBPASS_DEFERRED_LIGHTING,
        MAIN_CAMERA_SUBPASS_FORWARD_LIGHTING,
        MAIN_CAMERA_SUBPASS_TONE_MAPPING,
        MAIN_CAMERA_SUBPASS_COLOR_GRADIENT,
        MAIN_CAMERA_SUBPASS_FXAA,
        MAIN_CAMERA_SUBPASS_UI,
        MAIN_CAMERA_SUBPASS_COMBINE_UI,
        MAIN_CAMERA_SUBPASS_COUNT
    };

    struct VisiableNodes
    {
        std::vector<RenderMeshNode>*              mDirectionalLightVisibleMeshNodes {nullptr};
        std::vector<RenderMeshNode>*              mPointLightVisibleMeshNodes {nullptr};
        std::vector<RenderMeshNode>*              mMainCameraVisibleMeshNodes {nullptr};
        RenderAxisNode*                           mAxisNode {nullptr};
    };

    class RenderPass : public RenderPassBase
    {
    public:
        struct FrameBufferAttachment
        {
            RHIImage*        image;
            RHIDeviceMemory* mem;
            RHIImageView*    view;
            RHIFormat        format;
        };

        struct FrameBuffer
        {
            int           width;
            int           height;
            RHIFrameBuffer* framebuffer;
            RHIRenderPass*  render_pass;

            std::vector<FrameBufferAttachment> attachments;
        };

        struct Descriptor
        {
            RHIDescriptorSetLayout* layout;
            RHIDescriptorSet*       descriptor_set;
        };

        struct RenderPipelineBase
        {
            RHIPipelineLayout* layout;
            RHIPipeline*       pipeline;
        };

        GlobalRenderResource*      mGlobalRenderResource {nullptr};

        std::vector<Descriptor>         mDescInfos;
        std::vector<RenderPipelineBase> mRenderPipelines;
        FrameBuffer                     mFrameBuffer;

        void Initialize(const RenderPassInitInfo* init_info) override;
        void PostInitialize() override;

        virtual void Draw();

        virtual RHIRenderPass*                       GetRenderPass() const;
        virtual std::vector<RHIImageView*>           GetFramebufferImageViews() const;
        virtual std::vector<RHIDescriptorSetLayout*> GetDescriptorSetLayouts() const;

        static VisiableNodes mVisibleNodes;
    };
}