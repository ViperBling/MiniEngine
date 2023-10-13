#pragma once

#include "DebugDrawGroup.hpp"

namespace MiniEngine
{
    class DebugDrawContext
    {
    public:
        DebugDrawGroup* TryGetOrCreateDebugDrawGroup(const std::string& name);
        void Clear();
        void Tick(float delta_time);
    
    private:
        void removeDeadPrimitives(float deltaTime);
    
    public:
        std::vector<DebugDrawGroup*> mDebugDrawGroup;
    
    private:
        std::mutex mMutex;
    };
}