#pragma once

#include "MRuntime/Core/Math/Vector2.hpp"
#include "MRuntime/Function/Render/RenderPass.hpp"

namespace MiniEngine
{
    class RenderResourceBase;

    struct PickPassInitInfo : RenderPassInitInfo
    {
        RHIDescriptorSetLayout* mPerMeshLayout;
    };

    class PickPass : public RenderPass
    {
    public:
        void Initialize(const RenderPassInitInfo* initInfo) override final;
        void PostInitialize() override final;
        void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;
        void Draw() override final;

        uint32_t Pick(const Vector2& pickedUV);
        void RecreateFramebuffer();

    private:
        void setupAttachments();
        void setupRenderPass();
        void setupFrameBuffer();
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();

    public:
        MeshInefficientPickPerFrameStorageBufferObject mMeshInefficientPickPerFrameStorageBufferObject;

    private:
        RHIImage* mObjectIDImage = nullptr;
        RHIDeviceMemory* mObjectIDImageMemory = nullptr;
        RHIImageView* mObjectIDImageView = nullptr;

        RHIDescriptorSetLayout* mPerMeshLayout = nullptr;
    };
}