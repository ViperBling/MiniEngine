#include "MeshComponent.hpp"

#include "MRuntime/Resource/AssetManager/AssetManager.hpp"
#include "MRuntime/Resource/ResourceType/Data/Material.hpp"

#include "MRuntime/Function/Framework/Component/TransformComponent/TransformComponent.hpp"
#include "MRuntime/Function/Framework/Object/Object.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Render/RenderSwapContext.hpp"
#include "MRuntime/Function/Render/RenderSystem.hpp"

namespace MiniEngine
{
    void MeshComponent::PostLoadResource(std::weak_ptr<GObject> parent_object)
    {
        mParentObject = parent_object;

        std::shared_ptr<AssetManager> asset_manager = gRuntimeGlobalContext.mAssetManager;
        ASSERT(asset_manager);

        mRawMeshes.resize(mMeshRes.mSubMeshes.size());

        size_t raw_mesh_count = 0;
        for (const SubMeshRes& sub_mesh : mMeshRes.mSubMeshes)
        {
            GameObjectPartDesc& meshComponent = mRawMeshes[raw_mesh_count];
            meshComponent.mMeshDesc.mMeshFile =
                asset_manager->GetFullPath(sub_mesh.mObjectFileRef).generic_string();

            meshComponent.mMaterialDesc.mbWithTexture = sub_mesh.mMaterial.empty() == false;

            if (meshComponent.mMaterialDesc.mbWithTexture)
            {
                MaterialRes material_res;
                asset_manager->LoadAsset(sub_mesh.mMaterial, material_res);

                meshComponent.mMaterialDesc.mBaseColorTextureFile =
                    asset_manager->GetFullPath(material_res.mBaseColorTextureFile).generic_string();
                meshComponent.mMaterialDesc.mMetallicRoughnessTextureFile =
                    asset_manager->GetFullPath(material_res.mMetallicRoughnessTextureFile).generic_string();
                meshComponent.mMaterialDesc.mNormalTextureFile =
                    asset_manager->GetFullPath(material_res.mNormalTextureFile).generic_string();
                meshComponent.mMaterialDesc.mOcclusionTextureFile =
                    asset_manager->GetFullPath(material_res.mOcclusionTextureFile).generic_string();
                meshComponent.mMaterialDesc.mEmissiveTextureFile =
                    asset_manager->GetFullPath(material_res.mEmissiveTextureFile).generic_string();
            }

            auto object_space_transform = sub_mesh.mTransform.GetMatrix();

            meshComponent.mTransformDesc.mTransformMatrix = object_space_transform;

            ++raw_mesh_count;
        }
    }

    void MeshComponent::Tick(float delta_time)
    {
        if (!mParentObject.lock())
            return;

        TransformComponent* transformComp = mParentObject.lock()->TryGetComponent(TransformComponent);

        if (transformComp->IsDirty())
        {
            std::vector<GameObjectPartDesc> dirtyMeshParts;
            
            for (GameObjectPartDesc& mesh_part : mRawMeshes)
            {
                Matrix4x4 object_transform_matrix = mesh_part.mTransformDesc.mTransformMatrix;

                mesh_part.mTransformDesc.mTransformMatrix =
                    transformComp->GetMatrix() * object_transform_matrix;
                dirtyMeshParts.push_back(mesh_part);

                mesh_part.mTransformDesc.mTransformMatrix = object_transform_matrix;
            }

            RenderSwapContext& renderSwapContext = gRuntimeGlobalContext.mRenderSystem->GetSwapContext();
            RenderSwapData&    logicSwapData     = renderSwapContext.GetLogicSwapData();

            logicSwapData.AddDirtyGameObject(GameObjectDesc {mParentObject.lock()->GetID(), dirtyMeshParts});

            transformComp->SetDirtyFlag(false);
        }
    }
} // namespace MiniEngine
