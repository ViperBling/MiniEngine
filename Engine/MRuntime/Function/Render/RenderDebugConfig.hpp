namespace MiniEngine
{
    class RenderDebugConfig
    {
    public:
        struct Animation
        {
            bool mbShowSkeleton = false;
            bool mbShowBoneName = false;
        };
        struct Camera
        {
            bool mbShowRuntimeInfo = false;
        };
        struct GameObject
        {
            bool mbShowBoundingBox = false;
        };

        Animation animation;
        Camera camera;
        GameObject gameObject;
    };
}