#include "RenderGraph.h"


void RenderGraph::AddPass(std::string name, PassExecuteFn execute)
{
    m_passes.push_back(RenderPass{ std::move(name), std::move(execute) });
}

void RenderGraph::Clear()
{
    m_passes.clear();
}
