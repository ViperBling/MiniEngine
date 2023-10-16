#include "RenderResource.hpp"

#include "MRuntime/Function/Render/RenderCamera.hpp"
#include "MRuntime/Function/Render/RenderHelper.hpp"
#include "MRuntime/Function/Render/RenderMesh.hpp"
#include "MRuntime/Function/Render/Interface/Vulkan/VulkanRHI.hpp"
#include "MRuntime/Function/Render/Interface/Vulkan/VulkanUtil.hpp"
#include "MRuntime/Function/Render/Passes/MainCameraPass.hpp"
#include "MRuntime/Core/Base/Marco.hpp"

#include <stdexcept>

namespace MiniEngine
{
    void RenderResource::Clear()
    {
    }

    void RenderResource::UploadGlobalRenderResource(std::shared_ptr<RHI> rhi, SceneResourceDesc level_resource_desc)
    {
        // create and map global storage buffer
        createAndMapStorageBuffer(rhi);

        // sky box irradiance
        SkyBoxIrradianceMap skybox_irradiance_map        = level_resource_desc.mIBLResourceDesc.mSkyboxIrradianceMap;
        std::shared_ptr<TextureData> irradiace_pos_x_map = LoadTextureHDR(skybox_irradiance_map.mPositiveXMap);
        std::shared_ptr<TextureData> irradiace_neg_x_map = LoadTextureHDR(skybox_irradiance_map.mNegativeXMap);
        std::shared_ptr<TextureData> irradiace_pos_y_map = LoadTextureHDR(skybox_irradiance_map.mPositiveYMap);
        std::shared_ptr<TextureData> irradiace_neg_y_map = LoadTextureHDR(skybox_irradiance_map.mNegativeYMap);
        std::shared_ptr<TextureData> irradiace_pos_z_map = LoadTextureHDR(skybox_irradiance_map.mPositiveZMap);
        std::shared_ptr<TextureData> irradiace_neg_z_map = LoadTextureHDR(skybox_irradiance_map.mNegativeZMap);

        // sky box specular
        SkyBoxSpecularMap            skybox_specular_map = level_resource_desc.mIBLResourceDesc.mSkyboxSpecularMap;
        std::shared_ptr<TextureData> specular_pos_x_map  = LoadTextureHDR(skybox_specular_map.mPositiveXMap);
        std::shared_ptr<TextureData> specular_neg_x_map  = LoadTextureHDR(skybox_specular_map.mNegativeXMap);
        std::shared_ptr<TextureData> specular_pos_y_map  = LoadTextureHDR(skybox_specular_map.mPositiveYMap);
        std::shared_ptr<TextureData> specular_neg_y_map  = LoadTextureHDR(skybox_specular_map.mNegativeYMap);
        std::shared_ptr<TextureData> specular_pos_z_map  = LoadTextureHDR(skybox_specular_map.mPositiveZMap);
        std::shared_ptr<TextureData> specular_neg_z_map  = LoadTextureHDR(skybox_specular_map.mNegativeZMap);

        // brdf
        std::shared_ptr<TextureData> brdf_map = LoadTextureHDR(level_resource_desc.mIBLResourceDesc.mBrdfMap);

        // create IBL samplers
        createIBLSamplers(rhi);

        // create IBL textures, take care of the texture order
        std::array<std::shared_ptr<TextureData>, 6> irradiance_maps = {
            irradiace_pos_x_map, irradiace_neg_x_map,
            irradiace_pos_z_map, irradiace_neg_z_map,
            irradiace_pos_y_map, irradiace_neg_y_map};
        std::array<std::shared_ptr<TextureData>, 6> specular_maps = {
            specular_pos_x_map, specular_neg_x_map,
            specular_pos_z_map, specular_neg_z_map,
            specular_pos_y_map, specular_neg_y_map};

        createIBLTextures(rhi, irradiance_maps, specular_maps);

        // create brdf lut texture
        rhi->CreateGlobalImage(
            mGlobalRenderResource.mIBLResource.mBrdfLutTextureImage,
            mGlobalRenderResource.mIBLResource.mBrdfLutTextureImageView,
            mGlobalRenderResource.mIBLResource.mBrdfLutTextureImageAllocation,
            brdf_map->mWidth,
            brdf_map->mHeight,
            brdf_map->mPixels,
            brdf_map->mFormat);

        // color grading
        std::shared_ptr<TextureData> color_grading_map =
            LoadTexture(level_resource_desc.mColorGradientResourceDesc.mColorGradientMap);

        // create color grading texture
        rhi->CreateGlobalImage(
            mGlobalRenderResource.mColorGradientResource.mColorGradientLutTextureImage,
            mGlobalRenderResource.mColorGradientResource.mColorGradientLutTextureImageView,
            mGlobalRenderResource.mColorGradientResource.mColorGradientLutTextureImageAllocation,
            color_grading_map->mWidth,
            color_grading_map->mHeight,
            color_grading_map->mPixels,
            color_grading_map->mFormat);
    }

    void RenderResource::UploadGameObjectRenderResource(std::shared_ptr<RHI> rhi,
        RenderEntity         render_entity,
        RenderMeshData       mesh_data,
        RenderMaterialData   material_data)
    {
        getOrCreateVulkanMesh(rhi, render_entity, mesh_data);
        getOrCreateVulkanMaterial(rhi, render_entity, material_data);
    }

    void RenderResource::UploadGameObjectRenderResource(std::shared_ptr<RHI> rhi,
        RenderEntity         render_entity,
        RenderMeshData       mesh_data)
    {
        getOrCreateVulkanMesh(rhi, render_entity, mesh_data);
    }

    void RenderResource::UploadGameObjectRenderResource(std::shared_ptr<RHI> rhi,
        RenderEntity         render_entity,
        RenderMaterialData   material_data)
    {
        getOrCreateVulkanMaterial(rhi, render_entity, material_data);
    }

    void RenderResource::UpdatePerFrameBuffer(std::shared_ptr<RenderScene>  render_scene,
        std::shared_ptr<RenderCamera> camera)
    {
        Matrix4x4 view_matrix = camera->GetViewMatrix();
        Matrix4x4 proj_matrix = camera->GetPersProjMatrix();
        Vector3   camera_position = camera->Position();
        Matrix4x4 proj_view_matrix = proj_matrix * view_matrix;

        // ambient light
        Vector3  ambient_light = render_scene->mAmbientLight.mIrradiance;
        uint32_t point_light_num = static_cast<uint32_t>(render_scene->mPointLightList.mLights.size());

        mMeshPerFrameStorageBufferObject.proj_view_matrix = proj_view_matrix;
        mMeshPerFrameStorageBufferObject.camera_position = camera_position;
        mMeshPerFrameStorageBufferObject.ambient_light = ambient_light;
        mMeshPerFrameStorageBufferObject.point_light_num = point_light_num;

        mMeshPointLightShadowPerFrameStorageBufferObject.point_light_num = point_light_num;
        // point lights
        for (uint32_t i = 0; i < point_light_num; i++)
        {
            Vector3 point_light_position = render_scene->mPointLightList.mLights[i].mPosition;
            Vector3 point_light_intensity =
                render_scene->mPointLightList.mLights[i].mFlux / (4.0f * MATH_PI);

            float radius = render_scene->mPointLightList.mLights[i].CalculateRadius();

            mMeshPerFrameStorageBufferObject.scene_point_lights[i].position  = point_light_position;
            mMeshPerFrameStorageBufferObject.scene_point_lights[i].radius    = radius;
            mMeshPerFrameStorageBufferObject.scene_point_lights[i].intensity = point_light_intensity;

            mMeshPointLightShadowPerFrameStorageBufferObject.point_lights_position_and_radius[i] =
                Vector4(point_light_position, radius);
        }

        // directional light
        mMeshPerFrameStorageBufferObject.scene_directional_light.direction =
            render_scene->mDirectionalLight.mDirection.NormalizedCopy();
        mMeshPerFrameStorageBufferObject.scene_directional_light.color = render_scene->mDirectionalLight.mColor;

        // pick pass view projection matrix
        mMeshInEffPickPerFrameStorageBufferObject.proj_view_matrix = proj_view_matrix;
    }

