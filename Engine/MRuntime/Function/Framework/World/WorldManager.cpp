#include "WorldManager.hpp"

#include "MRuntime/Core/Base/Marco.hpp"

#include "MRuntime/resource/AssetManager/AssetManager.hpp"
#include "MRuntime/resource/ConfigManager/ConfigManager.hpp"

#include "MRuntime/Function/Framework/Scene/Scene.hpp"
#include "MRuntime/Function/Global/GlobalContext.hpp"

namespace MiniEngine
{
    WorldManager::~WorldManager() { Clear(); }

    void WorldManager::Initialize()
    {
        mbIsWorldLoaded   = false;
        mCurrentWorldURL = gRuntimeGlobalContext.mConfigManager->GetDefaultWorldURL();
    }

    void WorldManager::Clear()
    {
        // unload all loaded scenes
        for (auto scene_pair : mLoadedScenes)
        {
            scene_pair.second->Unload();
        }
        mLoadedScenes.clear();

        mCurrentActiveScene.reset();

        // clear world
        mCurrentWorldResource.reset();
        mCurrentWorldURL.clear();
        mbIsWorldLoaded = false;
    }

    void WorldManager::Tick(float delta_time)
    {
        if (!mbIsWorldLoaded)
        {
            LoadWorld(mCurrentWorldURL);
        }

        // tick the active scene
        std::shared_ptr<Scene> active_scene = mCurrentActiveScene.lock();
        if (active_scene)
        {
            active_scene->Tick(delta_time);
        }
    }

    bool WorldManager::LoadWorld(const std::string& world_url)
    {
        LOG_INFO("loading world: {}", world_url);
        WorldRes   world_res;
        const bool is_world_load_success = gRuntimeGlobalContext.mAssetManager->LoadAsset(world_url, world_res);
        if (!is_world_load_success)
        {
            return false;
        }

        mCurrentWorldResource = std::make_shared<WorldRes>(world_res);

        const bool is_scene_load_success = LoadScene(world_res.mDefaultSceneURL);
        if (!is_scene_load_success)
        {
            return false;
        }

        // set the default scene to be active scene
        auto iter = mLoadedScenes.find(world_res.mDefaultSceneURL);
        ASSERT(iter != mLoadedScenes.end());

        mCurrentActiveScene = iter->second;

        mbIsWorldLoaded = true;

        LOG_INFO("world load succeed!");
        return true;
    }

    bool WorldManager::LoadScene(const std::string& scene_url)
    {
        std::shared_ptr<Scene> scene = std::make_shared<Scene>();
        // set current scene temporary
        mCurrentActiveScene       = scene;

        const bool is_scene_load_success = scene->Load(scene_url);
        if (is_scene_load_success == false)
        {
            return false;
        }

        mLoadedScenes.emplace(scene_url, scene);

        return true;
    }

    void WorldManager::ReloadCurrentScene()
    {
        auto active_scene = mCurrentActiveScene.lock();
        if (active_scene == nullptr)
        {
            LOG_WARN("current scene is nil");
            return;
        }

        const std::string scene_url = active_scene->GetSceneResUrl();
        active_scene->Unload();
        mLoadedScenes.erase(scene_url);

        const bool is_load_success = LoadScene(scene_url);
        if (!is_load_success)
        {
            LOG_ERROR("load scene failed {}", scene_url);
            return;
        }

        // update the active scene instance
        auto iter = mLoadedScenes.find(scene_url);
        ASSERT(iter != mLoadedScenes.end());

        mCurrentActiveScene = iter->second;

        LOG_INFO("reload current evel succeed");
    }

    void WorldManager::SaveCurrentScene()
    {
        auto active_scene = mCurrentActiveScene.lock();

        if (active_scene == nullptr)
        {
            LOG_ERROR("save scene failed, no active scene");
            return;
        }

        active_scene->Save();
    }
} // namespace MiniEngine
