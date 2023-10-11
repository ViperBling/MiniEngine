#include "Scene.hpp"

#include "MRuntime/Core/Base/Marco.hpp"

#include "MRuntime/Resource/AssetManager/AssetManager.hpp"
#include "MRuntime/Resource/ResourceType/Common/Scene.hpp"

#include "MRuntime/MEngine.hpp"
#include "MRuntime/Function/Framework/Object/Object.hpp"

#include <limits>

namespace MiniEngine
{
    void Scene::Clear()
    {
        mGObjects.clear();
    }

    GObjectID Scene::CreateObject(const ObjectInstanceRes& object_instance_res)
    {
        GObjectID object_id = ObjectIDAllocator::Allocate();
        ASSERT(object_id != kInvalidGObjectID);

        std::shared_ptr<GObject> gobject;
        try
        {
            gobject = std::make_shared<GObject>(object_id);
        }
        catch (const std::bad_alloc&)
        {
            LOG_FATAL("cannot allocate memory for new gobject");
        }

        bool is_loaded = gobject->Load(object_instance_res);
        if (is_loaded)
        {
            mGObjects.emplace(object_id, gobject);
        }
        else
        {
            LOG_ERROR("loading object " + object_instance_res.mName + " failed");
            return kInvalidGObjectID;
        }
        return object_id;
    }

    bool Scene::Load(const std::string& scene_res_url)
    {
        LOG_INFO("loading scene: {}", scene_res_url);

        mSceneResURL = scene_res_url;

        SceneRes   scene_res;
        const bool is_load_success = gRuntimeGlobalContext.mAssetManager->LoadAsset(scene_res_url, scene_res);
        if (is_load_success == false)
        {
            return false;
        }

        for (const ObjectInstanceRes& object_instance_res : scene_res.mObjects)
        {
            CreateObject(object_instance_res);
        }

        // create active character
        for (const auto& object_pair : mGObjects)
        {
            std::shared_ptr<GObject> object = object_pair.second;
            if (object == nullptr)
                continue;

        }

        mbIsLoaded = true;

        LOG_INFO("scene load succeed");

        return true;
    }

    void Scene::Unload()
    {
        Clear();
        LOG_INFO("unload scene: {}", mSceneResURL);
    }

    bool Scene::Save()
    {
        LOG_INFO("saving scene: {}", mSceneResURL);
        SceneRes output_scene_res;

        const size_t                    object_cout    = mGObjects.size();
        std::vector<ObjectInstanceRes>& output_objects = output_scene_res.mObjects;
        output_objects.resize(object_cout);

        size_t object_index = 0;
        for (const auto& id_object_pair : mGObjects)
        {
            if (id_object_pair.second)
            {
                id_object_pair.second->Save(output_objects[object_index]);
                ++object_index;
            }
        }

        const bool is_save_success =
            gRuntimeGlobalContext.mAssetManager->SaveAsset(output_scene_res, mSceneResURL);

        if (is_save_success == false)
        {
            LOG_ERROR("failed to save {}", mSceneResURL);
        }
        else
        {
            LOG_INFO("scene save succeed");
        }

        return is_save_success;
    }

    void Scene::Tick(float delta_time)
    {
        if (!mbIsLoaded)
        {
            return;
        }

        for (const auto& id_object_pair : mGObjects)
        {
            assert(id_object_pair.second);
            if (id_object_pair.second)
            {
                id_object_pair.second->Tick(delta_time);
            }
        }

    }

    std::weak_ptr<GObject> Scene::GetGObjectByID(GObjectID go_id) const
    {
        auto iter = mGObjects.find(go_id);
        if (iter != mGObjects.end())
        {
            return iter->second;
        }

        return std::weak_ptr<GObject>();
    }

    void Scene::DeleteGObjectByID(GObjectID go_id)
    {
        auto iter = mGObjects.find(go_id);
        if (iter != mGObjects.end())
        {
            std::shared_ptr<GObject> object = iter->second;
        }

        mGObjects.erase(go_id);
    }

} // namespace MiniEngine
