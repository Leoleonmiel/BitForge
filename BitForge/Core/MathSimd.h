#pragma once
#include "Vertex.h"
#include <DirectXMath.h>
#include <cfloat>
#include <cstddef>

namespace MathSimd
{
    using namespace DirectX;

    inline void AccumulateAabb(const Vertex* verts, size_t count,
                               XMVECTOR& vmin, XMVECTOR& vmax)
    {
        for (size_t i = 0; i < count; ++i)
        {
            XMVECTOR p = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&verts[i].x));
            vmin = XMVectorMin(vmin, p);
            vmax = XMVectorMax(vmax, p);
        }
    }

    inline XMVECTOR AabbMinInit() { return XMVectorReplicate( FLT_MAX); }
    inline XMVECTOR AabbMaxInit() { return XMVectorReplicate(-FLT_MAX); }

    inline XMMATRIX TransposedMvp(const XMMATRIX& world, const XMMATRIX& viewProj)
    {
        return XMMatrixTranspose(XMMatrixMultiply(world, viewProj));
    }
}
