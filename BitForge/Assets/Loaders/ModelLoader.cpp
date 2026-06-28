#include "ModelLoader.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstdlib>

static bool ParseFaceVertex(const std::string& tok, int& vi, int& ti, int& ni)
{
    vi = ti = ni = 0;
    int positionIndex = 0, texIndex = 0, normalIndex = 0;
    size_t firstSlash = tok.find('/');
    if (firstSlash == std::string::npos)
    {
        positionIndex = std::atoi(tok.c_str());
        vi = positionIndex;
        return positionIndex != 0;
    }
    size_t secondSlash = tok.find('/', firstSlash + 1);
    positionIndex = std::atoi(tok.substr(0, firstSlash).c_str());
    if (secondSlash == std::string::npos)
    {
        texIndex = std::atoi(tok.substr(firstSlash + 1).c_str());
    }
    else
    {
        std::string mid = tok.substr(firstSlash + 1, secondSlash - firstSlash - 1);
        texIndex = mid.empty() ? 0 : std::atoi(mid.c_str());
        normalIndex = std::atoi(tok.substr(secondSlash + 1).c_str());
    }
    vi = positionIndex; ti = texIndex; ni = normalIndex;
    return positionIndex != 0;
}

static int ResolveIndex(int idx, size_t count)
{
    if (idx > 0) { return idx - 1; }
    if (idx < 0)
    {
        return (int)count + idx;
    }
    return -1;
}

MeshData LoadOBJ(const std::string& path)
{
    MeshData out;

    std::ifstream file(path);
    if (!file.is_open()) { return out; }

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;

    std::unordered_map<std::string, uint16_t> unique;
    unique.reserve(4096);

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#') { continue; }

        std::istringstream stream(line);
        std::string tag;
        stream >> tag;

        if (tag == "v")
        {
            float posX, posY, posZ; stream >> posX >> posY >> posZ;
            positions.push_back(posX); positions.push_back(posY); positions.push_back(posZ);
        }
        else if (tag == "vn")
        {
            float normalX, normalY, normalZ; stream >> normalX >> normalY >> normalZ;
            normals.push_back(normalX); normals.push_back(normalY); normals.push_back(normalZ);
        }
        else if (tag == "vt")
        {
            float texU, texV = 0.0f; stream >> texU >> texV;
            uvs.push_back(texU); uvs.push_back(texV);
        }
        else if (tag == "f")
        {
            std::vector<uint16_t> faceIdx;
            std::string tok;
            while (stream >> tok)
            {
                int vi, ti, ni;
                if (!ParseFaceVertex(tok, vi, ti, ni)) { continue; }

                int pIdx = ResolveIndex(vi, positions.size() / 3);
                int tIdx = ResolveIndex(ti, uvs.size() / 2);
                int nIdx = ResolveIndex(ni, normals.size() / 3);

                auto it = unique.find(tok);
                if (it != unique.end())
                {
                    faceIdx.push_back(it->second);
                    continue;
                }

                Vertex vert{};
                if (pIdx >= 0 && (size_t)pIdx * 3 + 2 < positions.size())
                {
                    vert.x = positions[(size_t)pIdx * 3 + 0];
                    vert.y = positions[(size_t)pIdx * 3 + 1];
                    vert.z = positions[(size_t)pIdx * 3 + 2];
                }
                if (nIdx >= 0 && (size_t)nIdx * 3 + 2 < normals.size())
                {
                    vert.nx = normals[(size_t)nIdx * 3 + 0];
                    vert.ny = normals[(size_t)nIdx * 3 + 1];
                    vert.nz = normals[(size_t)nIdx * 3 + 2];
                }
                if (tIdx >= 0 && (size_t)tIdx * 2 + 1 < uvs.size())
                {
                    vert.u = uvs[(size_t)tIdx * 2 + 0];
                    vert.v = 1.0f - uvs[(size_t)tIdx * 2 + 1];
                }
                vert.r = vert.g = vert.b = 1.0f;

                uint16_t newIndex = (uint16_t)out.vertices.size();
                out.vertices.push_back(vert);
                unique.emplace(tok, newIndex);
                faceIdx.push_back(newIndex);
            }

            for (size_t i = 1; i + 1 < faceIdx.size(); i++)
            {
                out.indices.push_back(faceIdx[0]);
                out.indices.push_back(faceIdx[i]);
                out.indices.push_back(faceIdx[i + 1]);
            }
        }
    }

    return out;
}
