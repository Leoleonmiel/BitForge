#include "Camera.h"
#include "../Input/Input.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

void Camera::Update(float dt)
{
    m_yaw   += Input::GetMouseDeltaX() * m_sensitivity;
    m_pitch += Input::GetMouseDeltaY() * m_sensitivity;

    m_pitch = std::clamp(m_pitch, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);

    const float cp = cosf(m_pitch);
    XMVECTOR forward = XMVector3Normalize(
        XMVectorSet(cp * sinf(m_yaw), sinf(m_pitch), cp * cosf(m_yaw), 0.0f));

    XMVECTOR right = XMVector3Normalize(
        XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward));

    XMVECTOR pos  = XMLoadFloat3(&m_position);
    const float   step = m_speed * dt;

    if (Input::IsKeyDown('W'))
    {
        pos = XMVectorAdd(pos, XMVectorScale(forward,  step));
    }
    if (Input::IsKeyDown('S'))
    {
        pos = XMVectorAdd(pos, XMVectorScale(forward, -step));
    }
    if (Input::IsKeyDown('D'))
    {
        pos = XMVectorAdd(pos, XMVectorScale(right,    step));
    }
    if (Input::IsKeyDown('A'))
    {
        pos = XMVectorAdd(pos, XMVectorScale(right,   -step));
    }

    XMStoreFloat3(&m_position, pos);
}

DirectX::XMMATRIX Camera::GetView() const
{
    const float cp = cosf(m_pitch);
    XMVECTOR forward = XMVector3Normalize(
        XMVectorSet(cp * sinf(m_yaw), sinf(m_pitch), cp * cosf(m_yaw), 0.0f));

    XMVECTOR eye    = XMLoadFloat3(&m_position);
    XMVECTOR target = XMVectorAdd(eye, forward);

    return XMMatrixLookAtLH(eye, target, XMVectorSet(0, 1, 0, 0));
}

DirectX::XMMATRIX Camera::GetProjection(float aspect) const
{
    return XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.5f, 20000.0f);
}

void Camera::SetPosition(float x, float y, float z)
{
    m_position = { x, y, z };
}

void Camera::LookAt(float tx, float ty, float tz)
{
    float dx = tx - m_position.x;
    float dy = ty - m_position.y;
    float dz = tz - m_position.z;

    const float horiz = sqrtf(dx * dx + dz * dz);
    m_yaw   = atan2f(dx, dz);
    m_pitch = atan2f(dy, (horiz > 1e-5f) ? horiz : 1e-5f);

    const float limit = XM_PIDIV2 - 0.01f;
    m_pitch = std::clamp(m_pitch, -limit, limit);
}