    void RenderResource::createIBLSamplers(std::shared_ptr<RHI> rhi)
    {
        VulkanRHI* raw_rhi = static_cast<VulkanRHI*>(rhi.get());

        RHIPhysicalDeviceProperties physical_device_properties{};
        rhi->GetPhysicalDeviceProperties(&physical_device_properties);

        RHISamplerCreateInfo samplerInfo{};
        samplerInfo.sType = RHI_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = RHI_FILTER_LINEAR;
        samplerInfo.minFilter = RHI_FILTER_LINEAR;
        samplerInfo.addressModeU = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = RHI_TRUE;                                                // close:false
        samplerInfo.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy; // close :1.0f
        samplerInfo.borderColor = RHI_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = RHI_FALSE;
        samplerInfo.compareEnable = RHI_FALSE;
        samplerInfo.compareOp = RHI_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = RHI_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.maxLod = 0.0f;

        if (mGlobalRenderResource.mIBLResource.mBrdfLutTextureSampler != RHI_NULL_HANDLE)
        {
            rhi->DestroySampler(mGlobalRenderResource.mIBLResource.mBrdfLutTextureSampler);
        }

        if (rhi->CreateSampler(&samplerInfo, mGlobalRenderResource.mIBLResource.mBrdfLutTextureSampler) != RHI_SUCCESS)
        {
            throw std::runtime_error("vk create sampler");
        }

        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 8.0f; // TODO: irradiance_texture_miplevels
        samplerInfo.mipLodBias = 0.0f;

        if (mGlobalRenderResource.mIBLResource.mIrradianceTextureSampler != RHI_NULL_HANDLE)
        {
            rhi->DestroySampler(mGlobalRenderResource.mIBLResource.mIrradianceTextureSampler);
        }

        if (rhi->CreateSampler(&samplerInfo, mGlobalRenderResource.mIBLResource.mIrradianceTextureSampler) != RHI_SUCCESS)
        {
            throw std::runtime_error("vk create sampler");
        }

        if (mGlobalRenderResource.mIBLResource.mSpecularTextureSampler != RHI_NULL_HANDLE)
        {
            rhi->DestroySampler(mGlobalRenderResource.mIBLResource.mSpecularTextureSampler);
        }

        if (rhi->CreateSampler(&samplerInfo, mGlobalRenderResource.mIBLResource.mSpecularTextureSampler) != RHI_SUCCESS)
        {
            throw std::runtime_error("vk create sampler");
        }
    }

    void RenderResource::createIBLTextures(std::shared_ptr<RHI>                        rhi,
        std::array<std::shared_ptr<TextureData>, 6> irradiance_maps,
        std::array<std::shared_ptr<TextureData>, 6> specular_maps)
    {
        // assume all textures have same width, height and format
        uint32_t irradiance_cubemap_miplevels =
            static_cast<uint32_t>(
                std::floor(log2(std::max(irradiance_maps[0]->mWidth, irradiance_maps[0]->mHeight)))) +
            1;
        rhi->CreateCubeMap(
            mGlobalRenderResource.mIBLResource.mIrradianceTextureImage,
            mGlobalRenderResource.mIBLResource.mIrradianceTextureImageView,
            mGlobalRenderResource.mIBLResource.mIrradianceTextureImageAllocation,
            irradiance_maps[0]->mWidth,
            irradiance_maps[0]->mHeight,
            { irradiance_maps[0]->mPixels,
             irradiance_maps[1]->mPixels,
             irradiance_maps[2]->mPixels,
             irradiance_maps[3]->mPixels,
             irradiance_maps[4]->mPixels,
             irradiance_maps[5]->mPixels },
            irradiance_maps[0]->mFormat,
            irradiance_cubemap_miplevels);

        uint32_t specular_cubemap_miplevels =
            static_cast<uint32_t>(
                std::floor(log2(std::max(specular_maps[0]->mWidth, specular_maps[0]->mHeight)))) +
            1;
        rhi->CreateCubeMap(
            mGlobalRenderResource.mIBLResource.mSpecularTextureImage,
            mGlobalRenderResource.mIBLResource.mSpecularTextureImageView,
            mGlobalRenderResource.mIBLResource.mSpecularTextureImageAllocation,
            specular_maps[0]->mWidth,
            specular_maps[0]->mHeight,
            { specular_maps[0]->mPixels,
             specular_maps[1]->mPixels,
             specular_maps[2]->mPixels,
             specular_maps[3]->mPixels,
             specular_maps[4]->mPixels,
             specular_maps[5]->mPixels },
            specular_maps[0]->mFormat,
            specular_cubemap_miplevels);
    }

    VulkanMesh&
        RenderResource::getOrCreateVulkanMesh(std::shared_ptr<RHI> rhi, RenderEntity entity, RenderMeshData mesh_data)
    {
        size_t assetid = entity.mMaterialAssetID;

        auto it = mVulkanMesh.find(assetid);
        if (it != mVulkanMesh.end())
        {
            return it->second;
        }
        else
        {
            VulkanMesh temp;
            auto       res = mVulkanMesh.insert(std::make_pair(assetid, std::move(temp)));
            assert(res.second);

            uint32_t index_buffer_size = static_cast<uint32_t>(mesh_data.mStaticMeshData.mIndexBuffer->mSize);
            void* index_buffer_data = mesh_data.mStaticMeshData.mIndexBuffer->mData;

            uint32_t vertex_buffer_size = static_cast<uint32_t>(mesh_data.mStaticMeshData.mVertexBuffer->mSize);
            MeshVertexDataDefinition* vertex_buffer_data =
                reinterpret_cast<MeshVertexDataDefinition*>(mesh_data.mStaticMeshData.mVertexBuffer->mData);

            VulkanMesh& now_mesh = res.first->second;

            if (mesh_data.mSkeletonBindingBuffer)
            {
                uint32_t joint_binding_buffer_size = (uint32_t)mesh_data.mSkeletonBindingBuffer->mSize;
                MeshVertexBindingDataDefinition* joint_binding_buffer_data =
                    reinterpret_cast<MeshVertexBindingDataDefinition*>(mesh_data.mSkeletonBindingBuffer->mData);
                updateMeshData(rhi,
                               true,
                               index_buffer_size,
                               index_buffer_data,
                               vertex_buffer_size,
                               vertex_buffer_data,
                               joint_binding_buffer_size,
                               joint_binding_buffer_data,
                               now_mesh);
            }
            else
            {
                updateMeshData(rhi,
                               false,
                               index_buffer_size,
                               index_buffer_data,
                               vertex_buffer_size,
                               vertex_buffer_data,
                               0,
                               NULL,
                               now_mesh);
            }

            return now_mesh;
        }
    }

