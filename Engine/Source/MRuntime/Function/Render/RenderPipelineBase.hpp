#pragma once

#include <memory>
#include <vector>

#include "Function/Render/Interface/RHI.hpp"
#include "Function/Render/RenderResourceBase.hpp"

namespace MiniEngine
{
    class RenderPipelineBase
    {
        friend class RenderSystem;

    public:
        virtual void ForwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> renderResource);

    protected:
        std::shared_ptr<RHI> mRHI;
    };
}

