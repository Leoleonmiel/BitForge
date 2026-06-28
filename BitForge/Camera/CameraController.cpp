#include "CinematicCamera.h"
#include "../Input/Input.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

static float ShakeNoise(float time, float seed)
{
    float hashed = sinf(time * 37.13f + seed * 17.7f) * 43758.5453f;
    return (hashed - floorf(hashed)) * 2.0f - 1.0f;
}

void CinematicCamera::Update(float dt)
{
    if (dt <= 0.0f) { dt = 1.0f / 60.0f; }

    m_yaw   += Input::GetMouseDeltaX() * m_sensitivity;
    m_pitch += Input::GetMouseDeltaY() * m_sensitivity;
    m_pitch  = std::clamp(m_pitch, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);

    XMVECTOR forward = ForwardVec();
    XMVECTOR right   = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward));

    XMVECTOR wish = XMVectorZero();
    if (Input::IsKeyDown('W') || Input::IsKeyDown('Z'))
    {
        wish = XMVectorAdd(wish, forward);
    }
    if (Input::IsKeyDown('S'))
    {
        wish = XMVectorSubtract(wish, forward);
    }
    if (Input::IsKeyDown('D'))
    {
        wish = XMVectorAdd(wish, right);
    }
    if (Input::IsKeyDown('A') || Input::IsKeyDown('Q'))
    {
        wish = XMVectorSubtract(wish, right);
    }
    if (XMVectorGetX(XMVector3LengthSq(wish)) > 1e-6f)
    {
        wish = XMVectorScale(XMVector3Normalize(wish), m_speed);
    }

    XMVECTOR vel = XMLoadFloat3(&m_velocity);
    if (smoothing)
    {
        const float k = 1.0f - expf(-8.0f * dt);
        vel = XMVectorLerp(vel, wish, k);
    }
    else
    {
        vel = wish;
    }
    XMStoreFloat3(&m_velocity, vel);

    XMVECTOR pos = XMVectorAdd(XMLoadFloat3(&m_position), XMVectorScale(vel, dt));
    XMStoreFloat3(&m_position, pos);

    m_shakeTime += dt;
    m_shake = std::clamp(m_shake - dt * 1.5f, 0.0f, 1.0f);
    const float amp = m_shake * m_shake;
    const float scale = (m_speed > 1.0f ? m_speed : 1.0f) * 0.05f;
    m_shakeOffset = {
        ShakeNoise(m_shakeTime, 1.0f) * amp * scale,
        ShakeNoise(m_shakeTime, 2.0f) * amp * scale,
        ShakeNoise(m_shakeTime, 3.0f) * amp * scale,
    };
}

void CinematicCamera::SetPosition(float x, float y, float z)
{
    m_position = { x, y, z };
    m_velocity = { 0, 0, 0 };
}

void CinematicCamera::LookAt(float tx, float ty, float tz)
{
    const float dx = tx - m_position.x;
    const float dy = ty - m_position.y;
    const float dz = tz - m_position.z;
    const float horiz = sqrtf(dx * dx + dz * dz);
    m_yaw   = atan2f(dx, dz);
    m_pitch = atan2f(dy, (horiz > 1e-5f) ? horiz : 1e-5f);
    const float limit = XM_PIDIV2 - 0.01f;
    m_pitch = std::clamp(m_pitch, -limit, limit);
}
