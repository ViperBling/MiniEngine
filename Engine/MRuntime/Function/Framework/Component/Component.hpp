#pragma once
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"

namespace MiniEngine
{
    class GObject;
    // Component
    REFLECTION_TYPE(Component)
    CLASS(Component, WhiteListFields)
    {
        REFLECTION_BODY(Component)

    public:
        Component() = default;
        virtual ~Component() {}

        // Instantiating the component after definition loaded
        virtual void PostLoadResource(std::weak_ptr<GObject> parent_object) { mParentObject = parent_object; }
        virtual void Tick(float delta_time) {};
        bool IsDirty() const { return mbIsDirty; }
        void SetDirtyFlag(bool is_dirty) { mbIsDirty = is_dirty; }

    public:
        bool mbTickInEditorMode {false};

    protected:
        std::weak_ptr<GObject> mParentObject;
        bool                   mbIsDirty {false};
        bool                   mbIsScaleDirty {false};
    };

} // namespace MiniEngine
