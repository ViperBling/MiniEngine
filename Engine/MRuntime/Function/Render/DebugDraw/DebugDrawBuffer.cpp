#include "DebugDrawBuffer.hpp"
#include "Function/Global/GlobalContext.hpp"

namespace MiniEngine
{
    void DebugDrawAllocator::Initialize(DebugDrawFont* font)
    { 
        mRHI = gRuntimeGlobalContext.mRenderSystem->GetRHI();
        mFont = font;
        setupDescriptorSet(); 
    }
    void DebugDrawAllocator::Destory()
    {
        Clear();
        unloadMeshBuffer();
    }

    void DebugDrawAllocator::Tick()
    {
        flushPendingDelete();
        mCurrentFrame = (mCurrentFrame + 1) % kDeferredDeleteResourceFrameCount;
    }

    RHIBuffer* DebugDrawAllocator::GetVertexBuffer(){return mVertexResource.mBuffer;}
    RHIDescriptorSet* &DebugDrawAllocator::GetDescriptorSet() { return mDescriptor.mDescSets[mRHI->GetCurrentFrameIndex()]; }

    size_t DebugDrawAllocator::CacheVertexs(const std::vector<DebugDrawVertex>& vertexs)
    {
        size_t offset = mVertexCache.size();
        mVertexCache.resize(offset + vertexs.size());
        for (size_t i = 0; i < vertexs.size(); i++)
        {
            mVertexCache[i + offset] = vertexs[i];
        }
        return offset;
    }
    void DebugDrawAllocator::CacheUniformObject(Matrix4x4 proj_view_matrix)
    {
        mUniformBufferObject.mProjViewMatrix = proj_view_matrix;
    }
    size_t DebugDrawAllocator::CacheUniformDynamicObject(const std::vector<std::pair<Matrix4x4, Vector4> >& model_colors)
    {
        size_t offset = mUBODynamicCache.size();
        mUBODynamicCache.resize(offset + model_colors.size());
        for (size_t i = 0; i < model_colors.size(); i++)
        {
            mUBODynamicCache[i + offset].mModelMatrix = model_colors[i].first;
            mUBODynamicCache[i + offset].mColor = model_colors[i].second;
        }
        return offset;
    }

    size_t DebugDrawAllocator::GetVertexCacheOffset() const
    {
        return mVertexCache.size();
    }
    size_t DebugDrawAllocator::GetUniformDynamicCacheOffset() const
    {
        return mUBODynamicCache.size();
    }

    void DebugDrawAllocator::Allocator()
    {
        ClearBuffer();
        uint64_t vertex_bufferSize = static_cast<uint64_t>(mVertexCache.size() * sizeof(DebugDrawVertex));
        if (vertex_bufferSize > 0)
        {
            mRHI->CreateBuffer(
                vertex_bufferSize,
                RHI_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                mVertexResource.mBuffer,
                mVertexResource.mMemory);

            void* data;
            mRHI->MapMemory(mVertexResource.mMemory, 0, vertex_bufferSize, 0, &data);
            memcpy(data, mVertexCache.data(), vertex_bufferSize);
            mRHI->UnmapMemory(mVertexResource.mMemory);
        }

        uint64_t uniform_BufferSize = static_cast<uint64_t>(sizeof(UniformBufferObject));
        if (uniform_BufferSize > 0)
        {
            mRHI->CreateBuffer(
                uniform_BufferSize,
                RHI_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                mUniformResource.mBuffer,
                mUniformResource.mMemory);
            
            void* data;
            mRHI->MapMemory(mUniformResource.mMemory, 0, uniform_BufferSize, 0, &data);
            memcpy(data, &mUniformBufferObject.mProjViewMatrix, uniform_BufferSize);
            mRHI->UnmapMemory(mUniformResource.mMemory);
        }

        uint64_t uniform_dynamic_BufferSize = static_cast<uint64_t>(sizeof(UniformBufferDynamicObject) * mUBODynamicCache.size());
        if (uniform_dynamic_BufferSize > 0)
        {
            mRHI->CreateBuffer(
                uniform_dynamic_BufferSize,
                RHI_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                mUniformDynamicResource.mBuffer,
                mUniformDynamicResource.mMemory);
            
            void* data;
            mRHI->MapMemory(mUniformDynamicResource.mMemory, 0, uniform_dynamic_BufferSize, 0, &data);
            memcpy(data, mUBODynamicCache.data(), uniform_dynamic_BufferSize);
            mRHI->UnmapMemory(mUniformDynamicResource.mMemory);
        }
        
        updateDescriptorSet();
    }

