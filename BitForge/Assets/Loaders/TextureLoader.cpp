#include "TextureLoader.h"
#include "../../thirdparty/stb_image.h"

namespace Assets
{
    DecodedImage DecodeImageFile(const std::string& absPath)
    {
        DecodedImage result;
        int width = 0, height = 0, channels = 0;

        unsigned char* pixelBytes = stbi_load(absPath.c_str(), &width, &height, &channels, 4);
        if (!pixelBytes) { return result; }

        result.width  = width;
        result.height = height;
        result.pixels.assign(pixelBytes, pixelBytes + (size_t)width * height * 4);
        result.ok = true;

        stbi_image_free(pixelBytes);
        return result;
    }
}
