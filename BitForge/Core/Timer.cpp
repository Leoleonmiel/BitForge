#include "Timer.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

Timer::Timer()
{
    LARGE_INTEGER freq, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    m_secondsPerCount = 1.0 / (double)freq.QuadPart;
    m_prevCount = now.QuadPart;
}

void Timer::Tick()
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double dt = (double)(now.QuadPart - m_prevCount) * m_secondsPerCount;
    m_prevCount = now.QuadPart;

    if (dt < 0.0) { dt = 0.0; }
    if (dt > 0.1) { dt = 0.1; }
    m_deltaTime = (float)dt;
}

float Timer::DeltaTime() const
{
    return m_deltaTime;
}
