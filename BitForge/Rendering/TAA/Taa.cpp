#include "Taa.h"

static float HaltonSeq(int index, int base)
{
    float fraction = 1.0f, result = 0.0f;
    int i = index;
    while (i > 0)
    {
        fraction /= (float)base;
        result += fraction * (float)(i % base);
        i /= base;
    }
    return result;
}

void TaaHaltonJitter(unsigned frame, float& jitterX, float& jitterY)
{
    const int i = (int)(frame % 16u) + 1;
    jitterX = HaltonSeq(i, 2) - 0.5f;
    jitterY = HaltonSeq(i, 3) - 0.5f;
}
