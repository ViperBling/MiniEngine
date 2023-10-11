#pragma once

#include "MRuntime/Function/Framework/Component/Component.hpp"
#include "MRuntime/Resource/ResourceType/Component/Mesh.hpp"
#include "MRuntime/Function/Render/RenderObject.hpp"

#include <vector>

namespace MiniEngine
{
    class RenderSwapContext;

    REFLECTION_TYPE(MeshComponent)
    CLASS(MeshComponent : public Component, WhiteListFields)
    {
        REFLECTION_BODY(MeshComponent)
    public:
        MeshComponent() {};

        void PostLoadResource(std::weak_ptr<GObject> parent_object) override;

        const std::vector<GameObjectPartDesc>& GetRawMeshes() const { return mRawMeshes; }

        void Tick(float delta_time) override;

    private:
        META(Enable)
        MeshComponentRes mMeshRes;
        std::vector<GameObjectPartDesc> mRawMeshes;
    };
} // namespace MiniEngine
