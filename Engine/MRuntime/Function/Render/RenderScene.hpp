#pragma once

#include "MRuntime/Function/Framework/Object/ObjectIDAllocator.hpp"
#include "MRuntime/Function/Render/RenderCommon.hpp"
#include "MRuntime/Function/Render/RenderEntity.hpp"
#include "MRuntime/Function/Render/RenderGuidAllocator.hpp"
#include "MRuntime/Function/Render/RenderObject.hpp"
#include "MRuntime/Function/Render/Light.hpp"

#include <optional>
#include <vector>

namespace MiniEngine
{
    class RenderResource;
    class RenderCamera;

    class RenderScene
    {
    public:

        // clear
        void Clear();

        // update visible objects in each frame
        void UpdateVisibleObjects(std::shared_ptr<RenderResource> render_resource,
                                  std::shared_ptr<RenderCamera>   camera);

        // set visible nodes ptr in render pass
        void SetVisibleNodesReference();

        GuidAllocator<GameObjectPartId>&   GetInstanceIdAllocator();
        GuidAllocator<MeshSourceDesc>&     GetMeshAssetIdAllocator();
        GuidAllocator<MaterialSourceDesc>& GetMaterialAssetdAllocator();

        void      AddInstanceIdToMap(uint32_t instance_id, GObjectID go_id);
        GObjectID GetGObjectIDByMeshID(uint32_t mesh_id) const;
        void      DeleteEntityByGObjectID(GObjectID go_id);

        void ClearForLevelReloading();

    private:
        void updateVisibleObjectsDirectionalLight(
            std::shared_ptr<RenderResource> render_resource,
            std::shared_ptr<RenderCamera>   camera
            );
        void updateVisibleObjectsPointLight(
            std::shared_ptr<RenderResource> render_resource
        );
        void updateVisibleObjectsMainCamera(
            std::shared_ptr<RenderResource> render_resource,
            std::shared_ptr<RenderCamera>   camera
        );
        void updateVisibleObjectsAxis(std::shared_ptr<RenderResource> render_resource);
        void updateVisibleObjectsParticle(std::shared_ptr<RenderResource> render_resource);

    public:
        // light
        AmbientLight      mAmbientLight;
        PDirectionalLight mDirectionalLight;
        PointLightList    mPointLightList;
        // render entities
        std::vector<RenderEntity> mRenderEntities;

        // axis, for editor
        std::optional<RenderEntity> mRenderAxis;

        // visible objects (updated per frame)
        std::vector<RenderMeshNode> mDirectionalLightVisibleMeshNodes;
        std::vector<RenderMeshNode> mPointLightsVisibleMeshNodes;
        std::vector<RenderMeshNode> mMainCameraVisibleMeshNodes;
        RenderAxisNode              mAxisNode;

    private:
        GuidAllocator<GameObjectPartId>   mInstanceIDAllocator;
        GuidAllocator<MeshSourceDesc>     mMeshAssetIDAllocator;
        GuidAllocator<MaterialSourceDesc> mMaterialIDAllocator;

        std::unordered_map<uint32_t, GObjectID> mMeshObjectIDMap;
    };
} // namespace MiniEngine
