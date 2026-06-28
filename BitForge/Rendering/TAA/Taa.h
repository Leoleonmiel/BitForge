#pragma once


struct TaaSettings
{
    bool  enabled = true;
    float blend   = 0.1f;
    int   debug   = 0;
};

void TaaHaltonJitter(unsigned frame, float& jitterX, float& jitterY);
