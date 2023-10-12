#pragma once

#include "MRuntime/Function/Render/RenderScene.hpp"
#include "MRuntime/Function/Render/RenderSwapContext.hpp"
#include "MRuntime/Function/Render/RenderType.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace MiniEngine
{
    class RHI;
    class RenderScene;
    class RenderCamera;

    class RenderResourceBase 
    {
    public:
        virtual ~RenderResourceBase() {}

        virtual void Clear() = 0;

        virtual void UploadGlobalRenderResource(std::shared_ptr<RHI> rhi, SceneResourceDesc level_resource_desc) = 0;
        virtual void UploadGameObjectRenderResource(
            std::shared_ptr<RHI> rhi,
            RenderEntity         render_entity,
            RenderMeshData       mesh_data,
            RenderMaterialData   material_data
        ) = 0;

        virtual void UploadGameObjectRenderResource(
            std::shared_ptr<RHI> rhi,
            RenderEntity         render_entity,
            RenderMeshData       mesh_data
        ) = 0;

        virtual void UploadGameObjectRenderResource(
            std::shared_ptr<RHI> rhi,
            RenderEntity         render_entity,
            RenderMaterialData   material_data
        ) = 0;

        virtual void UpdatePerFrameBuffer(
            std::shared_ptr<RenderScene>  render_scene,
            std::shared_ptr<RenderCamera> camera
        ) = 0;

        // TODO: data caching
        std::shared_ptr<TextureData> LoadTextureHDR(std::string file, int desired_channels = 4);
        std::shared_ptr<TextureData> LoadTexture(std::string file, bool is_srgb = false);
        RenderMeshData               LoadMeshData(const MeshSourceDesc& source, AxisAlignedBox& bounding_box);
        RenderMaterialData           LoadMaterialData(const MaterialSourceDesc& source);
        AxisAlignedBox               GetCachedBoudingBox(const MeshSourceDesc& source) const;

    private:
        StaticMeshData loadStaticMesh(std::string mesh_file, AxisAlignedBox& bounding_box);

        std::unordered_map<MeshSourceDesc, AxisAlignedBox> mBoundingBoxCacheMap;
    };
}