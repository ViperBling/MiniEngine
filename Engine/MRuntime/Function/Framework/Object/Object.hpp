#pragma once

#include "MRuntime/Function/Framework/Component/Component.hpp"
#include "ObjectIDAllocator.hpp"

#include "MRuntime/Resource/ResourceType/Common/Object.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace MiniEngine
{
    /// GObject : Game Object base class
    class GObject : public std::enable_shared_from_this<GObject>
    {
        typedef std::unordered_set<std::string> TypeNameSet;

    public:
        GObject(GObjectID id) : mID {id} {}
        virtual ~GObject();

        virtual void Tick(float delta_time);

        bool Load(const ObjectInstanceRes& object_instance_res);
        void Save(ObjectInstanceRes& out_object_instance_res);

        GObjectID GetID() const { return mID; }

        void               SetName(std::string name) { mName = name; }
        const std::string& GetName() const { return mName; }

        bool HasComponent(const std::string& compenent_type_name) const;

        std::vector<Reflection::ReflectionPtr<Component>> GetComponents() { return mComponents; }

        template<typename TComponent>
        TComponent* TryGetComponent(const std::string& compenent_type_name)
        {
            for (auto& component : mComponents)
            {
                if (component.GetTypeName() == compenent_type_name)
                {
                    return static_cast<TComponent*>(component.operator->());
                }
            }

            return nullptr;
        }

        template<typename TComponent>
        const TComponent* TryGetComponentConst(const std::string& compenent_type_name) const
        {
            for (const auto& component : mComponents)
            {
                if (component.GetTypeName() == compenent_type_name)
                {
                    return static_cast<const TComponent*>(component.operator->());
                }
            }
            return nullptr;
        }

#define TryGetComponent(COMPONENT_TYPE) TryGetComponent<COMPONENT_TYPE>(#COMPONENT_TYPE)
#define TryGetComponentConst(COMPONENT_TYPE) TryGetComponentConst<const COMPONENT_TYPE>(#COMPONENT_TYPE)

    protected:
        GObjectID   mID {kInvalidGObjectID};
        std::string mName;
        std::string mDefinitionURL;

        // we have to use the ReflectionPtr due to that the components need to be reflected 
        // in editor, and it's polymorphism
        std::vector<Reflection::ReflectionPtr<Component>> mComponents;
    };
} // namespace MiniEngine
