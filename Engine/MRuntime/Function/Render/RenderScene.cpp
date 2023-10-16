#include "RenderScene.hpp"
#include "MRuntime/Function/Render/RenderPass.hpp"
#include "MRuntime/Function/Render/RenderHelper.hpp"
#include "MRuntime/Function/Render/RenderResource.hpp"

namespace MiniEngine
{
    void RenderScene::Clear()
    {
    }

    void RenderScene::UpdateVisibleObjects(std::shared_ptr<RenderResource> render_resource, std::shared_ptr<RenderCamera> camera)
    {
        updateVisibleObjectsDirectionalLight(render_resource, camera);
        updateVisibleObjectsMainCamera(render_resource, camera);
        updateVisibleObjectsAxis(render_resource);
    }

    void RenderScene::SetVisibleNodesReference()
    {
        RenderPass::mVisibleNodes.mDirectionalLightVisibleMeshNodes = &mDirectionalLightVisibleMeshNodes;
        RenderPass::mVisibleNodes.mPointLightVisibleMeshNodes       = &mPointLightsVisibleMeshNodes;
        RenderPass::mVisibleNodes.mMainCameraVisibleMeshNodes       = &mMainCameraVisibleMeshNodes;
        RenderPass::mVisibleNodes.mAxisNode                         = &mAxisNode;
    }

    GuidAllocator<GameObjectPartId> &RenderScene::GetInstanceIDAllocator()
    {
        return mInstanceIDAllocator;
    }

    GuidAllocator<MeshSourceDesc> &RenderScene::GetMeshAssetIDAllocator()
    {
        return mMeshAssetIDAllocator;
    }

    GuidAllocator<MaterialSourceDesc> &RenderScene::GetMaterialAssetdAllocator()
    {
        return mMaterialIDAllocator;
    }

    void RenderScene::AddInstanceIDToMap(uint32_t instance_id, GObjectID go_id)
    {
        mMeshObjectIDMap[instance_id] = go_id;
    }

    GObjectID RenderScene::GetGObjectIDByMeshID(uint32_t mesh_id) const
    {
        auto find_it = mMeshObjectIDMap.find(mesh_id);
        if (find_it != mMeshObjectIDMap.end())
        {
            return find_it->second;
        }
        return GObjectID();
    }

    void RenderScene::DeleteEntityByGObjectID(GObjectID go_id)
    {
        for (auto it = mMeshObjectIDMap.begin(); it != mMeshObjectIDMap.end(); it++)
        {
            if (it->second == go_id)
            {
                mMeshObjectIDMap.erase(it);
                break;
            }
        }

        GameObjectPartId part_id = {go_id, 0};
        size_t           find_guid;
        if (mInstanceIDAllocator.GetElementGuid(part_id, find_guid))
        {
            for (auto it = mRenderEntities.begin(); it != mRenderEntities.end(); it++)
            {
                if (it->mInstanceID == find_guid)
                {
                    mRenderEntities.erase(it);
                    break;
                }
            }
        }
    }

    void RenderScene::ClearForLevelReloading()
    {
        mInstanceIDAllocator.Clear();
        mMeshObjectIDMap.clear();
        mRenderEntities.clear();
    }

