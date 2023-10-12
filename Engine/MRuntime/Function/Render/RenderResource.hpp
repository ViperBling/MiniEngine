#pragma once

#include "MRuntime/Function/Render/RenderResourceBase.hpp"
#include "MRuntime/Function/Render/RenderType.hpp"
#include "MRuntime/Function/Render/Interface/RHI.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <map>
#include <vector>
#include <cmath>

namespace MiniEngine
{
    class RHI;
    class RenderPassBase;
    class RenderCamera;

    struct IBLResource
    {
        RHIImage*       mBrdfLutTextureImage;
        RHIImageView*   mBrdfLutTextureImageView;
        RHISampler*     mBrdfLutTextureSampler;
        VmaAllocation   mBrdfLutTextureImageAllocation;

        RHIImage*       mIrradianceTextureImage;
        RHIImageView*   mIrradianceTextureImageView;
        RHISampler*     mIrradianceTextureSampler;
        VmaAllocation   mIrradianceTextureImageAllocation;

        RHIImage*       mSpecularTextureImage;
        RHIImageView*   mSpecularTextureImageView;
        RHISampler*     mSpecularTextureSampler;
        VmaAllocation   mSpecularTextureImageAllocation;
    };

    struct IBLResourceData
    {
        void*                mBrdfLutTextureImagePixels;
        uint32_t             mBrdfLutTextureImageWidth;
        uint32_t             mBrdfLutTextureImageHeight;
        RHIFormat            mBrdfLutTextureImageFormat;
        std::array<void*, 6> mIrradianceTextureImagePixels;
        uint32_t             mIrradianceTextureImageWidth;
        uint32_t             mIrradianceTextureImageHeight;
        RHIFormat            mIrradianceTextureImageFormat;
        std::array<void*, 6> mSpecularTextureImagePixels;
        uint32_t             mSpecularTextureImageWidth;
        uint32_t             mSpecularTextureImageHeight;
        RHIFormat            mSpecularTextureImageFormat;
    };

    struct ColorGradingResource
    {
        RHIImage*       mColorGradientLutTextureImage;
        RHIImageView*   mColorGradientLutTextureImageView;
        VmaAllocation   mColorGradientLutTextureImageAllocation;
    };

    struct ColorGradingResourceData
    {
        void*           mColorGradientLutTextureImagePixels;
        uint32_t        mColorGradientLutTextureImageWidth;
        uint32_t        mColorGradientLutTextureImageHeight;
        RHIFormat       mColorGradientLutTextureImageFormat;
    };

    struct StorageBuffer
    {
        // limits
        uint32_t                mMinUniformBufferOffsetAlignment{ 256 };
        uint32_t                mMinStorageBufferOffsetAlignment{ 256 };
        uint32_t                mMaxStorageBufferRange{ 1 << 27 };
        uint32_t                mNonCoherentAtomSize{ 256 };

        RHIBuffer*              mGlobalUploadRingbuffer;
        RHIDeviceMemory*        mGlobalUploadRingbufferMemory;
        void*                   mGlobalUploadRingbufferMemoryPointer;
        std::vector<uint32_t>   mGlobalUploadRingbuffersBegin;
        std::vector<uint32_t>   mGlobalUploadRingbuffersEnd;
        std::vector<uint32_t>   mGlobalUploadRingbuffersSize;

        RHIBuffer*              mGlobalNullDescStorageBuffer;
        RHIDeviceMemory*        mGlobalNullDescStorageBufferMemory;

        // axis
        RHIBuffer*              mAxisInefficientStorageBuffer;
        RHIDeviceMemory*        mAxisInefficientStorageBufferMemory;
        void*                   mAxisInefficientStorageBufferMemoryPointer;
    };

    struct GlobalRenderResource
    {
        IBLResource          mIBLResource;
        ColorGradingResource mColorGradientResource;
        StorageBuffer        mStorageBuffer;
    };

    class RenderResource : public RenderResourceBase
    {
    public:
        void Clear() override final;

        virtual void UploadGlobalRenderResource(std::shared_ptr<RHI> rhi,
            SceneResourceDesc    level_resource_desc) override final;

