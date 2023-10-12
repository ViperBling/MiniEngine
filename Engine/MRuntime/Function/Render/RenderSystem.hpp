#pragma once

#include <memory>

#include "MRuntime/Function/Render/Interface/RHI.hpp"
#include "MRuntime/Function/Render/WindowSystem.hpp"
#include "MRuntime/Function/Render/RenderEntity.hpp"
#include "MRuntime/Function/Render/RenderScene.hpp"
#include "MRuntime/Function/Render/RenderPipelineBase.hpp"
#include "MRuntime/Function/Render/RenderGuidAllocator.hpp"
#include "MRuntime/Function/Render/RenderSwapContext.hpp"

namespace MiniEngine
{
    class WindowSystem;
    class RHI;
    class RenderResourceBase;
    class RenderPipelineBase;
    class RenderScene;
    class RenderCamera;
    class WindowUI;
    class DebugDrawManager;

    struct RenderSystemInitInfo
    {
        std::shared_ptr<WindowSystem> mWindowSystem;
        std::shared_ptr<DebugDrawManager> mDebugDrawSystem;
    };

    struct EngineContentViewport
    {
        float x { 0.f};
        float y { 0.f};
        float width { 0.f};
        float height { 0.f};
    };

    class RenderSystem
    {
    public:
        RenderSystem() = default;
        ~RenderSystem();
        
        void Initialize(RenderSystemInitInfo initInfo);
        void Tick(float DeltaTime);
        void Clear();

        void                          SwapLogicRenderData();
        RenderSwapContext&            GetSwapContext();
        std::shared_ptr<RenderCamera> GetRenderCamera() const;
        std::shared_ptr<RHI>          GetRHI() const;

        void      SetRenderPipelineType(RENDER_PIPELINE_TYPE pipeline_type);
        void      InitializeUIRenderBackend(WindowUI* window_ui);
        void      UpdateEngineContentViewport(float offset_x, float offset_y, float width, float height);
        uint32_t  GetGuidOfPickedMesh(const Vector2& picked_uv);
        GObjectID GetGObjectIDByMeshID(uint32_t mesh_id) const;

        EngineContentViewport GetEngineContentViewport() const;

        void CreateAxis(std::array<RenderEntity, 3> axis_entities, std::array<RenderMeshData, 3> mesh_datas);
        void SetVisibleAxis(std::optional<RenderEntity> axis);
        void SetSelectedAxis(size_t selected_axis);
        GuidAllocator<GameObjectPartId>& GetGOInstanceIDAllocator();
        GuidAllocator<MeshSourceDesc>&   GetMeshAssetIDAllocator();

        void ClearForLevelReloading();

    private:
        RENDER_PIPELINE_TYPE mRenderPipelineType{RENDER_PIPELINE_TYPE::FORWARD_PIPELINE};

        RenderSwapContext mSwapContext;

        std::shared_ptr<RHI>                mRHI;
        std::shared_ptr<RenderCamera>       mRenderCamera;
        std::shared_ptr<RenderScene>        mRenderScene;
        std::shared_ptr<RenderResourceBase> mRenderResource;
        std::shared_ptr<RenderPipelineBase> mRenderPipeline;
    };
}