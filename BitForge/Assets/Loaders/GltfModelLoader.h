#pragma once
#include "GLTFLoader.h"
#include <string>

namespace Assets
{
    GLTFModel ParseGltfFile(const std::string& absPath);
}