        virtual void UploadGameObjectRenderResource(std::shared_ptr<RHI> rhi,
            RenderEntity         render_entity,
            RenderMeshData       mesh_data,
            RenderMaterialData   material_data) override final;

        virtual void UploadGameObjectRenderResource(std::shared_ptr<RHI> rhi,
            RenderEntity         render_entity,
            RenderMeshData       mesh_data) override final;

        virtual void UploadGameObjectRenderResource(std::shared_ptr<RHI> rhi,
            RenderEntity         render_entity,
            RenderMaterialData   material_data) override final;

        virtual void UpdatePerFrameBuffer(std::shared_ptr<RenderScene>  render_scene,
            std::shared_ptr<RenderCamera> camera) override final;

        VulkanMesh& getEntityMesh(RenderEntity entity);

        VulkanPBRMaterial& getEntityMaterial(RenderEntity entity);

        void resetRingBufferOffset(uint8_t current_frame_index);

    private:
        void createAndMapStorageBuffer(std::shared_ptr<RHI> rhi);
        void createIBLSamplers(std::shared_ptr<RHI> rhi);
        void createIBLTextures(std::shared_ptr<RHI>                        rhi,
                               std::array<std::shared_ptr<TextureData>, 6> irradiance_maps,
                               std::array<std::shared_ptr<TextureData>, 6> specular_maps);

        VulkanMesh& getOrCreateVulkanMesh(std::shared_ptr<RHI> rhi, RenderEntity entity, RenderMeshData mesh_data);
        VulkanPBRMaterial&
        getOrCreateVulkanMaterial(std::shared_ptr<RHI> rhi, RenderEntity entity, RenderMaterialData material_data);

        void updateMeshData(std::shared_ptr<RHI>                          rhi,
                            bool                                          enable_vertex_blending,
                            uint32_t                                      index_buffer_size,
                            void*                                         index_buffer_data,
                            uint32_t                                      vertex_buffer_size,
                            struct MeshVertexDataDefinition const*        vertex_buffer_data,
                            uint32_t                                      joint_binding_buffer_size,
                            struct MeshVertexBindingDataDefinition const* joint_binding_buffer_data,
                            VulkanMesh&                                   now_mesh);
        void updateVertexBuffer(std::shared_ptr<RHI>                          rhi,
                                bool                                          enable_vertex_blending,
                                uint32_t                                      vertex_buffer_size,
                                struct MeshVertexDataDefinition const*        vertex_buffer_data,
                                uint32_t                                      joint_binding_buffer_size,
                                struct MeshVertexBindingDataDefinition const* joint_binding_buffer_data,
                                uint32_t                                      index_buffer_size,
                                uint16_t*                                     index_buffer_data,
                                VulkanMesh&                                   now_mesh);
        void updateIndexBuffer(std::shared_ptr<RHI> rhi,
                               uint32_t             index_buffer_size,
                               void*                index_buffer_data,
                               VulkanMesh&          now_mesh);
        void updateTextureImageData(std::shared_ptr<RHI> rhi, const TextureDataToUpdate& texture_data);

    public:
        // global rendering resource, include IBL data, global storage buffer
        GlobalRenderResource mGlobalRenderResource;

        // storage buffer objects
        MeshPerframeStorageBufferObject                 mMeshPerFrameStorageBufferObject;
        MeshPointLightShadowPerframeStorageBufferObject mMeshPointLightShadowPerFrameStorageBufferObject;
        MeshDirectionalLightShadowPerframeStorageBufferObject mMeshDirectionalLightShadowPerFrameStorageBufferObject;
        AxisStorageBufferObject                        mAxisStorageBufferObject;
        MeshInefficientPickPerframeStorageBufferObject mMeshInEffPickPerFrameStorageBufferObject;

        // cached mesh and material
        std::map<size_t, VulkanMesh>        mVulkanMesh;
        std::map<size_t, VulkanPBRMaterial> mVulkanPBRMaterial;

        // descriptor set layout in main camera pass will be used when uploading resource
        RHIDescriptorSetLayout* const* mMeshDescLayout {nullptr};
        RHIDescriptorSetLayout* const* mMaterialDescLayout {nullptr};
    };
} // namespace MiniEngine
