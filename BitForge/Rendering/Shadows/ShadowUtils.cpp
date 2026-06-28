#include "ShadowUtils.h"

using namespace DirectX;

namespace ShadowUtils
{
    XMMATRIX DirectionalLightViewProj(XMFLOAT3 sceneCenter, float sceneRadius,
                                      XMFLOAT3 lightDir)
    {
        if (sceneRadius < 1.0f) { sceneRadius = 1.0f; }

        XMVECTOR center = XMLoadFloat3(&sceneCenter);
        XMVECTOR dir    = XMVector3Normalize(XMLoadFloat3(&lightDir));

        XMVECTOR eye = center + dir * (sceneRadius * 2.0f);

        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        if (fabsf(XMVectorGetX(XMVector3Dot(dir, up))) > 0.95f)
        {
            up = XMVectorSet(0, 0, 1, 0);
        }

        XMMATRIX view = XMMatrixLookAtLH(eye, center, up);

        const float diameter = sceneRadius * 2.0f;
        const float nearZ = sceneRadius * 0.5f;
        const float farZ  = sceneRadius * 4.0f;
        XMMATRIX proj = XMMatrixOrthographicLH(diameter, diameter, nearZ, farZ);

        return view * proj;
    }
}
