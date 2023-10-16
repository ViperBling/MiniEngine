#include "RenderSystem.hpp"
#include "MRuntime/Core/Base/Marco.hpp"

#include "MRuntime/Resource/AssetManager/AssetManager.hpp"
#include "MRuntime/Resource/ConfigManager/ConfigManager.hpp"

#include "MRuntime/Function/Render/Interface/Vulkan/VulkanRHI.hpp"

#include "MRuntime/Function/Render/RenderCamera.hpp"
#include "MRuntime/Function/Render/RenderPass.hpp"
#include "MRuntime/Function/Render/RenderPipeline.hpp"
#include "MRuntime/Function/Render/RenderResource.hpp"
#include "MRuntime/Function/Render/RenderResourceBase.hpp"
#include "MRuntime/Function/Render/RenderScene.hpp"
#include "MRuntime/Function/Render/WindowSystem.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"

#include "MRuntime/Function/Render/DebugDraw/DebugDrawManager.hpp"

#include "MRuntime/Function/Render/Passes/MainCameraPass.hpp"

namespace MiniEngine
{
    RenderSystem::~RenderSystem()
    {
        Clear();
    }

    void RenderSystem::Initialize(RenderSystemInitInfo initInfo) 
    {
        std::shared_ptr<ConfigManager> configManager = gRuntimeGlobalContext.mConfigManager;
        ASSERT(configManager);
        std::shared_ptr<AssetManager> assetManager = gRuntimeGlobalContext.mAssetManager;
        ASSERT(assetManager);

        RHIInitInfo rhiInitInfo;
        rhiInitInfo.windowSystem = initInfo.mWindowSystem;

        mRHI = std::make_shared<VulkanRHI>();
        mRHI->Initialize(rhiInitInfo);

        // global rendering resource
        GlobalRenderingRes global_rendering_res;
        const std::string& global_rendering_res_url = configManager->GetGlobalRenderingResURL();
        assetManager->LoadAsset(global_rendering_res_url, global_rendering_res);

        SceneResourceDesc sceneResourceDesc;
        sceneResourceDesc.mIBLResourceDesc.mSkyboxIrradianceMap        = global_rendering_res.mSkyboxIrradianceMap;
        sceneResourceDesc.mIBLResourceDesc.mSkyboxSpecularMap          = global_rendering_res.mSkyboxSpecularMap;
        sceneResourceDesc.mIBLResourceDesc.mBrdfMap                    = global_rendering_res.mBrdfMap;
        sceneResourceDesc.mColorGradientResourceDesc.mColorGradientMap = global_rendering_res.mColorGradientMap;

        // setup render camera
        const CameraPose& camera_pose = global_rendering_res.mCameraConfig.mPose;
        mRenderCamera               = std::make_shared<RenderCamera>();
        mRenderCamera->LookAt(camera_pose.mPosition, camera_pose.mTarget, camera_pose.mUp);
        mRenderCamera->mZFar  = global_rendering_res.mCameraConfig.mZFar;
        mRenderCamera->mZNear = global_rendering_res.mCameraConfig.mZNear;
        mRenderCamera->SetAspect(global_rendering_res.mCameraConfig.mAspect.x / global_rendering_res.mCameraConfig.mAspect.y);

        // setup render scene
        mRenderScene                  = std::make_shared<RenderScene>();
        mRenderScene->mAmbientLight = {global_rendering_res.mAmbientLight.ToVector3()};
        mRenderScene->mDirectionalLight.mDirection =
            global_rendering_res.mDirectionalLight.mDirection.NormalizedCopy();
        mRenderScene->mDirectionalLight.mColor = global_rendering_res.mDirectionalLight.mColor.ToVector3();
        mRenderScene->SetVisibleNodesReference();

        // initialize render pipeline
        RenderPipelineInitInfo pipeline_init_info;
        pipeline_init_info.mbEnableFXAA     = global_rendering_res.mbEnableFXAA;
        pipeline_init_info.mRenderResource = mRenderResource;

        mRenderPipeline        = std::make_shared<RenderPipeline>();
        mRenderPipeline->mRHI = mRHI;
        mRenderPipeline->Initialize(pipeline_init_info);

        // descriptor set layout in main camera pass will be used when uploading resource
        std::static_pointer_cast<RenderResource>(mRenderResource)->mMeshDescLayout = &static_cast<RenderPass*>(mRenderPipeline->mMainCameraPass.get())->mDescInfos[MainCameraPass::LayoutType::PerMesh].layout;
        std::static_pointer_cast<RenderResource>(mRenderResource)->mMaterialDescLayout = &static_cast<RenderPass*>(mRenderPipeline->mMainCameraPass.get())->mDescInfos[MainCameraPass::LayoutType::MeshPerMaterial].layout;
    }

