#pragma once

#include <atomic>
#include <limits>

namespace MiniEngine
{
    using GObjectID = std::size_t;

    constexpr GObjectID kInvalidGObjectID = std::numeric_limits<std::size_t>::max();

    class ObjectIDAllocator
    {
    public:
        static GObjectID Allocate();

    private:
        static std::atomic<GObjectID> mNextID;
    };
} // namespace MiniEngine
