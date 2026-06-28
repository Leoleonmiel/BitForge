#pragma once
#include <DirectXMath.h>

class Camera
{
public:
    void Update(float dt);

    DirectX::XMMATRIX GetView() const;
    DirectX::XMMATRIX GetProjection(float aspect) const;

    void SetPosition(float x, float y, float z);
    void SetSpeed(float unitsPerSecond) { m_speed = unitsPerSecond; }
    void LookAt(float tx, float ty, float tz);
    DirectX::XMFLOAT3 GetPosition() const { return m_position; }

    float  GetYaw()   const { return m_yaw; }
    float  GetPitch() const { return m_pitch; }
    float  GetSpeed() const { return m_speed; }
    void   SetYaw(float y)   { m_yaw = y; }
    void   SetPitch(float p) { m_pitch = p; }
    float* PtrPosition()     { return &m_position.x; }
    float* PtrYaw()          { return &m_yaw; }
    float* PtrPitch()        { return &m_pitch; }
    float* PtrSpeed()        { return &m_speed; }

private:
    DirectX::XMFLOAT3 m_position{ 0, 0, 0 };
    float m_yaw         = 0.0f;
    float m_pitch       = 0.0f;
    float m_speed       = 5.0f;
    float m_sensitivity = 0.002f;
};
