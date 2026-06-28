#include "CinematicCamera.h"
#include <cmath>

static float Ev100(float aperture, float shutterTime, float iso)
{
    return std::log2((aperture * aperture / shutterTime) * (100.0f / iso));
}

float CinematicCamera::GetExposure() const
{
    const float refEV = Ev100(2.8f, 1.0f / 50.0f, 100.0f);
    const float ev    = Ev100((std::max)(aperture, 0.1f),
                              (std::max)(shutterSpeed, 1e-5f),
                              (std::max)(ISO, 1.0f));
    return exposureComp * std::exp2(refEV - ev);
}
