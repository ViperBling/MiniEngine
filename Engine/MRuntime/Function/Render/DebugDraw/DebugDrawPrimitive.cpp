#include "DebugDrawPrimitive.hpp"

namespace MiniEngine
{
    bool DebugDrawPrimitive::IsTimeOut(float deltaTime)
    {
        if (mTimeType == DebugDrawTimeType::Infinity)
        {
            return false;
        }
        else if (mTimeType == DebugDrawTimeType::OneFrame)
        {
            if (!mbRendered)
            {
                mbRendered = true;
                return false;
            }
            else return true;
        }
        else
        {
            mLifeTime -= deltaTime;
            return (mLifeTime < 0.0f);
        }
        return false;
    }

    void DebugDrawPrimitive::SetTime(float inLifeTime)
    {
        if (fabs(inLifeTime - kDebugDrawInfinityLifeTime) < 1e-6)
        {
            mTimeType = DebugDrawTimeType::Infinity;
            mLifeTime = 0.0f;
        }
        else if (fabs(inLifeTime - kDebugDrawOneFrame) < 1e-6)
        {
            mTimeType = DebugDrawTimeType::OneFrame;
            mLifeTime = 0.03f;
        }
        else
        {
            mTimeType = DebugDrawTimeType::Common;
            mLifeTime = inLifeTime;
        }
    }
}