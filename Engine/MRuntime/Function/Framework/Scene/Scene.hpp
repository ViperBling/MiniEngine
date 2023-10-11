#pragma once

#include "MRuntime/Function/Framework/Object/ObjectIDAllocator.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace MiniEngine
{
    class GObject;
    class ObjectInstanceRes;

    using SceneObjectsMap = std::unordered_map<GObjectID, std::shared_ptr<GObject>>;

    /// The main class to manage all game objects
    class Scene
    {
    public:
        virtual ~Scene(){};

        bool Load(const std::string& scene_res_url);
        void Unload();

        bool Save();

        void Tick(float delta_time);

        const std::string& GetSceneResUrl() const { return mSceneResURL; }

        const SceneObjectsMap& GetAllGObjects() const { return mGObjects; }

        std::weak_ptr<GObject> GetGObjectByID(GObjectID go_id) const;

        GObjectID CreateObject(const ObjectInstanceRes& object_instance_res);
        void      DeleteGObjectByID(GObjectID go_id);

    protected:
        void Clear();

        bool        mbIsLoaded {false};
        std::string mSceneResURL;

        // all game objects in this scene, key: object id, value: object instance
        SceneObjectsMap mGObjects;
    };
} // namespace MiniEngine
