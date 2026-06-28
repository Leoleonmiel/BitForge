#include "RenderGraph.h"


void RenderGraph::Execute(const RenderContext& ctx) const
{
    for (const auto& pass : m_passes)
    {
        if (pass.execute)
        {
            pass.execute(ctx);
        }
    }
}
