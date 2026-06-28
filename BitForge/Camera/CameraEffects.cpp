#include "CinematicCamera.h"

void CinematicCamera::ApplyPreset(CameraPreset p)
{
    switch (p)
    {
    case CameraPreset::WideShot:
        focalLength = 24.0f;
        aperture    = 8.0f;
        depthOfFieldStrength = 0.3f;
        dofEnabled  = false;
        autoFocus   = true;
        break;

    case CameraPreset::CloseUp:
        focalLength = 85.0f;
        aperture    = 1.8f;
        depthOfFieldStrength = 1.0f;
        dofEnabled  = true;
        autoFocus   = true;
        break;

    case CameraPreset::Landscape:
        focalLength = 35.0f;
        aperture    = 11.0f;
        ISO         = 100.0f;
        shutterSpeed = 1.0f / 125.0f;
        depthOfFieldStrength = 0.15f;
        dofEnabled  = false;
        autoFocus   = true;
        break;
    }
}
