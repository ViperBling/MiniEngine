#include "DebugDrawBuffer.hpp"
#include "Function/Global/GlobalContext.hpp"

namespace MiniEngine
{
    DebugDrawAllocator::DebugDrawAllocator()
    {
        mRHI = gRuntimeGlobalContext.mRenderSystem->GetRHI();
    }

    void DebugDrawAllocator::Allocator()
    {
        uint64_t vertexBufferSize = static_cast<uint64_t>(mVertexCache.size() * sizeof(DebugDrawVertex));
        static bool created = false;
        if (vertexBufferSize > 0 && !created)
        {
            created = true;
            mRHI->CreateBuffer(
                vertexBufferSize, 
                RHI_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                mVertexResource.buffer, mVertexResource.memory);
            void * data;
            mRHI->MapMemory(mVertexResource.memory, 0, vertexBufferSize, 0, &data);
            memcpy(data, mVertexCache.data(), vertexBufferSize);
            mRHI->UnmapMemory(mVertexResource.memory);
        }
    }

    void DebugDrawAllocator::Clear()
    {
        mVertexCache.clear();
    }

    void DebugDrawAllocator::ClearBuffer()
    {
        if (mVertexResource.buffer)
        {
            mVertexResource.buffer = nullptr;
            mVertexResource.memory = nullptr;
        }
    }

    size_t DebugDrawAllocator::CacheVertexs(const std::vector<DebugDrawVertex> &vertices)
    {
        size_t offset = mVertexCache.size();
        mVertexCache.resize(offset + vertices.size());
        size_t vSize = vertices.size();
        for (size_t i = 0; i < vSize; i++)
        {
            mVertexCache[i + offset] = vertices[i];
        }
        return offset;
    }

} // namespace MiniEngine
