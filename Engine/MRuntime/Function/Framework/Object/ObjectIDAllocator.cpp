#include "ObjectIDAllocator.hpp"

#include "MRuntime/Core/Base/Marco.hpp"

namespace MiniEngine
{
    std::atomic<GObjectID> ObjectIDAllocator::mNextID {0};

    GObjectID ObjectIDAllocator::Allocate()
    {
        std::atomic<GObjectID> new_object_ret = mNextID.load();
        mNextID++;
        if (mNextID >= kInvalidGObjectID)
        {
            LOG_FATAL("gobject id overflow");
        }

        return new_object_ret;
    }

} // namespace MiniEngine
