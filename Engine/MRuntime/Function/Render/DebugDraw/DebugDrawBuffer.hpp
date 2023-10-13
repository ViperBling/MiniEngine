#pragma once

#include "DebugDrawPrimitive.hpp"
#include "DebugDrawFont.hpp"
#include "Function/Render/Interface/RHI.hpp"

#include <queue>
namespace MiniEngine
{
    class DebugDrawAllocator
    {
    public:
        DebugDrawAllocator() {};
        void Initialize(DebugDrawFont* font);
        void Destory();
        void Tick();
        void Clear();
        void ClearBuffer();
        
        size_t CacheVertexs(const std::vector<DebugDrawVertex>& vertexs);
        void CacheUniformObject(Matrix4x4 proj_view_matrix);
        size_t CacheUniformDynamicObject(const std::vector<std::pair<Matrix4x4,Vector4> >& model_colors);

        size_t GetVertexCacheOffset() const;
        size_t GetUniformDynamicCacheOffset() const;
        void Allocator();

        RHIBuffer* GetVertexBuffer();
        RHIDescriptorSet* &GetDescriptorSet();

        RHIBuffer* GetSphereVertexBuffer();
        RHIBuffer* GetCylinderVertexBuffer();
        RHIBuffer* GetCapsuleVertexBuffer();

        const size_t GetSphereVertexBufferSize() const;
        const size_t GetCylinderVertexBufferSize() const;
        const size_t GetCapsuleVertexBufferSize() const;
        const size_t GetCapsuleVertexBufferUpSize() const;
        const size_t GetCapsuleVertexBufferMidSize() const;
        const size_t GetCapsuleVertexBufferDownSize() const;
        const size_t GetSizeOfUniformBufferObject() const;

    private:
        void setupDescriptorSet();
        void prepareDescriptorSet();
        void updateDescriptorSet();
        void flushPendingDelete();
        void unloadMeshBuffer();
        void loadSphereMeshBuffer();
        void loadCylinderMeshBuffer();
        void loadCapsuleMeshBuffer();

    private:
        std::shared_ptr<RHI> mRHI;
        struct UniformBufferObject
        {
            Matrix4x4 mProjViewMatrix;
        };

        struct alignas(256) UniformBufferDynamicObject
        {
            Matrix4x4 mModelMatrix;
            Vector4 mColor;
        };

        struct Resource
        {
            RHIBuffer* mBuffer = nullptr;
            RHIDeviceMemory* mMemory = nullptr;
        };
        struct Descriptor
        {
            RHIDescriptorSetLayout* mDescLayout = nullptr;
            std::vector<RHIDescriptorSet*> mDescSets;
        };

        //descriptor
        Descriptor mDescriptor;

        //changeable resource
        Resource mVertexResource;
        std::vector<DebugDrawVertex> mVertexCache;

        Resource mUniformResource;
        UniformBufferObject mUniformBufferObject;

        Resource mUniformDynamicResource;
        std::vector<UniformBufferDynamicObject>mUBODynamicCache;

        //static mesh resource
        Resource mSphereResource;
        Resource mCylinderResource;
        Resource mCapsuleResource;

        //font resource
        DebugDrawFont* mFont = nullptr;

        //resource deleter
        static const uint32_t kDeferredDeleteResourceFrameCount = 5;//the count means after count-1 frame will be delete
        uint32_t mCurrentFrame = 0;
        std::queue<Resource> mDeferredDeleteQueue[kDeferredDeleteResourceFrameCount];

        const int mCircleSampleCount = 10;
    };
} // namespace MiniEngine
