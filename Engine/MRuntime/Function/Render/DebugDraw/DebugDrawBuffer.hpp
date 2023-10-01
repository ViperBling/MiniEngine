#pragma once

#include "DebugDrawPrimitive.hpp"
#include "Function/Render/Interface/RHI.hpp"

namespace MiniEngine
{
    class DebugDrawAllocator
    {
    public:
        DebugDrawAllocator();

        void Allocator();
        void Clear();
        void ClearBuffer();

        size_t CacheVertexs(const std::vector<DebugDrawVertex>& vertices);

        size_t GetVertexCacheOffset() const { return mVertexCache.size(); }
        RHIBuffer* GetVertexBuffer() const { return mVertexResource.buffer; }

    private:
        std::shared_ptr<RHI> mRHI;

        struct Resource
        {
            RHIBuffer*       buffer = nullptr;
            RHIDeviceMemory* memory = nullptr;
        };
        // changeable resource
        Resource                     mVertexResource; // vk顶点资源指针
        std::vector<DebugDrawVertex> mVertexCache;    // 顶点数据cpu暂存
    };
    
} // namespace MiniEngine
