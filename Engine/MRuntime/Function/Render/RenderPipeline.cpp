#include "RenderPipeline.hpp"
#include "MRuntime/Function/Render/Interface/Vulkan/VulkanRHI.hpp"
#include "MRuntime/Core/Base/Marco.hpp"
#include "MRuntime/Function/Render/DebugDraw/DebugDrawManager.hpp"

#include "MRuntime/Function/Render/Passes/ColorGradientPass.hpp"
#include "MRuntime/Function/Render/Passes/CombineUIPass.hpp"
#include "MRuntime/Function/Render/Passes/DirectionalLightPass.hpp"
#include "MRuntime/Function/Render/Passes/MainCameraPass.hpp"
#include "MRuntime/Function/Render/Passes/PickPass.hpp"
#include "MRuntime/Function/Render/Passes/ToneMappingPass.hpp"
#include "MRuntime/Function/Render/Passes/UIPass.hpp"

namespace MiniEngine
{
    void RenderPipeline::Initialize(RenderPipelineInitInfo init_info)
    {
        mDirectionalLightPass  = std::make_shared<DirectionalLightShadowPass>();
        mMainCameraPass        = std::make_shared<MainCameraPass>();
        mToneMappingPass       = std::make_shared<ToneMappingPass>();
        mColorGradientPass      = std::make_shared<ColorGradientPass>();
        mUIPass                 = std::make_shared<UIPass>();
        mCombineUIPass         = std::make_shared<CombineUIPass>();
        mPickPass               = std::make_shared<PickPass>();

        RenderPassCommonInfo pass_common_info;
        pass_common_info.mRHI             = mRHI;
        pass_common_info.mRenderResource = init_info.mRenderResource;

        mDirectionalLightPass->SetCommonInfo(pass_common_info);
        mMainCameraPass->SetCommonInfo(pass_common_info);
        mToneMappingPass->SetCommonInfo(pass_common_info);
        mColorGradientPass->SetCommonInfo(pass_common_info);
        mUIPass->SetCommonInfo(pass_common_info);
        mCombineUIPass->SetCommonInfo(pass_common_info);
        mPickPass->SetCommonInfo(pass_common_info);

        mDirectionalLightPass->Initialize(nullptr);

        std::shared_ptr<MainCameraPass> main_camera_pass = std::static_pointer_cast<MainCameraPass>(mMainCameraPass);
        std::shared_ptr<RenderPass>     _main_camera_pass = std::static_pointer_cast<RenderPass>(mMainCameraPass);

        main_camera_pass->mDirectionalLightShadowColorImageView =
            std::static_pointer_cast<RenderPass>(mDirectionalLightPass)->mFrameBuffer.attachments[0].view;

        MainCameraPassInitInfo main_camera_init_info;
        main_camera_init_info.mbEnableFXAA = init_info.mbEnableFXAA;
        mMainCameraPass->Initialize(&main_camera_init_info);

        std::vector<RHIDescriptorSetLayout*> descriptor_layouts = _main_camera_pass->GetDescriptorSetLayouts();
        std::static_pointer_cast<DirectionalLightShadowPass>(mDirectionalLightPass)
            ->SetPerMeshLayout(descriptor_layouts[MainCameraPass::LayoutType::LayoutType_PerMesh]);

        mDirectionalLightPass->PostInitialize();

        ToneMappingPassInitInfo tone_mapping_init_info;
        tone_mapping_init_info.mRenderPass = _main_camera_pass->GetRenderPass();
        tone_mapping_init_info.mInputAttachment =
            _main_camera_pass->GetFramebufferImageViews()[MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD];
        mToneMappingPass->Initialize(&tone_mapping_init_info);

        ColorGradientPassInitInfo color_grading_init_info;
        color_grading_init_info.mRenderPass = _main_camera_pass->GetRenderPass();
        color_grading_init_info.mInputAttachment =
            _main_camera_pass->GetFramebufferImageViews()[MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN];
        mColorGradientPass->Initialize(&color_grading_init_info);

        UIPassInitInfo ui_init_info;
        ui_init_info.mRenderPass = _main_camera_pass->GetRenderPass();
        mUIPass->Initialize(&ui_init_info);

        CombineUIPassInitInfo combine_ui_init_info;
        combine_ui_init_info.mRenderPass = _main_camera_pass->GetRenderPass();
        combine_ui_init_info.mSceneInputAttachment =
            _main_camera_pass->GetFramebufferImageViews()[MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD];
        combine_ui_init_info.mUIInputAttachment =
            _main_camera_pass->GetFramebufferImageViews()[MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN];
        mCombineUIPass->Initialize(&combine_ui_init_info);

        PickPassInitInfo pick_init_info;
        pick_init_info.mPerMeshLayout = descriptor_layouts[MainCameraPass::LayoutType::LayoutType_PerMesh];
        mPickPass->Initialize(&pick_init_info);
    }