    void RenderScene::updateVisibleObjectsDirectionalLight(std::shared_ptr<RenderResource> render_resource, std::shared_ptr<RenderCamera> camera)
    {
        Matrix4x4 directional_light_proj_view = CalculateDirectionalLightCamera(*this, *camera);

        render_resource->mMeshPerFrameStorageBufferObject.directional_light_proj_view =
            directional_light_proj_view;
        render_resource->mMeshDirectionalLightShadowPerFrameStorageBufferObject.light_proj_view =
            directional_light_proj_view;

        mDirectionalLightVisibleMeshNodes.clear();

        ClusterFrustum frustum =
            CreateClusterFrustumFromMatrix(directional_light_proj_view, -1.0, 1.0, -1.0, 1.0, 0.0, 1.0);

        for (const RenderEntity& entity : mRenderEntities)
        {
            BoundingBox mesh_asset_bounding_box {entity.mBoundingBox.GetMinCorner(),
                                                 entity.mBoundingBox.GetMaxCorner()};

            if (TiledFrustumIntersectBox(frustum, BoundingBoxTransform(mesh_asset_bounding_box, entity.mModelMatrix)))
            {
                mDirectionalLightVisibleMeshNodes.emplace_back();
                RenderMeshNode& temp_node = mDirectionalLightVisibleMeshNodes.back();

                temp_node.model_matrix = &entity.mModelMatrix;

                assert(entity.mJointMatrices.size() <= s_mesh_vertex_blending_max_joint_count);
                if (!entity.mJointMatrices.empty())
                {
                    temp_node.joint_count    = static_cast<uint32_t>(entity.mJointMatrices.size());
                    temp_node.joint_matrices = entity.mJointMatrices.data();
                }
                temp_node.node_id = entity.mInstanceID;

                VulkanMesh& mesh_asset           = render_resource->GetEntityMesh(entity);
                temp_node.ref_mesh               = &mesh_asset;
                temp_node.enable_vertex_blending = entity.mbEnableVertexBlending;

                VulkanPBRMaterial& material_asset = render_resource->GetEntityMaterial(entity);
                temp_node.ref_material            = &material_asset;
            }
        }
    }

    void RenderScene::updateVisibleObjectsMainCamera(std::shared_ptr<RenderResource> render_resource, std::shared_ptr<RenderCamera> camera)
    {
        mMainCameraVisibleMeshNodes.clear();

        Matrix4x4 view_matrix      = camera->GetViewMatrix();
        Matrix4x4 proj_matrix      = camera->GetPersProjMatrix();
        Matrix4x4 proj_view_matrix = proj_matrix * view_matrix;

        ClusterFrustum f = CreateClusterFrustumFromMatrix(proj_view_matrix, -1.0, 1.0, -1.0, 1.0, 0.0, 1.0);

        for (const RenderEntity& entity : mRenderEntities)
        {
            BoundingBox mesh_asset_bounding_box {entity.mBoundingBox.GetMinCorner(),
                                                 entity.mBoundingBox.GetMaxCorner()};

            if (TiledFrustumIntersectBox(f, BoundingBoxTransform(mesh_asset_bounding_box, entity.mModelMatrix)))
            {
                mMainCameraVisibleMeshNodes.emplace_back();
                RenderMeshNode& temp_node = mMainCameraVisibleMeshNodes.back();
                temp_node.model_matrix    = &entity.mModelMatrix;

                assert(entity.mJointMatrices.size() <= s_mesh_vertex_blending_max_joint_count);
                if (!entity.mJointMatrices.empty())
                {
                    temp_node.joint_count    = static_cast<uint32_t>(entity.mJointMatrices.size());
                    temp_node.joint_matrices = entity.mJointMatrices.data();
                }
                temp_node.node_id = entity.mInstanceID;

                VulkanMesh& mesh_asset           = render_resource->GetEntityMesh(entity);
                temp_node.ref_mesh               = &mesh_asset;
                temp_node.enable_vertex_blending = entity.mbEnableVertexBlending;

                VulkanPBRMaterial& material_asset = render_resource->GetEntityMaterial(entity);
                temp_node.ref_material            = &material_asset;
            }
        }
    }

    void RenderScene::updateVisibleObjectsAxis(std::shared_ptr<RenderResource> render_resource)
    {
        if (mRenderAxis.has_value())
        {
            RenderEntity& axis = *mRenderAxis;

            mAxisNode.model_matrix = axis.mModelMatrix;
            mAxisNode.node_id      = axis.mInstanceID;

            VulkanMesh& mesh_asset             = render_resource->GetEntityMesh(axis);
            mAxisNode.ref_mesh               = &mesh_asset;
            mAxisNode.enable_vertex_blending = axis.mbEnableVertexBlending;
        }
    }
}