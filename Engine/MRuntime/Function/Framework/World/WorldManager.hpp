#pragma once

#include "MRuntime/Resource/ResourceType/Common/World.hpp"

#include <filesystem>
#include <string>

namespace MiniEngine
{
    class Scene;

    /// Manage all game worlds, it should be support multiple worlds, including game world and editor world.
    /// Currently, the implement just supports one active world and one active scene
    class WorldManager
    {
    public:
        virtual ~WorldManager();

        void Initialize();
        void Clear();

        void ReloadCurrentScene();
        void SaveCurrentScene();

        void                 Tick(float delta_time);
        std::weak_ptr<Scene> GetCurrentActiveScene() const { return mCurrentActiveScene; }

    private:
        bool LoadWorld(const std::string& world_url);
        bool LoadScene(const std::string& scene_url);
    
    private:
        bool                      mbIsWorldLoaded {false};
        std::string               mCurrentWorldURL;
        std::shared_ptr<WorldRes> mCurrentWorldResource;

        // all loaded scenes, key: scene url, vaule: scene instance
        std::unordered_map<std::string, std::shared_ptr<Scene>> mLoadedScenes;
        // active scene, currently we just support one active scene
        std::weak_ptr<Scene> mCurrentActiveScene;
    };
} // namespace MiniEngine