    void RenderSystem::Tick(float DeltaTime) 
    {
        // process swap data between logic and render contexts
        processSwapData();

        // prepare render command context
        mRHI->PrepareContext();

        // update per-frame buffer
        mRenderResource->UpdatePerFrameBuffer(mRenderScene, mRenderCamera);

        // update per-frame visible objects
        mRenderScene->UpdateVisibleObjects(
            std::static_pointer_cast<RenderResource>(mRenderResource),
            mRenderCamera);

        // prepare pipeline's render passes data
        mRenderPipeline->PreparePassData(mRenderResource);

        gRuntimeGlobalContext.mDebugDrawManager->Tick(DeltaTime);

        // render one frame
        if (mRenderPipelineType == RENDER_PIPELINE_TYPE::FORWARD_PIPELINE)
        {
            mRenderPipeline->ForwardRender(mRHI, mRenderResource);
        }
        // else if (mRenderPipelineType == RENDER_PIPELINE_TYPE::DEFERRED_PIPELINE)
        // {
        //     mRenderPipeline->deferredRender(mRHI, mRenderResource);
        // }
        else
        {
            LOG_ERROR(__FUNCTION__, "unsupported render pipeline type");
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

    void RenderSystem::SwapLogicRenderData()
    {
        mSwapContext.SwapLogicRenderData();
    }

    RenderSwapContext &RenderSystem::GetSwapContext()
    {
        return mSwapContext;
    }

    std::shared_ptr<RenderCamera> RenderSystem::GetRenderCamera() const
    {
        return mRenderCamera;
    }
    
    std::shared_ptr<RHI> RenderSystem::GetRHI() const
    {
        return mRHI;
    }

    void RenderSystem::SetRenderPipelineType(RENDER_PIPELINE_TYPE pipeline_type)
    {
        mRenderPipelineType = pipeline_type;
    }

    void RenderSystem::InitializeUIRenderBackend(WindowUI *window_ui)
    {
        mRenderPipeline->InitializeUIRenderBackend(window_ui);
    }

    void RenderSystem::UpdateEngineContentViewport(float offset_x, float offset_y, float width, float height)
    {
        std::static_pointer_cast<VulkanRHI>(mRHI)->mViewport.x        = offset_x;
        std::static_pointer_cast<VulkanRHI>(mRHI)->mViewport.y        = offset_y;
        std::static_pointer_cast<VulkanRHI>(mRHI)->mViewport.width    = width;
        std::static_pointer_cast<VulkanRHI>(mRHI)->mViewport.height   = height;
        std::static_pointer_cast<VulkanRHI>(mRHI)->mViewport.minDepth = 0.0f;
        std::static_pointer_cast<VulkanRHI>(mRHI)->mViewport.maxDepth = 1.0f;

        mRenderCamera->SetAspect(width / height);
    }

    uint32_t RenderSystem::GetGuidOfPickedMesh(const Vector2 &picked_uv)
    {
        return mRenderPipeline->GetGuidOfPickedMesh(picked_uv);
    }

    GObjectID RenderSystem::GetGObjectIDByMeshID(uint32_t mesh_id) const
    {
        return mRenderScene->GetGObjectIDByMeshID(mesh_id);
    }

    EngineContentViewport RenderSystem::GetEngineContentViewport() const
    {
        float x      = std::static_pointer_cast<VulkanRHI>(mRHI)->mViewport.x;
        float y      = std::static_pointer_cast<VulkanRHI>(mRHI)->mViewport.y;
        float width  = std::static_pointer_cast<VulkanRHI>(mRHI)->mViewport.width;
        float height = std::static_pointer_cast<VulkanRHI>(mRHI)->mViewport.height;
        return {x, y, width, height};
    }

    void RenderSystem::CreateAxis(std::array<RenderEntity, 3> axis_entities, std::array<RenderMeshData, 3> mesh_datas)
    {
        for (int i = 0; i < axis_entities.size(); i++)
        {
            mRenderResource->UploadGameObjectRenderResource(mRHI, axis_entities[i], mesh_datas[i]);
        }
    }

    void RenderSystem::SetVisibleAxis(std::optional<RenderEntity> axis)
    {
        mRenderScene->mRenderAxis = axis;

        if (axis.has_value())
        {
            std::static_pointer_cast<RenderPipeline>(mRenderPipeline)->SetAxisVisibleState(true);
        }
        else
        {
            std::static_pointer_cast<RenderPipeline>(mRenderPipeline)->SetAxisVisibleState(false);
        }
    }

    void RenderSystem::SetSelectedAxis(size_t selected_axis)
    {
        std::static_pointer_cast<RenderPipeline>(mRenderPipeline)->SetSelectedAxis(selected_axis);
    }

    GuidAllocator<GameObjectPartId> &RenderSystem::GetGOInstanceIDAllocator()
    {
        return mRenderScene->GetInstanceIDAllocator();
    }

    GuidAllocator<MeshSourceDesc> &RenderSystem::GetMeshAssetIDAllocator()
    {
        return mRenderScene->GetMeshAssetIDAllocator();
    }

    void RenderSystem::ClearForLevelReloading()
    {
        mRenderScene->ClearForLevelReloading();
    }

    void RenderSystem::processSwapData()
    {
        RenderSwapData& swap_data = mSwapContext.GetRenderSwapData();

        std::shared_ptr<AssetManager> asset_manager = gRuntimeGlobalContext.mAssetManager;
        ASSERT(asset_manager);

        // TODO: update global resources if needed
        if (swap_data.mSceneResourceDesc.has_value())
        {
            mRenderResource->UploadGlobalRenderResource(mRHI, *swap_data.mSceneResourceDesc);

            // reset level resource swap data to a clean state
            mSwapContext.ResetSceneResourceSwapData();
        }

        // update game object if needed
        if (swap_data.mGameObjectResourceDesc.has_value())
        {
            while (!swap_data.mGameObjectResourceDesc->IsEmpty())
            {
                GameObjectDesc gobject = swap_data.mGameObjectResourceDesc->GetNextProcessObject();

                for (size_t part_index = 0; part_index < gobject.GetObjectParts().size(); part_index++)
                {
                    const auto&      game_object_part = gobject.GetObjectParts()[part_index];
                    GameObjectPartId part_id          = {gobject.GetID(), part_index};

                    bool is_entity_in_scene = mRenderScene->GetInstanceIDAllocator().HasElement(part_id);

                    RenderEntity render_entity;
                    render_entity.mInstanceID =
                        static_cast<uint32_t>(mRenderScene->GetInstanceIDAllocator().AllocateGuid(part_id));
                    render_entity.mModelMatrix = game_object_part.mTransformDesc.mTransformMatrix;

                    mRenderScene->AddInstanceIDToMap(render_entity.mInstanceID, gobject.GetID());

                    // mesh properties
                    MeshSourceDesc mesh_source    = {game_object_part.mMeshDesc.mMeshFile};
                    bool           is_mesh_loaded = mRenderScene->GetMeshAssetIDAllocator().HasElement(mesh_source);

                    RenderMeshData mesh_data;
                    if (!is_mesh_loaded)
                    {
                        mesh_data = mRenderResource->LoadMeshData(mesh_source, render_entity.mBoundingBox);
                    }
                    else
                    {
                        render_entity.mBoundingBox = mRenderResource->GetCachedBoudingBox(mesh_source);
                    }

                    render_entity.mMeshAssetID = mRenderScene->GetMeshAssetIDAllocator().AllocateGuid(mesh_source);
                    render_entity.mbEnableVertexBlending =
                        game_object_part.mSkeletonAnimationResult.mTransforms.size() > 1; // take care
                    render_entity.mJointMatrices.resize(
                        game_object_part.mSkeletonAnimationResult.mTransforms.size());
                    for (size_t i = 0; i < game_object_part.mSkeletonAnimationResult.mTransforms.size(); ++i)
                    {
                        render_entity.mJointMatrices[i] =
                            game_object_part.mSkeletonAnimationResult.mTransforms[i].mMatrix;
                    }

                    // material properties
                    MaterialSourceDesc material_source;
                    if (game_object_part.mMaterialDesc.mbWithTexture)
                    {
                        material_source = {game_object_part.mMaterialDesc.mBaseColorTextureFile,
                                           game_object_part.mMaterialDesc.mMetallicRoughnessTextureFile,
                                           game_object_part.mMaterialDesc.mNormalTextureFile,
                                           game_object_part.mMaterialDesc.mOcclusionTextureFile,
                                           game_object_part.mMaterialDesc.mEmissiveTextureFile};
                    }
                    else
                    {
                        // TODO: move to default material definition json file
                        material_source = {
                            asset_manager->GetFullPath("Asset/Textures/default/albedo.jpg").generic_string(),
                            asset_manager->GetFullPath("Asset/Textures/default/mr.jpg").generic_string(),
                            asset_manager->GetFullPath("Asset/Textures/default/normal.jpg").generic_string(),
                            "",
                            ""};
                    }
                    bool is_material_loaded = mRenderScene->GetMaterialAssetdAllocator().HasElement(material_source);

                    RenderMaterialData material_data;
                    if (!is_material_loaded)
                    {
                        material_data = mRenderResource->LoadMaterialData(material_source);
                    }

                    render_entity.mMaterialAssetID =
                        mRenderScene->GetMaterialAssetdAllocator().AllocateGuid(material_source);

                    // create game object on the graphics api side
                    if (!is_mesh_loaded)
                    {
                        mRenderResource->UploadGameObjectRenderResource(mRHI, render_entity, mesh_data);
                    }

                    if (!is_material_loaded)
                    {
                        mRenderResource->UploadGameObjectRenderResource(mRHI, render_entity, material_data);
                    }

                    // add object to render scene if needed
                    if (!is_entity_in_scene)
                    {
                        mRenderScene->mRenderEntities.push_back(render_entity);
                    }
                    else
                    {
                        for (auto& entity : mRenderScene->mRenderEntities)
                        {
                            if (entity.mInstanceID == render_entity.mInstanceID)
                            {
                                entity = render_entity;
                                break;
                            }
                        }
                    }
                }
                // after finished processing, pop this game object
                swap_data.mGameObjectResourceDesc->Pop();
            }

            // reset game object swap data to a clean state
            mSwapContext.ResetGameObjectResourceSwapData();
        }

        // remove deleted objects
        if (swap_data.mGameObjectToDelete.has_value())
        {
            while (!swap_data.mGameObjectToDelete->IsEmpty())
            {
                GameObjectDesc gobject = swap_data.mGameObjectToDelete->GetNextProcessObject();
                mRenderScene->DeleteEntityByGObjectID(gobject.GetID());
                swap_data.mGameObjectToDelete->Pop();
            }

            mSwapContext.ResetGameObjectToDelete();
        }

        // process camera swap data
        if (swap_data.mCameraSwapData.has_value())
        {
            if (swap_data.mCameraSwapData->mFovX.has_value())
            {
                mRenderCamera->SetFovX(*swap_data.mCameraSwapData->mFovX);
            }

            if (swap_data.mCameraSwapData->mViewMatrix.has_value())
            {
                mRenderCamera->SetMainViewMatrix(*swap_data.mCameraSwapData->mViewMatrix);
            }

            if (swap_data.mCameraSwapData->mCameraType.has_value())
            {
                mRenderCamera->SetCurrentCameraType(*swap_data.mCameraSwapData->mCameraType);
            }

            mSwapContext.ResetCameraSwapData();
        }
    }
}