#include "GBuffer.h"


void GBuffer::Transition(ID3D12GraphicsCommandList* cl, D3D12_RESOURCE_STATES after)
{
    if (m_state == after) { return; }

    D3D12_RESOURCE_BARRIER bars[kCount]{};
    for (int i = 0; i < kCount; i++)
    {
        bars[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        bars[i].Transition.pResource = m_tex[i].Get();
        bars[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        bars[i].Transition.StateBefore = m_state;
        bars[i].Transition.StateAfter = after;
    }
    cl->ResourceBarrier(kCount, bars);
    m_state = after;
}

void GBuffer::ToRenderTargets(ID3D12GraphicsCommandList* cl)
{
    Transition(cl, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void GBuffer::ToShaderResources(ID3D12GraphicsCommandList* cl)
{
    Transition(cl, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
