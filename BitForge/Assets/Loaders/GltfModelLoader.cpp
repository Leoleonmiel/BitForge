#include "GltfModelLoader.h"

namespace Assets
{
    GLTFModel ParseGltfFile(const std::string& absPath)
    {
        return LoadGLTF(absPath);
    }
}
