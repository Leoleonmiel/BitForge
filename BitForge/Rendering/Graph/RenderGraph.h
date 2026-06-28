#pragma once
#include "RenderPass.h"
#include <vector>
#include <string>
#include <utility>

class RenderGraph
{
public:
    void AddPass(std::string name, PassExecuteFn execute);

    void Execute(const RenderContext& ctx) const;

    void Clear();

    size_t PassCount() const { return m_passes.size(); }

private:
    std::vector<RenderPass> m_passes;
};