    void DebugDrawAllocator::Clear()
    {
        ClearBuffer();
        mVertexCache.clear();
        mUniformBufferObject.mProjViewMatrix = Matrix4x4::IDENTITY;
        mUBODynamicCache.clear();
    }

    void DebugDrawAllocator::ClearBuffer()
    {
        if (mVertexResource.mBuffer)
        {
            mDeferredDeleteQueue[mCurrentFrame].push(mVertexResource);
            mVertexResource.mBuffer = nullptr;
            mVertexResource.mMemory = nullptr;
        }
        if (mUniformResource.mBuffer)
        {
            mDeferredDeleteQueue[mCurrentFrame].push(mUniformResource);
            mUniformResource.mBuffer = nullptr;
            mUniformResource.mMemory = nullptr;
        }
        if (mUniformDynamicResource.mBuffer)
        {
            mDeferredDeleteQueue[mCurrentFrame].push(mUniformDynamicResource);
            mUniformDynamicResource.mBuffer = nullptr;
            mUniformDynamicResource.mMemory = nullptr;
        }
    }

    void DebugDrawAllocator::flushPendingDelete()
    {
        uint32_t current_frame_to_delete = (mCurrentFrame + 1) % kDeferredDeleteResourceFrameCount;
        while (!mDeferredDeleteQueue[current_frame_to_delete].empty())
        {
            Resource resource_to_delete = mDeferredDeleteQueue[current_frame_to_delete].front();
            mDeferredDeleteQueue[current_frame_to_delete].pop();
            if (resource_to_delete.mBuffer == nullptr)continue;
            mRHI->FreeMemory(resource_to_delete.mMemory);
            mRHI->DestroyBuffer(resource_to_delete.mBuffer);
        }
    }

    void DebugDrawAllocator::setupDescriptorSet()
    {
        RHIDescriptorSetLayoutBinding uboLayoutBinding[3];
        uboLayoutBinding[0].binding = 0;
        uboLayoutBinding[0].descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding[0].descriptorCount = 1;
        uboLayoutBinding[0].stageFlags = RHI_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding[0].pImmutableSamplers = nullptr;

        uboLayoutBinding[1].binding = 1;
        uboLayoutBinding[1].descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        uboLayoutBinding[1].descriptorCount = 1;
        uboLayoutBinding[1].stageFlags = RHI_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding[1].pImmutableSamplers = nullptr;

        uboLayoutBinding[2].binding = 2;
        uboLayoutBinding[2].descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        uboLayoutBinding[2].descriptorCount = 1;
        uboLayoutBinding[2].stageFlags = RHI_SHADER_STAGE_FRAGMENT_BIT;
        uboLayoutBinding[2].pImmutableSamplers = nullptr;

        RHIDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 3;
        layoutInfo.pBindings = uboLayoutBinding;

        if (mRHI->CreateDescriptorSetLayout(&layoutInfo, mDescriptor.mDescLayout) != RHI_SUCCESS)
        {
            throw std::runtime_error("create debug draw layout");
        }

        mDescriptor.mDescSets.resize(mRHI->GetMaxFramesInFlight());
        for (size_t i = 0; i < mRHI->GetMaxFramesInFlight(); i++)
        {
            RHIDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.pNext = NULL;
            allocInfo.descriptorPool = mRHI->GetDescriptorPool();
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &mDescriptor.mDescLayout;

            if (RHI_SUCCESS != mRHI->AllocateDescriptorSets(&allocInfo, mDescriptor.mDescSets[i]))
            {
                throw std::runtime_error("debug draw descriptor set");
            }
        }

        prepareDescriptorSet();
    }

