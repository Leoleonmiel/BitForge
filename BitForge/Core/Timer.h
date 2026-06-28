#pragma once

class Timer
{
public:
    Timer();
    void  Tick();
    float DeltaTime() const;

private:
    double m_secondsPerCount = 0.0;
    long long m_prevCount = 0;
    float m_deltaTime = 0.0f;
};
