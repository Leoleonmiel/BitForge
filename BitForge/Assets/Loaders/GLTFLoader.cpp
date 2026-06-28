#include "GLTFLoader.h"
#include "thirdparty/json.hpp"

#include <fstream>
#include <iterator>
#include <cstring>

using json = nlohmann::json;

namespace
{
    constexpr int CT_BYTE           = 5120;
    constexpr int CT_UNSIGNED_BYTE  = 5121;
    constexpr int CT_SHORT          = 5122;
    constexpr int CT_UNSIGNED_SHORT = 5123;
    constexpr int CT_UNSIGNED_INT   = 5125;
    constexpr int CT_FLOAT          = 5126;

    int ComponentByteSize(int ct)
    {
        switch (ct)
        {
        case CT_BYTE:
        case CT_UNSIGNED_BYTE:  return 1;
        case CT_SHORT:
        case CT_UNSIGNED_SHORT: return 2;
        case CT_UNSIGNED_INT:
        case CT_FLOAT:          return 4;
        default:                return 0;
        }
    }

    int NumComponents(const std::string& type)
    {
        if (type == "SCALAR") { return 1; }
        if (type == "VEC2") { return 2; }
        if (type == "VEC3") { return 3; }
        if (type == "VEC4") { return 4; }
        if (type == "MAT4") { return 16; }
        return 0;
    }

    std::string DirOf(const std::string& path)
    {
        size_t slash = path.find_last_of("/\\");
        return (slash == std::string::npos) ? "" : path.substr(0, slash + 1);
    }

    bool ResolveAccessor(const json& gltf,
                         const std::vector<std::vector<uint8_t>>& buffers,
                         int accessorIndex,
                         const uint8_t*& outBase,
                         size_t& outStride,
                         size_t& outCount,
                         int& outComponentType,
                         int& outNumComponents)
    {
        if (accessorIndex < 0 || accessorIndex >= (int)gltf["accessors"].size()) { return false; }

        const json& acc = gltf["accessors"][accessorIndex];
        outCount         = acc.value("count", 0);
        outComponentType = acc.value("componentType", 0);
        outNumComponents = NumComponents(acc.value("type", std::string()));
        if (outCount == 0 || outNumComponents == 0) { return false; }

        const size_t accByteOffset = acc.value("byteOffset", 0);
        const int bvIndex = acc.value("bufferView", -1);
        if (bvIndex < 0 || bvIndex >= (int)gltf["bufferViews"].size()) { return false; }

        const json& bv = gltf["bufferViews"][bvIndex];
        const int bufIndex      = bv.value("buffer", -1);
        const size_t bvOffset   = bv.value("byteOffset", 0);
        if (bufIndex < 0 || bufIndex >= (int)buffers.size()) { return false; }

        const std::vector<uint8_t>& buf = buffers[bufIndex];

        const size_t elemSize = (size_t)ComponentByteSize(outComponentType) * outNumComponents;
        outStride = bv.contains("byteStride") ? (size_t)bv["byteStride"].get<int>() : elemSize;

        const size_t start = bvOffset + accByteOffset;
        const size_t lastByte = start + outStride * (outCount - 1) + elemSize;
        if (start > buf.size() || lastByte > buf.size()) { return false; }

        outBase = buf.data() + start;
        return true;
    }

    std::string ResolveTexturePath(const json& gltf, const json& prim,
                                   const std::string& baseDir)
    {
        try
        {
            if (!prim.contains("material") || !gltf.contains("materials")) { return ""; }

            const int matIdx = prim["material"].get<int>();
            const json& mat = gltf["materials"].at(matIdx);
            if (!mat.contains("pbrMetallicRoughness")) { return ""; }

            const json& pbr = mat["pbrMetallicRoughness"];
            if (!pbr.contains("baseColorTexture")) { return ""; }

            const int texIdx = pbr["baseColorTexture"]["index"].get<int>();
            const int imgIdx = gltf["textures"].at(texIdx)["source"].get<int>();
            const json& img = gltf["images"].at(imgIdx);
            if (img.contains("uri"))
            {
                return baseDir + img["uri"].get<std::string>();
            }
        }
        catch (...) {}
        return "";
    }

