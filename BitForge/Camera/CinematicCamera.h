#pragma once
#include <DirectXMath.h>
#include <algorithm>


enum class CameraPreset { WideShot, CloseUp, Landscape };

class CinematicCamera
{
public:
    void Update(float dt);

    DirectX::XMMATRIX GetView() const;
    DirectX::XMMATRIX GetProjection(float aspect) const;
    float GetExposure() const;
    float PhysicalFovY() const;

    void SetPosition(float x, float y, float z);
    void LookAt(float tx, float ty, float tz);
    void SetSpeed(float unitsPerSecond) { m_speed = unitsPerSecond; }
    DirectX::XMFLOAT3 GetPosition() const { return m_position; }
    DirectX::XMFLOAT3 GetForward() const;

    void ApplyPreset(CameraPreset p);
    void Shake(float intensity) { m_shake = (std::max)(m_shake, intensity); }

    float focalLength = 35.0f;
    float sensorSize  = 24.0f;
    float aperture    = 2.8f;
    float shutterSpeed = 1.0f / 50.0f;
    float ISO         = 100.0f;
    float exposureComp = 1.2f;
    float nearPlane   = 0.5f;
    float farPlane    = 20000.0f;
    float focusDistance = 100.0f;
    float depthOfFieldStrength = 1.0f;
    bool  autoFocus   = true;
    bool  dofEnabled  = false;
    bool  smoothing   = true;

    float  GetYaw()   const { return m_yaw; }
    float  GetPitch() const { return m_pitch; }
    float  GetSpeed() const { return m_speed; }
    void   SetYaw(float yaw)   { m_yaw = yaw; }
    void   SetPitch(float pitch) { m_pitch = pitch; }
    float* PtrPosition()     { return &m_position.x; }
    float* PtrYaw()          { return &m_yaw; }
    float* PtrPitch()        { return &m_pitch; }
    float* PtrSpeed()        { return &m_speed; }

private:
    DirectX::XMVECTOR ForwardVec() const;

    DirectX::XMFLOAT3 m_position{ 0, 0, 0 };
    DirectX::XMFLOAT3 m_velocity{ 0, 0, 0 };
    float m_yaw         = 0.0f;
    float m_pitch       = 0.0f;
    float m_speed       = 5.0f;
    float m_sensitivity = 0.002f;

    float m_shake       = 0.0f;
    float m_shakeTime   = 0.0f;
    DirectX::XMFLOAT3 m_shakeOffset{ 0, 0, 0 };
};