    void RenderPipeline::ForwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> renderResource)
    {
        VulkanRHI*      vulkan_rhi      = static_cast<VulkanRHI*>(rhi.get());
        RenderResource* vulkan_resource = static_cast<RenderResource*>(renderResource.get());

        vulkan_resource->ResetRingBufferOffset(vulkan_rhi->mCurrentFrameIndex);

        vulkan_rhi->WaitForFences();

        vulkan_rhi->ResetCommandPool();

        bool recreate_swapchain =
            vulkan_rhi->PrepareBeforePass(std::bind(&RenderPipeline::PassUpdateAfterRecreateSwapChain, this));
        if (recreate_swapchain)
        {
            return;
        }

        static_cast<DirectionalLightShadowPass*>(mDirectionalLightPass.get())->Draw();

        ColorGradientPass& color_grading_pass = *(static_cast<ColorGradientPass*>(mColorGradientPass.get()));
        ToneMappingPass&  tone_mapping_pass  = *(static_cast<ToneMappingPass*>(mToneMappingPass.get()));
        UIPass&           ui_pass            = *(static_cast<UIPass*>(mUIPass.get()));
        CombineUIPass&    combine_ui_pass    = *(static_cast<CombineUIPass*>(mCombineUIPass.get()));

        static_cast<MainCameraPass*>(mMainCameraPass.get())
            ->DrawForward(color_grading_pass,
                          tone_mapping_pass,
                          ui_pass,
                          combine_ui_pass,
                          vulkan_rhi->mCurrentSwapChainImageIndex);

        
        gRuntimeGlobalContext.mDebugDrawManager->Draw(vulkan_rhi->mCurrentSwapChainImageIndex);

        vulkan_rhi->SubmitRendering(std::bind(&RenderPipeline::PassUpdateAfterRecreateSwapChain, this));
    }

    void RenderPipeline::PassUpdateAfterRecreateSwapChain()
    {
        MainCameraPass&   main_camera_pass   = *(static_cast<MainCameraPass*>(mMainCameraPass.get()));
        ColorGradientPass& color_grading_pass = *(static_cast<ColorGradientPass*>(mColorGradientPass.get()));
        ToneMappingPass&  tone_mapping_pass  = *(static_cast<ToneMappingPass*>(mToneMappingPass.get()));
        CombineUIPass&    combine_ui_pass    = *(static_cast<CombineUIPass*>(mCombineUIPass.get()));
        PickPass&         pick_pass          = *(static_cast<PickPass*>(mPickPass.get()));

        main_camera_pass.UpdateAfterFramebufferRecreate();
        tone_mapping_pass.UpdateAfterFramebufferRecreate(
            main_camera_pass.GetFramebufferImageViews()[MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD]);
        color_grading_pass.UpdateAfterFramebufferRecreate(
            main_camera_pass.GetFramebufferImageViews()[MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN]);
        combine_ui_pass.UpdateAfterFramebufferRecreate(
            main_camera_pass.GetFramebufferImageViews()[MAIN_CAMERA_PASS_BACKUP_BUFFER_ODD],
            main_camera_pass.GetFramebufferImageViews()[MAIN_CAMERA_PASS_BACKUP_BUFFER_EVEN]);
        pick_pass.RecreateFramebuffer();
        gRuntimeGlobalContext.mDebugDrawManager->UpdateAfterRecreateSwapChain();
    }

    uint32_t RenderPipeline::GetGuidOfPickedMesh(const Vector2 &picked_uv)
    {
        PickPass& pick_pass = *(static_cast<PickPass*>(mPickPass.get()));
        return pick_pass.Pick(picked_uv);
    }

    void RenderPipeline::SetAxisVisibleState(bool state)
    {
        MainCameraPass& main_camera_pass = *(static_cast<MainCameraPass*>(mMainCameraPass.get()));
        main_camera_pass.mbIsShowAxis  = state;
    }

    void RenderPipeline::SetSelectedAxis(size_t selected_axis)
    {
        MainCameraPass& main_camera_pass = *(static_cast<MainCameraPass*>(mMainCameraPass.get()));
        main_camera_pass.mSelectedAxis = selected_axis;
    }
}