    bool LoadPrimitive(const json& gltf,
                       const std::vector<std::vector<uint8_t>>& buffers,
                       const json& prim,
                       const std::string& baseDir,
                       GLTFPrimitive& out)
    {
        if (!prim.contains("attributes")) { return false; }
        const json& attr = prim["attributes"];
        if (!attr.contains("POSITION")) { return false; }

        const uint8_t* base = nullptr;
        size_t stride = 0, count = 0;
        int ct = 0, nc = 0;

        if (!ResolveAccessor(gltf, buffers, attr["POSITION"].get<int>(),
                             base, stride, count, ct, nc) || ct != CT_FLOAT || nc != 3) { return false; }

        out.vertices.resize(count);
        for (size_t i = 0; i < count; i++)
        {
            const float* floatData = reinterpret_cast<const float*>(base + i * stride);
            out.vertices[i].x = floatData[0];
            out.vertices[i].y = floatData[1];
            out.vertices[i].z = floatData[2];
            out.vertices[i].nx = 0.0f; out.vertices[i].ny = 0.0f; out.vertices[i].nz = 1.0f;
            out.vertices[i].r = 1.0f;  out.vertices[i].g = 1.0f;  out.vertices[i].b = 1.0f;
            out.vertices[i].u = 0.0f;  out.vertices[i].v = 0.0f;
        }

        if (attr.contains("NORMAL") &&
            ResolveAccessor(gltf, buffers, attr["NORMAL"].get<int>(), base, stride, count, ct, nc) &&
            ct == CT_FLOAT && nc == 3)
        {
            const size_t vertexLimit = (count < out.vertices.size()) ? count : out.vertices.size();
            for (size_t i = 0; i < vertexLimit; i++)
            {
                const float* floatData = reinterpret_cast<const float*>(base + i * stride);
                out.vertices[i].nx = floatData[0];
                out.vertices[i].ny = floatData[1];
                out.vertices[i].nz = floatData[2];
            }
        }

        if (attr.contains("TEXCOORD_0") &&
            ResolveAccessor(gltf, buffers, attr["TEXCOORD_0"].get<int>(), base, stride, count, ct, nc) &&
            ct == CT_FLOAT && nc == 2)
        {
            const size_t vertexLimit = (count < out.vertices.size()) ? count : out.vertices.size();
            for (size_t i = 0; i < vertexLimit; i++)
            {
                const float* floatData = reinterpret_cast<const float*>(base + i * stride);
                out.vertices[i].u = floatData[0];
                out.vertices[i].v = floatData[1];
            }
        }

        if (prim.contains("indices") &&
            ResolveAccessor(gltf, buffers, prim["indices"].get<int>(), base, stride, count, ct, nc) &&
            nc == 1)
        {
            out.indices.resize(count);
            if (ct == CT_UNSIGNED_INT)
            {
                for (size_t i = 0; i < count; i++)
                {
                    out.indices[i] = *reinterpret_cast<const uint32_t*>(base + i * stride);
                }
            }
            else if (ct == CT_UNSIGNED_SHORT)
            {
                for (size_t i = 0; i < count; i++)
                {
                    out.indices[i] = *reinterpret_cast<const uint16_t*>(base + i * stride);
                }
            }
            else if (ct == CT_UNSIGNED_BYTE)
            {
                for (size_t i = 0; i < count; i++)
                {
                    out.indices[i] = *reinterpret_cast<const uint8_t*>(base + i * stride);
                }
            }
            else
            {
                out.indices.clear();
            }
        }

        if (out.indices.empty()) { return false; }

        out.texturePath = ResolveTexturePath(gltf, prim, baseDir);

        try
        {
            if (prim.contains("material") && gltf.contains("materials"))
            {
                const json& mat = gltf["materials"].at(prim["material"].get<int>());
                if (mat.contains("pbrMetallicRoughness"))
                {
                    const json& pbr = mat["pbrMetallicRoughness"];
                    out.metallicFactor  = pbr.value("metallicFactor", 0.0f);
                    out.roughnessFactor = pbr.value("roughnessFactor", 0.7f);
                }
            }
        }
        catch (...) {}

        return true;
    }
}

GLTFModel LoadGLTF(const std::string& path)
{
    GLTFModel model;

    std::ifstream f(path);
    if (!f.is_open()) { return model; }

    json gltf;
    try { f >> gltf; }
    catch (...) { return model; }

    const std::string baseDir = DirOf(path);

    std::vector<std::vector<uint8_t>> buffers;
    if (!gltf.contains("buffers")) { return model; }
    for (const json& b : gltf["buffers"])
    {
        std::vector<uint8_t> data;
        if (b.contains("uri"))
        {
            const std::string uri = b["uri"].get<std::string>();
            std::ifstream bin(baseDir + uri, std::ios::binary);
            if (bin.is_open())
            {
                data.assign(std::istreambuf_iterator<char>(bin),
                            std::istreambuf_iterator<char>());
            }
        }
        buffers.push_back(std::move(data));
    }

    if (!gltf.contains("meshes")) { return model; }

    for (const json& mesh : gltf["meshes"])
    {
        if (!mesh.contains("primitives")) { continue; }
        for (const json& prim : mesh["primitives"])
        {
            GLTFPrimitive p;
            if (LoadPrimitive(gltf, buffers, prim, baseDir, p))
            {
                model.primitives.push_back(std::move(p));
            }
        }
    }

    return model;
}