    //prepare at the start tick
    void DebugDrawAllocator::prepareDescriptorSet()
    {
        RHIDescriptorImageInfo image_info[1];
        image_info[0].imageView = mFont->GetImageView();
        image_info[0].sampler = mRHI->GetOrCreateDefaultSampler(Default_Sampler_Linear);
        image_info[0].imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        for (size_t i = 0; i < mRHI->GetMaxFramesInFlight(); i++)
        {
            RHIWriteDescriptorSet descriptor_write[1];
            descriptor_write[0].sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write[0].dstSet = mDescriptor.mDescSets[i];
            descriptor_write[0].dstBinding = 2;
            descriptor_write[0].dstArrayElement = 0;
            descriptor_write[0].pNext = nullptr;
            descriptor_write[0].descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_write[0].descriptorCount = 1;
            descriptor_write[0].pBufferInfo = nullptr;
            descriptor_write[0].pImageInfo = &image_info[0];
            descriptor_write[0].pTexelBufferView = nullptr;

            mRHI->UpdateDescriptorSets(1, descriptor_write, 0, nullptr);
        }
    }

    //update every tick
    void DebugDrawAllocator::updateDescriptorSet()
    {
        RHIDescriptorBufferInfo buffer_info[2];
        buffer_info[0].buffer = mUniformResource.mBuffer;
        buffer_info[0].offset = 0;
        buffer_info[0].range = sizeof(UniformBufferObject);

        buffer_info[1].buffer = mUniformDynamicResource.mBuffer;
        buffer_info[1].offset = 0;
        buffer_info[1].range = sizeof(UniformBufferDynamicObject);
        
        RHIWriteDescriptorSet descriptor_write[2];
        descriptor_write[0].sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write[0].dstSet = mDescriptor.mDescSets[mRHI->GetCurrentFrameIndex()];
        descriptor_write[0].dstBinding = 0;
        descriptor_write[0].dstArrayElement = 0;
        descriptor_write[0].pNext = nullptr;
        descriptor_write[0].descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write[0].descriptorCount = 1;
        descriptor_write[0].pBufferInfo = &buffer_info[0];
        descriptor_write[0].pImageInfo = nullptr;
        descriptor_write[0].pTexelBufferView = nullptr;

        descriptor_write[1].sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write[1].dstSet = mDescriptor.mDescSets[mRHI->GetCurrentFrameIndex()];
        descriptor_write[1].dstBinding = 1;
        descriptor_write[1].dstArrayElement = 0;
        descriptor_write[1].pNext = nullptr;
        descriptor_write[1].descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_write[1].descriptorCount = 1;
        descriptor_write[1].pBufferInfo = &buffer_info[1];
        descriptor_write[1].pImageInfo = nullptr;
        descriptor_write[1].pTexelBufferView = nullptr;

        mRHI->UpdateDescriptorSets(2, descriptor_write, 0, nullptr);
    }

    void DebugDrawAllocator::unloadMeshBuffer()
    {
        mDeferredDeleteQueue[mCurrentFrame].push(mSphereResource);
        mSphereResource.mBuffer = nullptr;
        mSphereResource.mMemory = nullptr;
        mDeferredDeleteQueue[mCurrentFrame].push(mCylinderResource);
        mCylinderResource.mBuffer = nullptr;
        mCylinderResource.mMemory = nullptr;
        mDeferredDeleteQueue[mCurrentFrame].push(mCapsuleResource);
        mCapsuleResource.mBuffer = nullptr;
        mCapsuleResource.mMemory = nullptr;
    }
    
