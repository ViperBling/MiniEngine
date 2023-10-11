#include "MRuntime/Function/Framework/Object/Object.hpp"
#include "MRuntime/MEngine.hpp"
#include "MRuntime/Core/Meta/Reflection/Reflection.hpp"
#include "MRuntime/Resource/AssetManager/AssetManager.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Framework/Component/Component.hpp"
#include "MRuntime/Function/Framework/Component/TransformComponent/TransformComponent.hpp"

#include <cassert>
#include <unordered_set>

#include "Generated/Serializer/all_serializer.h"

namespace MiniEngine
{
    bool shouldComponentTick(std::string component_type_name)
    {
        if (gbIsEditorMode)
        {
            return gEditorTickComponentTypes.find(component_type_name) != gEditorTickComponentTypes.end();
        }
        else
        {
            return true;
        }
    }

    GObject::~GObject()
    {
        for (auto &component : mComponents)
        {
            ME_REFLECTION_DELETE(component);
        }
        mComponents.clear();
    }

    void GObject::Tick(float delta_time)
    {
        for (auto &component : mComponents)
        {
            if (shouldComponentTick(component.GetTypeName()))
            {
                component->Tick(delta_time);
            }
        }
    }

    bool GObject::HasComponent(const std::string &compenent_type_name) const
    {
        for (const auto &component : mComponents)
        {
            if (component.GetTypeName() == compenent_type_name)
                return true;
        }

        return false;
    }

    bool GObject::Load(const ObjectInstanceRes &object_instance_res)
    {
        // clear old components
        mComponents.clear();

        SetName(object_instance_res.mName);

        // load object instanced components
        mComponents = object_instance_res.mInstancedComponents;
        for (auto component : mComponents)
        {
            if (component)
            {
                component->PostLoadResource(weak_from_this());
            }
        }

        // load object definition components
        mDefinitionURL = object_instance_res.mDefinition;

        ObjectDefinitionRes definition_res;

        const bool is_loaded_success = gRuntimeGlobalContext.mAssetManager->LoadAsset(mDefinitionURL, definition_res);
        if (!is_loaded_success)
            return false;

        for (auto loaded_component : definition_res.mComponents)
        {
            const std::string type_name = loaded_component.GetTypeName();
            // don't create component if it has been instanced
            if (HasComponent(type_name))
                continue;

            loaded_component->PostLoadResource(weak_from_this());

            mComponents.push_back(loaded_component);
        }

        return true;
    }

    void GObject::Save(ObjectInstanceRes &out_object_instance_res)
    {
        out_object_instance_res.mName = mName;
        out_object_instance_res.mDefinition = mDefinitionURL;

        out_object_instance_res.mInstancedComponents = mComponents;
    }

} // namespace MiniEngine