    VulkanPBRMaterial& RenderResource::getOrCreateVulkanMaterial(std::shared_ptr<RHI> rhi,
        RenderEntity         entity,
        RenderMaterialData   material_data)
    {
        VulkanRHI* vulkan_context = static_cast<VulkanRHI*>(rhi.get());

        size_t assetid = entity.mMaterialAssetID;

        auto it = mVulkanPBRMaterial.find(assetid);
        if (it != mVulkanPBRMaterial.end())
        {
            return it->second;
        }
        else
        {
            VulkanPBRMaterial temp;
            auto              res = mVulkanPBRMaterial.insert(std::make_pair(assetid, std::move(temp)));
            assert(res.second);

            float empty_image[] = { 0.5f, 0.5f, 0.5f, 0.5f };

            void* base_color_image_pixels = empty_image;
            uint32_t           base_color_image_width = 1;
            uint32_t           base_color_image_height = 1;
            RHIFormat base_color_image_format = RHIFormat::RHI_FORMAT_R8G8B8A8_SRGB;
            if (material_data.mBaseColorTexture)
            {
                base_color_image_pixels = material_data.mBaseColorTexture->mPixels;
                base_color_image_width = static_cast<uint32_t>(material_data.mBaseColorTexture->mWidth);
                base_color_image_height = static_cast<uint32_t>(material_data.mBaseColorTexture->mHeight);
                base_color_image_format = material_data.mBaseColorTexture->mFormat;
            }

            void* metallic_roughness_image_pixels = empty_image;
            uint32_t           metallic_roughness_width = 1;
            uint32_t           metallic_roughness_height = 1;
            RHIFormat metallic_roughness_format = RHIFormat::RHI_FORMAT_R8G8B8A8_UNORM;
            if (material_data.mMetallicRoughnessTexture)
            {
                metallic_roughness_image_pixels = material_data.mMetallicRoughnessTexture->mPixels;
                metallic_roughness_width = static_cast<uint32_t>(material_data.mMetallicRoughnessTexture->mWidth);
                metallic_roughness_height = static_cast<uint32_t>(material_data.mMetallicRoughnessTexture->mHeight);
                metallic_roughness_format = material_data.mMetallicRoughnessTexture->mFormat;
            }

            void* normal_roughness_image_pixels = empty_image;
            uint32_t           normal_roughness_width = 1;
            uint32_t           normal_roughness_height = 1;
            RHIFormat normal_roughness_format = RHIFormat::RHI_FORMAT_R8G8B8A8_UNORM;
            if (material_data.mNormalTexture)
            {
                normal_roughness_image_pixels = material_data.mNormalTexture->mPixels;
                normal_roughness_width = static_cast<uint32_t>(material_data.mNormalTexture->mWidth);
                normal_roughness_height = static_cast<uint32_t>(material_data.mNormalTexture->mHeight);
                normal_roughness_format = material_data.mNormalTexture->mFormat;
            }

            void* occlusion_image_pixels = empty_image;
            uint32_t           occlusion_image_width = 1;
            uint32_t           occlusion_image_height = 1;
            RHIFormat occlusion_image_format = RHIFormat::RHI_FORMAT_R8G8B8A8_UNORM;
            if (material_data.mOcclusionTexture)
            {
                occlusion_image_pixels = material_data.mOcclusionTexture->mPixels;
                occlusion_image_width = static_cast<uint32_t>(material_data.mOcclusionTexture->mWidth);
                occlusion_image_height = static_cast<uint32_t>(material_data.mOcclusionTexture->mHeight);
                occlusion_image_format = material_data.mOcclusionTexture->mFormat;
            }

            void* emissive_image_pixels = empty_image;
            uint32_t           emissive_image_width = 1;
            uint32_t           emissive_image_height = 1;
            RHIFormat emissive_image_format = RHIFormat::RHI_FORMAT_R8G8B8A8_UNORM;
            if (material_data.mEmissiveTexture)
            {
                emissive_image_pixels = material_data.mEmissiveTexture->mPixels;
                emissive_image_width  = static_cast<uint32_t>(material_data.mEmissiveTexture->mWidth);
                emissive_image_height = static_cast<uint32_t>(material_data.mEmissiveTexture->mHeight);
                emissive_image_format = material_data.mEmissiveTexture->mFormat;
            }

            VulkanPBRMaterial& now_material = res.first->second;

            // similiarly to the vertex/index buffer, we should allocate the uniform
            // buffer in DEVICE_LOCAL memory and use the temp stage buffer to copy the
            // data
            {
                // temporary staging buffer

                RHIDeviceSize buffer_size = sizeof(MeshPerMaterialUniformBufferObject);

                RHIBuffer* inefficient_staging_buffer = RHI_NULL_HANDLE;
                RHIDeviceMemory* inefficient_staging_buffer_memory = RHI_NULL_HANDLE;
                rhi->CreateBuffer(
                    buffer_size,
                    RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    inefficient_staging_buffer,
                    inefficient_staging_buffer_memory);
                // RHI_BUFFER_USAGE_TRANSFER_SRC_BIT: buffer can be used as source in a
                // memory transfer operation

                void* staging_buffer_data = nullptr;
                rhi->MapMemory(
                    inefficient_staging_buffer_memory,
                    0,
                    buffer_size,
                    0,
                    &staging_buffer_data);
                
                MeshPerMaterialUniformBufferObject& material_uniform_buffer_info =
                    (*static_cast<MeshPerMaterialUniformBufferObject*>(staging_buffer_data));
                material_uniform_buffer_info.is_blend = entity.mBlend;
                material_uniform_buffer_info.is_double_sided = entity.mDoubleSided;
                material_uniform_buffer_info.baseColorFactor = entity.mBaseColorFactor;
                material_uniform_buffer_info.metallicFactor = entity.mMetalicFactor;
                material_uniform_buffer_info.roughnessFactor = entity.mRoughnessFactor;
                material_uniform_buffer_info.normalScale = entity.mNormalScale;
                material_uniform_buffer_info.occlusionStrength = entity.mOcclusionStrength;
                material_uniform_buffer_info.emissiveFactor = entity.mEmissiveFactor;

                rhi->UnmapMemory(inefficient_staging_buffer_memory);

                // use the vmaAllocator to allocate asset uniform buffer
                RHIBufferCreateInfo bufferInfo = { RHI_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
                bufferInfo.size = buffer_size;
                bufferInfo.usage = RHI_BUFFER_USAGE_UNIFORM_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT;

                VmaAllocationCreateInfo allocInfo = {};
                allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

                rhi->CreateBufferWithAlignmentVMA(
                    vulkan_context->mAssetsAllocator,
                    &bufferInfo,
                    &allocInfo,
                    mGlobalRenderResource.mStorageBuffer.mMinUniformBufferOffsetAlignment,
                    now_material.material_uniform_buffer,
                    &now_material.material_uniform_buffer_allocation,
                    NULL);

                // use the data from staging buffer
                rhi->CopyBuffer(inefficient_staging_buffer, now_material.material_uniform_buffer, 0, 0, buffer_size);

                // release staging buffer
                rhi->DestroyBuffer(inefficient_staging_buffer);
                rhi->FreeMemory(inefficient_staging_buffer_memory);
            }

            TextureDataToUpdate update_texture_data;
            update_texture_data.base_color_image_pixels         = base_color_image_pixels;
            update_texture_data.base_color_image_width          = base_color_image_width;
            update_texture_data.base_color_image_height         = base_color_image_height;
            update_texture_data.base_color_image_format         = base_color_image_format;
            update_texture_data.metallic_roughness_image_pixels = metallic_roughness_image_pixels;
            update_texture_data.metallic_roughness_image_width  = metallic_roughness_width;
            update_texture_data.metallic_roughness_image_height = metallic_roughness_height;
            update_texture_data.metallic_roughness_image_format = metallic_roughness_format;
            update_texture_data.normal_roughness_image_pixels   = normal_roughness_image_pixels;
            update_texture_data.normal_roughness_image_width    = normal_roughness_width;
            update_texture_data.normal_roughness_image_height   = normal_roughness_height;
            update_texture_data.normal_roughness_image_format   = normal_roughness_format;
            update_texture_data.occlusion_image_pixels          = occlusion_image_pixels;
            update_texture_data.occlusion_image_width           = occlusion_image_width;
            update_texture_data.occlusion_image_height          = occlusion_image_height;
            update_texture_data.occlusion_image_format          = occlusion_image_format;
            update_texture_data.emissive_image_pixels           = emissive_image_pixels;
            update_texture_data.emissive_image_width            = emissive_image_width;
            update_texture_data.emissive_image_height           = emissive_image_height;
            update_texture_data.emissive_image_format           = emissive_image_format;
            update_texture_data.now_material                    = &now_material;

            updateTextureImageData(rhi, update_texture_data);

            RHIDescriptorSetAllocateInfo material_descriptor_set_alloc_info;
            material_descriptor_set_alloc_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            material_descriptor_set_alloc_info.pNext = NULL;
            material_descriptor_set_alloc_info.descriptorPool = vulkan_context->mDescPool;
            material_descriptor_set_alloc_info.descriptorSetCount = 1;
            material_descriptor_set_alloc_info.pSetLayouts        = mMaterialDescLayout;

            if (RHI_SUCCESS != rhi->AllocateDescriptorSets(
                &material_descriptor_set_alloc_info,
                now_material.material_descriptor_set))
            {
                throw std::runtime_error("allocate material descriptor set");
            }

            RHIDescriptorBufferInfo material_uniform_buffer_info = {};
            material_uniform_buffer_info.offset = 0;
            material_uniform_buffer_info.range = sizeof(MeshPerMaterialUniformBufferObject);
            material_uniform_buffer_info.buffer = now_material.material_uniform_buffer;

            RHIDescriptorImageInfo base_color_image_info = {};
            base_color_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            base_color_image_info.imageView = now_material.base_color_image_view;
            base_color_image_info.sampler = rhi->GetOrCreateMipmapSampler(base_color_image_width,
                                                                          base_color_image_height);

            RHIDescriptorImageInfo metallic_roughness_image_info = {};
            metallic_roughness_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            metallic_roughness_image_info.imageView = now_material.metallic_roughness_image_view;
            metallic_roughness_image_info.sampler = rhi->GetOrCreateMipmapSampler(metallic_roughness_width,
                                                                                  metallic_roughness_height);

            RHIDescriptorImageInfo normal_roughness_image_info = {};
            normal_roughness_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normal_roughness_image_info.imageView = now_material.normal_image_view;
            normal_roughness_image_info.sampler = rhi->GetOrCreateMipmapSampler(normal_roughness_width,
                                                                                normal_roughness_height);

            RHIDescriptorImageInfo occlusion_image_info = {};
            occlusion_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            occlusion_image_info.imageView = now_material.occlusion_image_view;
            occlusion_image_info.sampler = rhi->GetOrCreateMipmapSampler(occlusion_image_width,occlusion_image_height);

            RHIDescriptorImageInfo emissive_image_info = {};
            emissive_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            emissive_image_info.imageView = now_material.emissive_image_view;
            emissive_image_info.sampler = rhi->GetOrCreateMipmapSampler(emissive_image_width, emissive_image_height);

            RHIWriteDescriptorSet mesh_descriptor_writes_info[6];

            mesh_descriptor_writes_info[0].sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            mesh_descriptor_writes_info[0].pNext = NULL;
            mesh_descriptor_writes_info[0].dstSet = now_material.material_descriptor_set;
            mesh_descriptor_writes_info[0].dstBinding = 0;
            mesh_descriptor_writes_info[0].dstArrayElement = 0;
            mesh_descriptor_writes_info[0].descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            mesh_descriptor_writes_info[0].descriptorCount = 1;
            mesh_descriptor_writes_info[0].pBufferInfo = &material_uniform_buffer_info;

            mesh_descriptor_writes_info[1].sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            mesh_descriptor_writes_info[1].pNext = NULL;
            mesh_descriptor_writes_info[1].dstSet = now_material.material_descriptor_set;
            mesh_descriptor_writes_info[1].dstBinding = 1;
            mesh_descriptor_writes_info[1].dstArrayElement = 0;
            mesh_descriptor_writes_info[1].descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            mesh_descriptor_writes_info[1].descriptorCount = 1;
            mesh_descriptor_writes_info[1].pImageInfo = &base_color_image_info;

            mesh_descriptor_writes_info[2] = mesh_descriptor_writes_info[1];
            mesh_descriptor_writes_info[2].dstBinding = 2;
            mesh_descriptor_writes_info[2].pImageInfo = &metallic_roughness_image_info;

            mesh_descriptor_writes_info[3] = mesh_descriptor_writes_info[1];
            mesh_descriptor_writes_info[3].dstBinding = 3;
            mesh_descriptor_writes_info[3].pImageInfo = &normal_roughness_image_info;

            mesh_descriptor_writes_info[4] = mesh_descriptor_writes_info[1];
            mesh_descriptor_writes_info[4].dstBinding = 4;
            mesh_descriptor_writes_info[4].pImageInfo = &occlusion_image_info;

            mesh_descriptor_writes_info[5] = mesh_descriptor_writes_info[1];
            mesh_descriptor_writes_info[5].dstBinding = 5;
            mesh_descriptor_writes_info[5].pImageInfo = &emissive_image_info;

            rhi->UpdateDescriptorSets(6, mesh_descriptor_writes_info, 0, nullptr);

            return now_material;
        }
    }

    void RenderResource::updateMeshData(std::shared_ptr<RHI>                   rhi,
                                        bool                                   enable_vertex_blending,
                                        uint32_t                               index_buffer_size,
                                        void*                                  index_buffer_data,
                                        uint32_t                               vertex_buffer_size,
                                        MeshVertexDataDefinition const*        vertex_buffer_data,
                                        uint32_t                               joint_binding_buffer_size,
                                        MeshVertexBindingDataDefinition const* joint_binding_buffer_data,
                                        VulkanMesh&                            now_mesh)
    {
        now_mesh.enable_vertex_blending = enable_vertex_blending;
        assert(0 == (vertex_buffer_size % sizeof(MeshVertexDataDefinition)));
        now_mesh.mesh_vertex_count = vertex_buffer_size / sizeof(MeshVertexDataDefinition);
        updateVertexBuffer(rhi,
                           enable_vertex_blending,
                           vertex_buffer_size,
                           vertex_buffer_data,
                           joint_binding_buffer_size,
                           joint_binding_buffer_data,
                           index_buffer_size,
                           reinterpret_cast<uint16_t*>(index_buffer_data),
                           now_mesh);
        assert(0 == (index_buffer_size % sizeof(uint16_t)));
        now_mesh.mesh_index_count = index_buffer_size / sizeof(uint16_t);
        updateIndexBuffer(rhi, index_buffer_size, index_buffer_data, now_mesh);
    }

    void RenderResource::updateVertexBuffer(std::shared_ptr<RHI>                   rhi,
                                            bool                                   enable_vertex_blending,
                                            uint32_t                               vertex_buffer_size,
                                            MeshVertexDataDefinition const*        vertex_buffer_data,
                                            uint32_t                               joint_binding_buffer_size,
                                            MeshVertexBindingDataDefinition const* joint_binding_buffer_data,
                                            uint32_t                               index_buffer_size,
                                            uint16_t*                              index_buffer_data,
                                            VulkanMesh&                            now_mesh)
    {
        VulkanRHI* vulkan_context = static_cast<VulkanRHI*>(rhi.get());

        if (enable_vertex_blending)
        {
            assert(0 == (vertex_buffer_size % sizeof(MeshVertexDataDefinition)));
            uint32_t vertex_count = vertex_buffer_size / sizeof(MeshVertexDataDefinition);
            assert(0 == (index_buffer_size % sizeof(uint16_t)));
            uint32_t index_count = index_buffer_size / sizeof(uint16_t);

            RHIDeviceSize vertex_position_buffer_size = sizeof(MeshVertex::VulkanMeshVertexPostition) * vertex_count;
            RHIDeviceSize vertex_varying_enable_blending_buffer_size =
                sizeof(MeshVertex::VulkanMeshVertexVaryingEnableBlending) * vertex_count;
            RHIDeviceSize vertex_varying_buffer_size = sizeof(MeshVertex::VulkanMeshVertexVarying) * vertex_count;
            RHIDeviceSize vertex_joint_binding_buffer_size =
                sizeof(MeshVertex::VulkanMeshVertexJointBinding) * index_count;

            RHIDeviceSize vertex_position_buffer_offset = 0;
            RHIDeviceSize vertex_varying_enable_blending_buffer_offset =
                vertex_position_buffer_offset + vertex_position_buffer_size;
            RHIDeviceSize vertex_varying_buffer_offset =
                vertex_varying_enable_blending_buffer_offset + vertex_varying_enable_blending_buffer_size;
            RHIDeviceSize vertex_joint_binding_buffer_offset = vertex_varying_buffer_offset + vertex_varying_buffer_size;

            // temporary staging buffer
            RHIDeviceSize inefficient_staging_buffer_size =
                vertex_position_buffer_size + vertex_varying_enable_blending_buffer_size + vertex_varying_buffer_size +
                vertex_joint_binding_buffer_size;
            RHIBuffer* inefficient_staging_buffer = RHI_NULL_HANDLE;
            RHIDeviceMemory* inefficient_staging_buffer_memory = RHI_NULL_HANDLE;
            rhi->CreateBuffer(inefficient_staging_buffer_size,
                              RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              inefficient_staging_buffer,
                              inefficient_staging_buffer_memory);

            void* inefficient_staging_buffer_data;
            rhi->MapMemory(inefficient_staging_buffer_memory,
                           0,
                           RHI_WHOLE_SIZE,
                           0,
                           &inefficient_staging_buffer_data);

            MeshVertex::VulkanMeshVertexPostition* mesh_vertex_positions =
                reinterpret_cast<MeshVertex::VulkanMeshVertexPostition*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) + vertex_position_buffer_offset);
            MeshVertex::VulkanMeshVertexVaryingEnableBlending* mesh_vertex_blending_varyings =
                reinterpret_cast<MeshVertex::VulkanMeshVertexVaryingEnableBlending*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) +
                    vertex_varying_enable_blending_buffer_offset);
            MeshVertex::VulkanMeshVertexVarying* mesh_vertex_varyings =
                reinterpret_cast<MeshVertex::VulkanMeshVertexVarying*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) + vertex_varying_buffer_offset);
            MeshVertex::VulkanMeshVertexJointBinding* mesh_vertex_joint_binding =
                reinterpret_cast<MeshVertex::VulkanMeshVertexJointBinding*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) + vertex_joint_binding_buffer_offset);

            for (uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
            {
                Vector3 normal = Vector3(vertex_buffer_data[vertex_index].nx,
                    vertex_buffer_data[vertex_index].ny,
                    vertex_buffer_data[vertex_index].nz);
                Vector3 tangent = Vector3(vertex_buffer_data[vertex_index].tx,
                    vertex_buffer_data[vertex_index].ty,
                    vertex_buffer_data[vertex_index].tz);

                mesh_vertex_positions[vertex_index].position = Vector3(vertex_buffer_data[vertex_index].x,
                    vertex_buffer_data[vertex_index].y,
                    vertex_buffer_data[vertex_index].z);

                mesh_vertex_blending_varyings[vertex_index].normal = normal;
                mesh_vertex_blending_varyings[vertex_index].tangent = tangent;

                mesh_vertex_varyings[vertex_index].texcoord =
                    Vector2(vertex_buffer_data[vertex_index].u, vertex_buffer_data[vertex_index].v);
            }

            for (uint32_t index_index = 0; index_index < index_count; ++index_index)
            {
                uint32_t vertex_buffer_index = index_buffer_data[index_index];

                // TODO: move to assets loading process

                mesh_vertex_joint_binding[index_index].indices[0] = joint_binding_buffer_data[vertex_buffer_index].mIndex0;
                mesh_vertex_joint_binding[index_index].indices[1] = joint_binding_buffer_data[vertex_buffer_index].mIndex1;
                mesh_vertex_joint_binding[index_index].indices[2] = joint_binding_buffer_data[vertex_buffer_index].mIndex2;
                mesh_vertex_joint_binding[index_index].indices[3] = joint_binding_buffer_data[vertex_buffer_index].mIndex3;

                float inv_total_weight = joint_binding_buffer_data[vertex_buffer_index].mWeight0 +
                                         joint_binding_buffer_data[vertex_buffer_index].mWeight1 +
                                         joint_binding_buffer_data[vertex_buffer_index].mWeight2 +
                                         joint_binding_buffer_data[vertex_buffer_index].mWeight3;

                inv_total_weight = (inv_total_weight != 0.0) ? 1 / inv_total_weight : 1.0;

                mesh_vertex_joint_binding[index_index].weights =
                    Vector4(joint_binding_buffer_data[vertex_buffer_index].mWeight0 * inv_total_weight,
                        joint_binding_buffer_data[vertex_buffer_index].mWeight1 * inv_total_weight,
                        joint_binding_buffer_data[vertex_buffer_index].mWeight2 * inv_total_weight,
                        joint_binding_buffer_data[vertex_buffer_index].mWeight3 * inv_total_weight);
            }

            rhi->UnmapMemory(inefficient_staging_buffer_memory);

            // use the vmaAllocator to allocate asset vertex buffer
            RHIBufferCreateInfo bufferInfo = { RHI_STRUCTURE_TYPE_BUFFER_CREATE_INFO };

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            bufferInfo.usage = RHI_BUFFER_USAGE_VERTEX_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.size = vertex_position_buffer_size;
            rhi->CreateBufferVMA(vulkan_context->mAssetsAllocator,
                                 &bufferInfo,
                                 &allocInfo,
                                 now_mesh.mesh_vertex_position_buffer,
                                 &now_mesh.mesh_vertex_position_buffer_allocation,
                                 NULL);
            bufferInfo.size = vertex_varying_enable_blending_buffer_size;
            rhi->CreateBufferVMA(vulkan_context->mAssetsAllocator,
                                 &bufferInfo,
                                 &allocInfo,
                                 now_mesh.mesh_vertex_varying_enable_blending_buffer,
                                 &now_mesh.mesh_vertex_varying_enable_blending_buffer_allocation,
                                 NULL);
            bufferInfo.size = vertex_varying_buffer_size;
            rhi->CreateBufferVMA(vulkan_context->mAssetsAllocator,
                                 &bufferInfo,
                                 &allocInfo,
                                 now_mesh.mesh_vertex_varying_buffer,
                                 &now_mesh.mesh_vertex_varying_buffer_allocation,
                                 NULL);

            bufferInfo.usage = RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.size = vertex_joint_binding_buffer_size;
            rhi->CreateBufferVMA(vulkan_context->mAssetsAllocator,
                                 &bufferInfo,
                                 &allocInfo,
                                 now_mesh.mesh_vertex_joint_binding_buffer,
                                 &now_mesh.mesh_vertex_joint_binding_buffer_allocation,
                                 NULL);

            // use the data from staging buffer
            rhi->CopyBuffer(inefficient_staging_buffer,
                            now_mesh.mesh_vertex_position_buffer,
                            vertex_position_buffer_offset,
                            0,
                            vertex_position_buffer_size);
            rhi->CopyBuffer(inefficient_staging_buffer,
                            now_mesh.mesh_vertex_varying_enable_blending_buffer,
                            vertex_varying_enable_blending_buffer_offset,
                            0,
                            vertex_varying_enable_blending_buffer_size);
            rhi->CopyBuffer(inefficient_staging_buffer,
                            now_mesh.mesh_vertex_varying_buffer,
                            vertex_varying_buffer_offset,
                            0,
                            vertex_varying_buffer_size);
            rhi->CopyBuffer(inefficient_staging_buffer,
                            now_mesh.mesh_vertex_joint_binding_buffer,
                            vertex_joint_binding_buffer_offset,
                            0,
                            vertex_joint_binding_buffer_size);

            // release staging buffer
            rhi->DestroyBuffer(inefficient_staging_buffer);
            rhi->FreeMemory(inefficient_staging_buffer_memory);

            // update descriptor set
            RHIDescriptorSetAllocateInfo mesh_vertex_blending_per_mesh_descriptor_set_alloc_info;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.sType =
                RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.pNext = NULL;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.descriptorPool = vulkan_context->mDescPool;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.descriptorSetCount = 1;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.pSetLayouts        = mMeshDescLayout;

            if (RHI_SUCCESS != rhi->AllocateDescriptorSets(
                &mesh_vertex_blending_per_mesh_descriptor_set_alloc_info,
                now_mesh.mesh_vertex_blending_descriptor_set))
            {
                throw std::runtime_error("allocate mesh vertex blending per mesh descriptor set");
            }

            RHIDescriptorBufferInfo mesh_vertex_Joint_binding_storage_buffer_info = {};
            mesh_vertex_Joint_binding_storage_buffer_info.offset = 0;
            mesh_vertex_Joint_binding_storage_buffer_info.range = vertex_joint_binding_buffer_size;
            mesh_vertex_Joint_binding_storage_buffer_info.buffer = now_mesh.mesh_vertex_joint_binding_buffer;
            assert(mesh_vertex_Joint_binding_storage_buffer_info.range <
                mGlobalRenderResource.mStorageBuffer.mMaxStorageBufferRange);

            RHIDescriptorSet* descriptor_set_to_write = now_mesh.mesh_vertex_blending_descriptor_set;

            RHIWriteDescriptorSet descriptor_writes[1];

            RHIWriteDescriptorSet& mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info =
                descriptor_writes[0];
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.sType =
                RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.pNext = NULL;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.dstSet = descriptor_set_to_write;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.dstBinding = 0;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.dstArrayElement = 0;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.descriptorType =
                RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.descriptorCount = 1;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.pBufferInfo =
                &mesh_vertex_Joint_binding_storage_buffer_info;

            rhi->UpdateDescriptorSets((sizeof(descriptor_writes) / sizeof(descriptor_writes[0])),
                                      descriptor_writes,
                                      0,
                                      NULL);
        }
        else
        {
            assert(0 == (vertex_buffer_size % sizeof(MeshVertexDataDefinition)));
            uint32_t vertex_count = vertex_buffer_size / sizeof(MeshVertexDataDefinition);

            RHIDeviceSize vertex_position_buffer_size = sizeof(MeshVertex::VulkanMeshVertexPostition) * vertex_count;
            RHIDeviceSize vertex_varying_enable_blending_buffer_size =
                sizeof(MeshVertex::VulkanMeshVertexVaryingEnableBlending) * vertex_count;
            RHIDeviceSize vertex_varying_buffer_size = sizeof(MeshVertex::VulkanMeshVertexVarying) * vertex_count;

            RHIDeviceSize vertex_position_buffer_offset = 0;
            RHIDeviceSize vertex_varying_enable_blending_buffer_offset =
                vertex_position_buffer_offset + vertex_position_buffer_size;
            RHIDeviceSize vertex_varying_buffer_offset =
                vertex_varying_enable_blending_buffer_offset + vertex_varying_enable_blending_buffer_size;

            // temporary staging buffer
            RHIDeviceSize inefficient_staging_buffer_size =
                vertex_position_buffer_size + vertex_varying_enable_blending_buffer_size + vertex_varying_buffer_size;
            RHIBuffer* inefficient_staging_buffer = RHI_NULL_HANDLE;
            RHIDeviceMemory* inefficient_staging_buffer_memory = RHI_NULL_HANDLE;
            rhi->CreateBuffer(inefficient_staging_buffer_size,
                              RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              inefficient_staging_buffer,
                              inefficient_staging_buffer_memory);

            void* inefficient_staging_buffer_data;
            rhi->MapMemory(inefficient_staging_buffer_memory,
                           0,
                           RHI_WHOLE_SIZE,
                           0,
                           &inefficient_staging_buffer_data);

            MeshVertex::VulkanMeshVertexPostition* mesh_vertex_positions =
                reinterpret_cast<MeshVertex::VulkanMeshVertexPostition*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) + vertex_position_buffer_offset);
            MeshVertex::VulkanMeshVertexVaryingEnableBlending* mesh_vertex_blending_varyings =
                reinterpret_cast<MeshVertex::VulkanMeshVertexVaryingEnableBlending*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) +
                    vertex_varying_enable_blending_buffer_offset);
            MeshVertex::VulkanMeshVertexVarying* mesh_vertex_varyings =
                reinterpret_cast<MeshVertex::VulkanMeshVertexVarying*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) + vertex_varying_buffer_offset);

            for (uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
            {
                Vector3 normal = Vector3(vertex_buffer_data[vertex_index].nx,
                    vertex_buffer_data[vertex_index].ny,
                    vertex_buffer_data[vertex_index].nz);
                Vector3 tangent = Vector3(vertex_buffer_data[vertex_index].tx,
                    vertex_buffer_data[vertex_index].ty,
                    vertex_buffer_data[vertex_index].tz);

                mesh_vertex_positions[vertex_index].position = Vector3(vertex_buffer_data[vertex_index].x,
                    vertex_buffer_data[vertex_index].y,
                    vertex_buffer_data[vertex_index].z);

                mesh_vertex_blending_varyings[vertex_index].normal = normal;
                mesh_vertex_blending_varyings[vertex_index].tangent = tangent;

                mesh_vertex_varyings[vertex_index].texcoord =
                    Vector2(vertex_buffer_data[vertex_index].u, vertex_buffer_data[vertex_index].v);
            }

            rhi->UnmapMemory(inefficient_staging_buffer_memory);

            // use the vmaAllocator to allocate asset vertex buffer
            RHIBufferCreateInfo bufferInfo = { RHI_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.usage = RHI_BUFFER_USAGE_VERTEX_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            bufferInfo.size = vertex_position_buffer_size;
            rhi->CreateBufferVMA(vulkan_context->mAssetsAllocator,
                                 &bufferInfo,
                                 &allocInfo,
                                 now_mesh.mesh_vertex_position_buffer,
                                 &now_mesh.mesh_vertex_position_buffer_allocation,
                                 NULL);
            bufferInfo.size = vertex_varying_enable_blending_buffer_size;
            rhi->CreateBufferVMA(vulkan_context->mAssetsAllocator,
                                 &bufferInfo,
                                 &allocInfo,
                                 now_mesh.mesh_vertex_varying_enable_blending_buffer,
                                 &now_mesh.mesh_vertex_varying_enable_blending_buffer_allocation,
                                 NULL);
            bufferInfo.size = vertex_varying_buffer_size;
            rhi->CreateBufferVMA(vulkan_context->mAssetsAllocator,
                                 &bufferInfo,
                                 &allocInfo,
                                 now_mesh.mesh_vertex_varying_buffer,
                                 &now_mesh.mesh_vertex_varying_buffer_allocation,
                                 NULL);

            // use the data from staging buffer
            rhi->CopyBuffer(inefficient_staging_buffer,
                            now_mesh.mesh_vertex_position_buffer,
                            vertex_position_buffer_offset,
                            0,
                            vertex_position_buffer_size);
            rhi->CopyBuffer(inefficient_staging_buffer,
                            now_mesh.mesh_vertex_varying_enable_blending_buffer,
                            vertex_varying_enable_blending_buffer_offset,
                            0,
                            vertex_varying_enable_blending_buffer_size);
            rhi->CopyBuffer(inefficient_staging_buffer,
                            now_mesh.mesh_vertex_varying_buffer,
                            vertex_varying_buffer_offset,
                            0,
                            vertex_varying_buffer_size);

            // release staging buffer
            rhi->DestroyBuffer(inefficient_staging_buffer);
            rhi->FreeMemory(inefficient_staging_buffer_memory);

            // update descriptor set
            RHIDescriptorSetAllocateInfo mesh_vertex_blending_per_mesh_descriptor_set_alloc_info;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.sType =
                RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.pNext = NULL;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.descriptorPool = vulkan_context->mDescPool;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.descriptorSetCount = 1;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.pSetLayouts        = mMeshDescLayout;

            if (RHI_SUCCESS != rhi->AllocateDescriptorSets(
                &mesh_vertex_blending_per_mesh_descriptor_set_alloc_info,
                now_mesh.mesh_vertex_blending_descriptor_set))
            {
                throw std::runtime_error("allocate mesh vertex blending per mesh descriptor set");
            }

            RHIDescriptorBufferInfo mesh_vertex_Joint_binding_storage_buffer_info = {};
            mesh_vertex_Joint_binding_storage_buffer_info.offset = 0;
            mesh_vertex_Joint_binding_storage_buffer_info.range = 1;
            mesh_vertex_Joint_binding_storage_buffer_info.buffer =
                mGlobalRenderResource.mStorageBuffer.mGlobalNullDescStorageBuffer;
            assert(mesh_vertex_Joint_binding_storage_buffer_info.range <
                mGlobalRenderResource.mStorageBuffer.mMaxStorageBufferRange);

            RHIDescriptorSet* descriptor_set_to_write = now_mesh.mesh_vertex_blending_descriptor_set;

            RHIWriteDescriptorSet descriptor_writes[1];

            RHIWriteDescriptorSet& mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info =
                descriptor_writes[0];
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.sType =
                RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.pNext = NULL;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.dstSet = descriptor_set_to_write;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.dstBinding = 0;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.dstArrayElement = 0;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.descriptorType =
                RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.descriptorCount = 1;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.pBufferInfo =
                &mesh_vertex_Joint_binding_storage_buffer_info;

            rhi->UpdateDescriptorSets((sizeof(descriptor_writes) / sizeof(descriptor_writes[0])),
                                      descriptor_writes,
                                      0,
                                      NULL);
        }
    }

    void RenderResource::updateIndexBuffer(std::shared_ptr<RHI> rhi,
                                           uint32_t             index_buffer_size,
                                           void*                index_buffer_data,
                                           VulkanMesh&          now_mesh)
    {
        VulkanRHI* vulkan_context = static_cast<VulkanRHI*>(rhi.get());

        // temp staging buffer
        RHIDeviceSize buffer_size = index_buffer_size;

        RHIBuffer* inefficient_staging_buffer;
        RHIDeviceMemory* inefficient_staging_buffer_memory;
        rhi->CreateBuffer(buffer_size,
                          RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          inefficient_staging_buffer,
                          inefficient_staging_buffer_memory);

        void* staging_buffer_data;
        rhi->MapMemory(inefficient_staging_buffer_memory, 0, buffer_size, 0, &staging_buffer_data);
        memcpy(staging_buffer_data, index_buffer_data, (size_t)buffer_size);
        rhi->UnmapMemory(inefficient_staging_buffer_memory);

        // use the vmaAllocator to allocate asset index buffer
        RHIBufferCreateInfo bufferInfo = { RHI_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = buffer_size;
        bufferInfo.usage = RHI_BUFFER_USAGE_INDEX_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        rhi->CreateBufferVMA(vulkan_context->mAssetsAllocator,
                             &bufferInfo,
                             &allocInfo,
                             now_mesh.mesh_index_buffer,
                             &now_mesh.mesh_index_buffer_allocation,
                             NULL);

        // use the data from staging buffer
        rhi->CopyBuffer( inefficient_staging_buffer, now_mesh.mesh_index_buffer, 0, 0, buffer_size);

        // release temp staging buffer
        rhi->DestroyBuffer(inefficient_staging_buffer);
        rhi->FreeMemory(inefficient_staging_buffer_memory);
    }

    void RenderResource::updateTextureImageData(std::shared_ptr<RHI> rhi, const TextureDataToUpdate& texture_data)
    {
        rhi->CreateGlobalImage(
            texture_data.now_material->base_color_texture_image,
            texture_data.now_material->base_color_image_view,
            texture_data.now_material->base_color_image_allocation,
            texture_data.base_color_image_width,
            texture_data.base_color_image_height,
            texture_data.base_color_image_pixels,
            texture_data.base_color_image_format);

        rhi->CreateGlobalImage(
            texture_data.now_material->metallic_roughness_texture_image,
            texture_data.now_material->metallic_roughness_image_view,
            texture_data.now_material->metallic_roughness_image_allocation,
            texture_data.metallic_roughness_image_width,
            texture_data.metallic_roughness_image_height,
            texture_data.metallic_roughness_image_pixels,
            texture_data.metallic_roughness_image_format);

        rhi->CreateGlobalImage(
            texture_data.now_material->normal_texture_image,
            texture_data.now_material->normal_image_view,
            texture_data.now_material->normal_image_allocation,
            texture_data.normal_roughness_image_width,
            texture_data.normal_roughness_image_height,
            texture_data.normal_roughness_image_pixels,
            texture_data.normal_roughness_image_format);

        rhi->CreateGlobalImage(
            texture_data.now_material->occlusion_texture_image,
            texture_data.now_material->occlusion_image_view,
            texture_data.now_material->occlusion_image_allocation,
            texture_data.occlusion_image_width,
            texture_data.occlusion_image_height,
            texture_data.occlusion_image_pixels,
            texture_data.occlusion_image_format);

        rhi->CreateGlobalImage(
            texture_data.now_material->emissive_texture_image,
            texture_data.now_material->emissive_image_view,
            texture_data.now_material->emissive_image_allocation,
            texture_data.emissive_image_width,
            texture_data.emissive_image_height,
            texture_data.emissive_image_pixels,
            texture_data.emissive_image_format);
    }

    VulkanMesh& RenderResource::GetEntityMesh(RenderEntity entity)
    {
        size_t assetid = entity.mMeshAssetID;

        auto it = mVulkanMesh.find(assetid);
        if (it != mVulkanMesh.end())
        {
            return it->second;
        }
        else
        {
            throw std::runtime_error("failed to get entity mesh");
        }
    }

    VulkanPBRMaterial& RenderResource::GetEntityMaterial(RenderEntity entity)
    {
        size_t assetid = entity.mMaterialAssetID;

        auto it = mVulkanPBRMaterial.find(assetid);
        if (it != mVulkanPBRMaterial.end())
        {
            return it->second;
        }
        else
        {
            throw std::runtime_error("failed to get entity material");
        }
    }

    void RenderResource::ResetRingBufferOffset(uint8_t current_frame_index)
    {
        mGlobalRenderResource.mStorageBuffer.mGlobalUploadRingbuffersEnd[current_frame_index] =
            mGlobalRenderResource.mStorageBuffer.mGlobalUploadRingbuffersBegin[current_frame_index];
    }

    void RenderResource::createAndMapStorageBuffer(std::shared_ptr<RHI> rhi)
    {
        VulkanRHI* raw_rhi = static_cast<VulkanRHI*>(rhi.get());
        StorageBuffer& _storage_buffer = mGlobalRenderResource.mStorageBuffer;
        uint32_t       frames_in_flight = raw_rhi->mkMaxFramesInFlight;
        RHIPhysicalDeviceProperties properties;
        rhi->GetPhysicalDeviceProperties(&properties);

        _storage_buffer.mMinUniformBufferOffsetAlignment =
            static_cast<uint32_t>(properties.limits.minUniformBufferOffsetAlignment);
        _storage_buffer.mMinStorageBufferOffsetAlignment =
            static_cast<uint32_t>(properties.limits.minStorageBufferOffsetAlignment);
        _storage_buffer.mMaxStorageBufferRange = properties.limits.maxStorageBufferRange;
        _storage_buffer.mNonCoherentAtomSize = properties.limits.nonCoherentAtomSize;

        // In Vulkan, the storage buffer should be pre-allocated.
        // The size is 128MB in NVIDIA D3D11
        // driver(https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0).
        uint32_t global_storage_buffer_size = 1024 * 1024 * 128;
        rhi->CreateBuffer(global_storage_buffer_size,
                          RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          _storage_buffer.mGlobalUploadRingbuffer,
                          _storage_buffer.mGlobalUploadRingbufferMemory);

        _storage_buffer.mGlobalUploadRingbuffersBegin.resize(frames_in_flight);
        _storage_buffer.mGlobalUploadRingbuffersEnd.resize(frames_in_flight);
        _storage_buffer.mGlobalUploadRingbuffersSize.resize(frames_in_flight);
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            _storage_buffer.mGlobalUploadRingbuffersBegin[i] = (global_storage_buffer_size * i) / frames_in_flight;
            _storage_buffer.mGlobalUploadRingbuffersSize[i] =
                (global_storage_buffer_size * (i + 1)) / frames_in_flight -
                (global_storage_buffer_size * i) / frames_in_flight;
        }

        // axis
        rhi->CreateBuffer(sizeof(AxisStorageBufferObject),
                          RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          _storage_buffer.mAxisInefficientStorageBuffer,
                          _storage_buffer.mAxisInefficientStorageBufferMemory);

        // null descriptor
        rhi->CreateBuffer(64,
                          RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          0,
                          _storage_buffer.mGlobalNullDescStorageBuffer,
                          _storage_buffer.mGlobalNullDescStorageBufferMemory);

        // TODO: Unmap when program terminates
        rhi->MapMemory(_storage_buffer.mGlobalUploadRingbufferMemory,
                       0,
                       RHI_WHOLE_SIZE,
                       0,
                       &_storage_buffer.mGlobalUploadRingbufferMemoryPointer);

        rhi->MapMemory(_storage_buffer.mAxisInefficientStorageBufferMemory,
                       0,
                       RHI_WHOLE_SIZE,
                       0,
                       &_storage_buffer.mAxisInefficientStorageBufferMemoryPointer);

        static_assert(64 >= sizeof(MeshVertex::VulkanMeshVertexJointBinding), "");
    }
}