    void DebugDrawAllocator::loadSphereMeshBuffer()
    {
        int32_t param = mCircleSampleCount;
        //radios is 1
        float _2pi = 2.0f * MATH_PI;
        std::vector<DebugDrawVertex> vertexs((param * 2 + 2) * (param * 2) * 2 + (param * 2 + 1) * (param * 2) * 2);

        int32_t current_index = 0;
        for (int32_t i = -param - 1; i < param + 1; i++)
        {
            float h = Math::Sin(_2pi / 4.0f * i / (param + 1.0f));
            float h1 = Math::Sin(_2pi / 4.0f * (i + 1) / (param + 1.0f));
            float r = Math::Sqrt(1.0f - h * h);
            float r1 = Math::Sqrt(1.0f - h1 * h1);
            for (int32_t j = 0; j < 2 * param; j++)
            {
                Vector3 p(Math::Cos(_2pi / (2.0f * param) * j) * r, Math::Sin(_2pi / (2.0f * param) * j) * r, h);
                Vector3 p1(Math::Cos(_2pi / (2.0f * param) * j) * r1, Math::Sin(_2pi / (2.0f * param) * j) * r1, h1);
                vertexs[current_index].mPos = p;
                vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);

                vertexs[current_index].mPos = p1;
                vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            }
            if (i != -param - 1)
            {
                for (int32_t j = 0; j < 2 * param; j++)
                {
                    Vector3 p(Math::Cos(_2pi / (2.0f * param) * j) * r, Math::Sin(_2pi / (2.0f * param) * j) * r, h);
                    Vector3 p1(Math::Cos(_2pi / (2.0f * param) * (j + 1)) * r, Math::Sin(_2pi / (2.0f * param) * (j + 1)) * r, h);
                    vertexs[current_index].mPos = p;
                    vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);

                    vertexs[current_index].mPos = p1;
                    vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
                }
            }
        }

        uint64_t bufferSize = static_cast<uint64_t>(vertexs.size() * sizeof(DebugDrawVertex));

        mRHI->CreateBuffer(
            bufferSize,
            RHI_BUFFER_USAGE_VERTEX_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            mSphereResource.mBuffer,
            mSphereResource.mMemory);

        Resource stagingBuffer;
        mRHI->CreateBuffer(
            bufferSize,
            RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
            RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer.mBuffer,
            stagingBuffer.mMemory);
        void* data;
        mRHI->MapMemory(stagingBuffer.mMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertexs.data(), bufferSize);
        mRHI->UnmapMemory(stagingBuffer.mMemory);

        mRHI->CopyBuffer(stagingBuffer.mBuffer, mSphereResource.mBuffer, 0, 0, bufferSize);

        mRHI->DestroyBuffer(stagingBuffer.mBuffer);
        mRHI->FreeMemory(stagingBuffer.mMemory);
    }

    void DebugDrawAllocator::loadCylinderMeshBuffer()
    {
        int param = mCircleSampleCount;
        //radios is 1 , height is 2
        float _2pi = 2.0f * MATH_PI;
        std::vector<DebugDrawVertex> vertexs(2 * param * 5 * 2);

        size_t current_index = 0;
        for (int32_t i = 0; i < 2 * param; i++)
        {
            Vector3 p(Math::Cos(_2pi / (2.0f * param) * i), Math::Sin(_2pi / (2.0f * param) * i), 1.0f);
            Vector3 p_(Math::Cos(_2pi / (2.0f * param) * (i + 1)), Math::Sin(_2pi / (2.0f * param) * (i + 1)), 1.0f);
            Vector3 p1(Math::Cos(_2pi / (2.0f * param) * i), Math::Sin(_2pi / (2.0f * param) * i), -1.0f);
            Vector3 p1_(Math::Cos(_2pi / (2.0f * param) * (i + 1)), Math::Sin(_2pi / (2.0f * param) * (i + 1)), -1.0f);

            vertexs[current_index].mPos = p;
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            vertexs[current_index].mPos = p_;
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);

            vertexs[current_index].mPos = p1;
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            vertexs[current_index].mPos = p1_;
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
        
            vertexs[current_index].mPos = p;
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            vertexs[current_index].mPos = p1;
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
        
            vertexs[current_index].mPos = p;
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            vertexs[current_index].mPos = Vector3(0.0f, 0.0f, 1.0f);
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);

            vertexs[current_index].mPos = p1;
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            vertexs[current_index].mPos = Vector3(0.0f, 0.0f, -1.0f);
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
        }

        uint64_t bufferSize = static_cast<uint64_t>(vertexs.size() * sizeof(DebugDrawVertex));

        mRHI->CreateBuffer(
            bufferSize,
            RHI_BUFFER_USAGE_VERTEX_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            mCylinderResource.mBuffer,
            mCylinderResource.mMemory);

        Resource stagingBuffer;
        mRHI->CreateBuffer(
            bufferSize,
            RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
            RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer.mBuffer,
            stagingBuffer.mMemory);
        void* data;
        mRHI->MapMemory(stagingBuffer.mMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertexs.data(), bufferSize);
        mRHI->UnmapMemory(stagingBuffer.mMemory);

        mRHI->CopyBuffer(stagingBuffer.mBuffer, mCylinderResource.mBuffer, 0, 0, bufferSize);

        mRHI->DestroyBuffer(stagingBuffer.mBuffer);
        mRHI->FreeMemory(stagingBuffer.mMemory);
    }

    void DebugDrawAllocator::loadCapsuleMeshBuffer()
    {
        int param = mCircleSampleCount;
        //radios is 1,height is 4
        float _2pi = 2.0f * MATH_PI;
        std::vector<DebugDrawVertex> vertexs(2 * param * param * 4 + 2 * param * param * 4 + 2 * param * 2);

        size_t current_index = 0;
        for (int32_t i = 0; i < param; i++)
        {
            float h = Math::Sin(_2pi / 4.0 / param * i);
            float h1 = Math::Sin(_2pi / 4.0 / param * (i + 1));
            float r = Math::Sqrt(1 - h * h);
            float r1 = Math::Sqrt(1 - h1 * h1);
            for (int32_t j = 0; j < 2 * param; j++)
            {
                Vector3 p(Math::Cos(_2pi / (2.0f * param) * j) * r, Math::Sin(_2pi / (2.0f * param) * j) * r, h + 1.0f);
                Vector3 p_(Math::Cos(_2pi / (2.0f * param) * (j + 1)) * r, Math::Sin(_2pi / (2.0f * param) * (j + 1)) * r, h + 1.0f);
                Vector3 p1(Math::Cos(_2pi / (2.0f * param) * j) * r1, Math::Sin(_2pi / (2.0f * param) * j) * r1, h1 + 1.0f);
                vertexs[current_index].mPos = p;
                vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
                vertexs[current_index].mPos = p1;
                vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
                
                vertexs[current_index].mPos = p;
                vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
                vertexs[current_index].mPos = p_;
                vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        for (int32_t j = 0; j < 2 * param; j++)
        {
            Vector3 p(Math::Cos(_2pi / (2.0f * param) * j), Math::Sin(_2pi / (2.0f * param) * j), 1.0f);
            Vector3 p1(Math::Cos(_2pi / (2.0f * param) * j), Math::Sin(_2pi / (2.0f * param) * j), -1.0f);
            vertexs[current_index].mPos = p;
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            vertexs[current_index].mPos = p1;
            vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
        }

        for (int32_t i = 0; i > -param ; i--)
        {
            float h = Math::Sin(_2pi / 4.0f / param * i);
            float h1 = Math::Sin(_2pi / 4.0f / param * (i - 1));
            float r = Math::Sqrt(1 - h * h);
            float r1 = Math::Sqrt(1 - h1 * h1);
            for (int32_t j = 0; j < (2 * param); j++)
            {
                Vector3 p(Math::Cos(_2pi / (2.0f * param) * j) * r, Math::Sin(_2pi / (2.0f * param) * j) * r, h - 1.0f);
                Vector3 p_(Math::Cos(_2pi / (2.0f * param) * (j + 1)) * r, Math::Sin(_2pi / (2.0f * param) * (j + 1)) * r, h - 1.0f);
                Vector3 p1(Math::Cos(_2pi / (2.0f * param) * j) * r1, Math::Sin(_2pi / (2.0f * param) * j) * r1, h1 - 1.0f);
                vertexs[current_index].mPos = p;
                vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
                vertexs[current_index].mPos = p1;
                vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);

                vertexs[current_index].mPos = p;
                vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
                vertexs[current_index].mPos = p_;
                vertexs[current_index++].mColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        uint64_t bufferSize = static_cast<uint64_t>(vertexs.size() * sizeof(DebugDrawVertex));

        mRHI->CreateBuffer(
            bufferSize,
            RHI_BUFFER_USAGE_VERTEX_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            mCapsuleResource.mBuffer,
            mCapsuleResource.mMemory);

        Resource stagingBuffer;
        mRHI->CreateBuffer(
            bufferSize,
            RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
            RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer.mBuffer,
            stagingBuffer.mMemory);
        void* data;
        mRHI->MapMemory(stagingBuffer.mMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertexs.data(), bufferSize);
        mRHI->UnmapMemory(stagingBuffer.mMemory);

        mRHI->CopyBuffer(stagingBuffer.mBuffer, mCapsuleResource.mBuffer, 0, 0, bufferSize);

        mRHI->DestroyBuffer(stagingBuffer.mBuffer);
        mRHI->FreeMemory(stagingBuffer.mMemory);
    }

    
    RHIBuffer* DebugDrawAllocator::GetSphereVertexBuffer()
    {
        if (mSphereResource.mBuffer == nullptr)
        {
            loadSphereMeshBuffer();
        }
        return mSphereResource.mBuffer;
    }
    RHIBuffer* DebugDrawAllocator::GetCylinderVertexBuffer()
    {
        if (mCylinderResource.mBuffer == nullptr)
        {
            loadCylinderMeshBuffer();
        }
        return mCylinderResource.mBuffer;
    }
    RHIBuffer* DebugDrawAllocator::GetCapsuleVertexBuffer()
    {
        if (mCapsuleResource.mBuffer == nullptr)
        {
            loadCapsuleMeshBuffer();
        }
        return mCapsuleResource.mBuffer;
    }

    const size_t DebugDrawAllocator::GetSphereVertexBufferSize() const
    {
        return ((mCircleSampleCount * 2 + 2) * (mCircleSampleCount * 2) * 2 + (mCircleSampleCount * 2 + 1) * (mCircleSampleCount * 2) * 2);
    }
    const size_t DebugDrawAllocator::GetCylinderVertexBufferSize() const
    {
        return (mCircleSampleCount * 2) * 5 * 2;
    }
    const size_t DebugDrawAllocator::GetCapsuleVertexBufferSize() const
    {
        return (mCircleSampleCount * 2) * mCircleSampleCount * 4 + (2 * mCircleSampleCount) * 2 + (2 * mCircleSampleCount) * mCircleSampleCount * 4;
    }
    const size_t DebugDrawAllocator::GetCapsuleVertexBufferUpSize() const
    {
        return (mCircleSampleCount * 2) * mCircleSampleCount * 4;
    }
    const size_t DebugDrawAllocator::GetCapsuleVertexBufferMidSize() const
    {
        return 2 * mCircleSampleCount * 2;
    }
    const size_t DebugDrawAllocator::GetCapsuleVertexBufferDownSize() const
    {
        return 2 * mCircleSampleCount * mCircleSampleCount * 4;
    }

    const size_t DebugDrawAllocator::GetSizeOfUniformBufferObject() const
    {
        return sizeof(UniformBufferDynamicObject);
    }
} // namespace MiniEngine
