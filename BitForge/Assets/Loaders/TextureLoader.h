#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Assets
{
    struct DecodedImage
    {
        std::vector<uint8_t> pixels;
        int  width  = 0;
        int  height = 0;
        bool ok     = false;
    };

    DecodedImage DecodeImageFile(const std::string& absPath);
}
