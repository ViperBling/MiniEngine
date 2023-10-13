#include "DebugDrawContext.hpp"

namespace MiniEngine
{
    DebugDrawGroup *DebugDrawContext::TryGetOrCreateDebugDrawGroup(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        
        size_t debug_draw_group_count = mDebugDrawGroup.size();
        for (size_t debug_draw_group_index = 0; debug_draw_group_index < debug_draw_group_count; debug_draw_group_index++)
        {
            DebugDrawGroup* debug_draw_group = mDebugDrawGroup[debug_draw_group_index];
            if (debug_draw_group->GetName() == name)
            {
                return debug_draw_group;
            }
        }

        DebugDrawGroup* new_debug_draw_group = new DebugDrawGroup;
        new_debug_draw_group->Initialize();
        new_debug_draw_group->SetName(name);
        mDebugDrawGroup.push_back(new_debug_draw_group);

        return new_debug_draw_group;
    }

    void DebugDrawContext::Clear()
    {
        std::lock_guard<std::mutex> guard(mMutex);

        size_t debug_draw_group_count = mDebugDrawGroup.size();
        for (size_t debug_draw_group_index = 0; debug_draw_group_index < debug_draw_group_count; debug_draw_group_index++)
        {
            delete mDebugDrawGroup[debug_draw_group_index];
        }

        mDebugDrawGroup.clear();
    }

    void DebugDrawContext::Tick(float deltaTime)
    {
        removeDeadPrimitives(deltaTime);
    }

    void DebugDrawContext::removeDeadPrimitives(float deltaTime)
    {
        std::lock_guard<std::mutex> guard(mMutex);

        size_t debug_draw_group_count = mDebugDrawGroup.size();
        for (size_t debug_draw_group_index = 0; debug_draw_group_index < debug_draw_group_count; debug_draw_group_index++)
        {
            if (mDebugDrawGroup[debug_draw_group_index] == nullptr)continue;
            mDebugDrawGroup[debug_draw_group_index]->RemoveDeadPrimitives(deltaTime);
        }
    }

} // namespace MiniEngine
