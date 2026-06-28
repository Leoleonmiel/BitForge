#include "CinematicCamera.h"
#include <cmath>

using namespace DirectX;

XMVECTOR CinematicCamera::ForwardVec() const
{
    const float cp = cosf(m_pitch);
    return XMVector3Normalize(
        XMVectorSet(cp * sinf(m_yaw), sinf(m_pitch), cp * cosf(m_yaw), 0.0f));
}

DirectX::XMFLOAT3 CinematicCamera::GetForward() const
{
    XMFLOAT3 forward; XMStoreFloat3(&forward, ForwardVec());
    return forward;
}

float CinematicCamera::PhysicalFovY() const
{
    const float clampedFocalLength = (focalLength > 1.0f) ? focalLength : 1.0f;
    return 2.0f * atanf(sensorSize / (2.0f * clampedFocalLength));
}

DirectX::XMMATRIX CinematicCamera::GetView() const
{
    XMVECTOR eye = XMVectorAdd(XMLoadFloat3(&m_position), XMLoadFloat3(&m_shakeOffset));
    XMVECTOR target = XMVectorAdd(eye, ForwardVec());
    return XMMatrixLookAtLH(eye, target, XMVectorSet(0, 1, 0, 0));
}

DirectX::XMMATRIX CinematicCamera::GetProjection(float aspect) const
{
    return XMMatrixPerspectiveFovLH(PhysicalFovY(), aspect, nearPlane, farPlane);
}
