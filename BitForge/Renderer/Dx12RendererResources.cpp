#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include "Vertex.h"
#include "ModelLoader.h"
#include "GLTFLoader.h"
#include "Rendering/GpuDriven/InstanceData.h"
#include "Core/MathSimd.h"
#include <d3dcompiler.h>
#include <vector>
#include <cstring>
#include <cstdio>
#include <stdexcept>

using namespace DirectX;
#include "thirdparty/stb_image.h"

static const Vertex cubeVertices[] =
{
    {-0.5f,-0.5f, 0.5f, 0,0,1, 1,0,0, 0,1},{-0.5f, 0.5f, 0.5f, 0,0,1, 1,0,0, 0,0},
    { 0.5f, 0.5f, 0.5f, 0,0,1, 1,0,0, 1,0},{ 0.5f,-0.5f, 0.5f, 0,0,1, 1,0,0, 1,1},
    { 0.5f,-0.5f,-0.5f, 0,0,-1, 0,1,0, 0,1},{ 0.5f, 0.5f,-0.5f, 0,0,-1, 0,1,0, 0,0},
    {-0.5f, 0.5f,-0.5f, 0,0,-1, 0,1,0, 1,0},{-0.5f,-0.5f,-0.5f, 0,0,-1, 0,1,0, 1,1},
    {-0.5f,-0.5f,-0.5f, -1,0,0, 1,1,0, 0,1},{-0.5f, 0.5f,-0.5f, -1,0,0, 1,1,0, 0,0},
    {-0.5f, 0.5f, 0.5f, -1,0,0, 1,1,0, 1,0},{-0.5f,-0.5f, 0.5f, -1,0,0, 1,1,0, 1,1},
    { 0.5f,-0.5f, 0.5f, 1,0,0, 0,0,1, 0,1},{ 0.5f, 0.5f, 0.5f, 1,0,0, 0,0,1, 0,0},
    { 0.5f, 0.5f,-0.5f, 1,0,0, 0,0,1, 1,0},{ 0.5f,-0.5f,-0.5f, 1,0,0, 0,0,1, 1,1},
    {-0.5f, 0.5f, 0.5f, 0,1,0, 0,1,1, 0,1},{-0.5f, 0.5f,-0.5f, 0,1,0, 0,1,1, 0,0},
    { 0.5f, 0.5f,-0.5f, 0,1,0, 0,1,1, 1,0},{ 0.5f, 0.5f, 0.5f, 0,1,0, 0,1,1, 1,1},
    {-0.5f,-0.5f,-0.5f, 0,-1,0, 1,0,1, 0,1},{-0.5f,-0.5f, 0.5f, 0,-1,0, 1,0,1, 0,0},
    { 0.5f,-0.5f, 0.5f, 0,-1,0, 1,0,1, 1,0},{ 0.5f,-0.5f,-0.5f, 0,-1,0, 1,0,1, 1,1},
};

static const uint16_t cubeIndices[] =
{
    0,1,2,0,2,3, 4,5,6,4,6,7,
    8,9,10,8,10,11, 12,13,14,12,14,15,
    16,17,18,16,18,19, 20,21,22,20,22,23
};

static const Vertex groundVertices[] =
{
    {-5,-0.5f,-5, 0,1,0, 0.4f,0.4f,0.4f, 0,0},
    {-5,-0.5f, 5, 0,1,0, 0.4f,0.4f,0.4f, 0,1},
    { 5,-0.5f, 5, 0,1,0, 0.4f,0.4f,0.4f, 1,1},
    { 5,-0.5f,-5, 0,1,0, 0.4f,0.4f,0.4f, 1,0},
};

static const uint16_t groundIndices[] = { 0,2,1, 0,3,2 };

Mesh Dx12Renderer::CreateCube()
{
    Mesh mesh;
    mesh.indexCount = (UINT)_countof(cubeIndices);
    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_UPLOAD;

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(sizeof(cubeVertices)),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.vb)));
    void* mapped;
    mesh.vb->Map(0, nullptr, &mapped);
    memcpy(mapped, cubeVertices, sizeof(cubeVertices));
    mesh.vb->Unmap(0, nullptr);
    mesh.vbView = { mesh.vb->GetGPUVirtualAddress(), (UINT)sizeof(cubeVertices), sizeof(Vertex) };

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(sizeof(cubeIndices)),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.ib)));
    mesh.ib->Map(0, nullptr, &mapped);
    memcpy(mapped, cubeIndices, sizeof(cubeIndices));
    mesh.ib->Unmap(0, nullptr);
    mesh.ibView = { mesh.ib->GetGPUVirtualAddress(), (UINT)sizeof(cubeIndices), DXGI_FORMAT_R16_UINT };

    return mesh;
}

Mesh Dx12Renderer::CreateGround()
{
    Mesh mesh;
    mesh.indexCount = (UINT)_countof(groundIndices);
    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_UPLOAD;

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(sizeof(groundVertices)),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.vb)));
    void* mapped;
    mesh.vb->Map(0, nullptr, &mapped);
    memcpy(mapped, groundVertices, sizeof(groundVertices));
    mesh.vb->Unmap(0, nullptr);
    mesh.vbView = { mesh.vb->GetGPUVirtualAddress(), (UINT)sizeof(groundVertices), sizeof(Vertex) };

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(sizeof(groundIndices)),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.ib)));
    mesh.ib->Map(0, nullptr, &mapped);
    memcpy(mapped, groundIndices, sizeof(groundIndices));
    mesh.ib->Unmap(0, nullptr);
    mesh.ibView = { mesh.ib->GetGPUVirtualAddress(), (UINT)sizeof(groundIndices), DXGI_FORMAT_R16_UINT };

    return mesh;
}

Mesh Dx12Renderer::CreateLightSphere()
{
    const int segments = 16, rings = 16;
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    for (int ring = 0; ring <= rings; ring++)
    {
        float ringFraction = float(ring) / rings;
        float phi = ringFraction * XM_PI;
        float sphereY = cosf(phi);
        float ringRadius = sinf(phi);

        for (int segment = 0; segment <= segments; segment++)
        {
            float segmentFraction = float(segment) / segments;
            float theta = segmentFraction * XM_2PI;
            float sphereX = ringRadius * cosf(theta);
            float sphereZ = ringRadius * sinf(theta);

            vertices.push_back({ sphereX * 0.5f, sphereY * 0.5f, sphereZ * 0.5f, sphereX, sphereY, sphereZ, 1,1,1, segmentFraction, ringFraction });
        }
    }

    for (int ring = 0; ring < rings; ring++)
    {
        for (int segment = 0; segment < segments; segment++)
        {
            int cornerIndex = ring * (segments + 1) + segment;
            int nextRingIndex = cornerIndex + (segments + 1);
            indices.insert(indices.end(),
            {
                (uint16_t)cornerIndex,(uint16_t)nextRingIndex,(uint16_t)(cornerIndex + 1),
                (uint16_t)(cornerIndex + 1),(uint16_t)nextRingIndex,(uint16_t)(nextRingIndex + 1)
            });
        }
    }

    Mesh mesh;
    mesh.indexCount = (UINT)indices.size();

    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_UPLOAD;

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(vertices.size() * sizeof(Vertex)),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.vb)));
    void* mapped;
    mesh.vb->Map(0, nullptr, &mapped);
    memcpy(mapped, vertices.data(), vertices.size() * sizeof(Vertex));
    mesh.vb->Unmap(0, nullptr);
    mesh.vbView = { mesh.vb->GetGPUVirtualAddress(), (UINT)(vertices.size() * sizeof(Vertex)), sizeof(Vertex) };

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(indices.size() * sizeof(uint16_t)),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.ib)));
    mesh.ib->Map(0, nullptr, &mapped);
    memcpy(mapped, indices.data(), indices.size() * sizeof(uint16_t));
    mesh.ib->Unmap(0, nullptr);
    mesh.ibView = { mesh.ib->GetGPUVirtualAddress(), (UINT)(indices.size() * sizeof(uint16_t)), DXGI_FORMAT_R16_UINT };

    return mesh;
}

Mesh Dx12Renderer::CreateMeshFromData(const MeshData& meshData)
{
    Mesh mesh;
    mesh.indexCount = (UINT)meshData.indices.size();
    if (meshData.vertices.empty() || meshData.indices.empty()) { return mesh; }

    const size_t vbBytes = meshData.vertices.size() * sizeof(Vertex);
    const size_t ibBytes = meshData.indices.size() * sizeof(uint16_t);

    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_UPLOAD;

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(vbBytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.vb)));
    void* mapped;
    mesh.vb->Map(0, nullptr, &mapped);
    memcpy(mapped, meshData.vertices.data(), vbBytes);
    mesh.vb->Unmap(0, nullptr);
    mesh.vbView = { mesh.vb->GetGPUVirtualAddress(), (UINT)vbBytes, sizeof(Vertex) };

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(ibBytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.ib)));
    mesh.ib->Map(0, nullptr, &mapped);
    memcpy(mapped, meshData.indices.data(), ibBytes);
    mesh.ib->Unmap(0, nullptr);
    mesh.ibView = { mesh.ib->GetGPUVirtualAddress(), (UINT)ibBytes, DXGI_FORMAT_R16_UINT };

    return mesh;
}

Mesh Dx12Renderer::LoadModel(const std::string& path)
{
    MeshData meshData = LoadOBJ(ExeDirA() + path);
    return CreateMeshFromData(meshData);
}

Mesh Dx12Renderer::CreateMeshFromData(const GLTFPrimitive& primitiveData)
{
    Mesh mesh;
    mesh.indexCount = (UINT)primitiveData.indices.size();
    if (primitiveData.vertices.empty() || primitiveData.indices.empty()) { return mesh; }

    const size_t vbBytes = primitiveData.vertices.size() * sizeof(Vertex);
    const size_t ibBytes = primitiveData.indices.size() * sizeof(uint32_t);

    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_UPLOAD;

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(vbBytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.vb)));
    void* mapped;
    mesh.vb->Map(0, nullptr, &mapped);
    memcpy(mapped, primitiveData.vertices.data(), vbBytes);
    mesh.vb->Unmap(0, nullptr);
    mesh.vbView = { mesh.vb->GetGPUVirtualAddress(), (UINT)vbBytes, sizeof(Vertex) };

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(ibBytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.ib)));
    mesh.ib->Map(0, nullptr, &mapped);
    memcpy(mapped, primitiveData.indices.data(), ibBytes);
    mesh.ib->Unmap(0, nullptr);
    mesh.ibView = { mesh.ib->GetGPUVirtualAddress(), (UINT)ibBytes, DXGI_FORMAT_R32_UINT };

    return mesh;
}

UINT Dx12Renderer::LoadTextureCached(const std::string& absPath)
{
    auto it = m_textureCache.find(absPath);
    if (it != m_textureCache.end()) { return it->second; }

    UINT index = LoadTextureAbsolute(absPath);
    m_textureCache.emplace(absPath, index);
    return index;
}

std::vector<RenderObject> Dx12Renderer::LoadGLTFScene(const std::string& path)
{
    std::vector<RenderObject> objects;

    Assets::ModelHandle mh = m_assets.LoadModelAsync(ExeDirA() + path);
    m_assets.WaitModel(mh);
    const GLTFModel* parsed = m_assets.GetModel(mh);
    if (!parsed) { return objects; }
    const GLTFModel& model = *parsed;
    objects.reserve(model.primitives.size());

    std::unordered_map<std::string, Assets::TextureHandle> texHandles;
    for (const GLTFPrimitive& prim : model.primitives)
    {
        if (!prim.texturePath.empty() && !texHandles.count(prim.texturePath))
        {
            texHandles.emplace(prim.texturePath, m_assets.LoadTextureAsync(prim.texturePath));
        }
    }
    m_assets.WaitAll();

    DirectX::XMVECTOR vMin = MathSimd::AabbMinInit();
    DirectX::XMVECTOR vMax = MathSimd::AabbMaxInit();

    std::vector<Vertex>              mergedVerts;
    std::vector<uint32_t>            mergedIndices;
    std::vector<InstanceData>        instances;
    std::vector<IndirectDrawCommand> cmds;
    instances.reserve(model.primitives.size());
    cmds.reserve(model.primitives.size());

    for (const GLTFPrimitive& prim : model.primitives)
    {
        RenderObject renderObject{};
        renderObject.mesh = CreateMeshFromData(prim);

        renderObject.textureIndex = prim.texturePath.empty()
            ? 0u
            : m_assets.TextureSrv(texHandles[prim.texturePath]);

        Material material;
        material.textures.albedo    = renderObject.textureIndex;
        material.metallicFactor     = prim.metallicFactor;
        material.roughnessFactor    = prim.roughnessFactor;
        material.aoFactor           = 1.0f;
        renderObject.materialID = m_materialManager.AddMaterial(material);

        renderObject.transform = DirectX::XMMatrixIdentity();

        MathSimd::AccumulateAabb(prim.vertices.data(), prim.vertices.size(), vMin, vMax);

        IndirectDrawCommand cmd{};
        cmd.instanceIndex         = (uint32_t)instances.size();
        cmd.indexCountPerInstance = (uint32_t)prim.indices.size();
        cmd.instanceCount         = 1;
        cmd.startIndexLocation    = (uint32_t)mergedIndices.size();
        cmd.baseVertexLocation    = (int32_t)mergedVerts.size();
        cmd.startInstanceLocation = 0;
        cmds.push_back(cmd);

        mergedVerts.insert(mergedVerts.end(), prim.vertices.begin(), prim.vertices.end());
        mergedIndices.insert(mergedIndices.end(), prim.indices.begin(), prim.indices.end());

        InstanceData inst{};
        DirectX::XMMATRIX transposedWorld = DirectX::XMMatrixTranspose(renderObject.transform);
        memcpy(inst.world, &transposedWorld, sizeof(inst.world));
        inst.texIndex  = renderObject.textureIndex;
        inst.metallic  = prim.metallicFactor;
        inst.roughness = prim.roughnessFactor;
        inst.ao        = 1.0f;
        instances.push_back(inst);

        objects.push_back(std::move(renderObject));
    }

    BuildGpuDrivenScene(mergedVerts, mergedIndices, instances, cmds);

    DirectX::XMFLOAT3 mnF, mxF;
    DirectX::XMStoreFloat3(&mnF, vMin);
    DirectX::XMStoreFloat3(&mxF, vMax);
    const float mn[3] = { mnF.x, mnF.y, mnF.z };
    const float mx[3] = { mxF.x, mxF.y, mxF.z };

    if (!objects.empty() && mx[0] >= mn[0])
    {
        const float cx = 0.5f * (mn[0] + mx[0]);
        const float cy = 0.5f * (mn[1] + mx[1]);
        const float cz = 0.5f * (mn[2] + mx[2]);

        float ex = 0.5f * (mx[0] - mn[0]);
        float ey = 0.5f * (mx[1] - mn[1]);
        float ez = 0.5f * (mx[2] - mn[2]);

        float extents[3] = { ex, ey, ez };
        float lo = extents[0], hi = extents[0];
        for (int k = 1; k < 3; k++)
        {
            lo = (extents[k] < lo) ? extents[k] : lo;
            hi = (extents[k] > hi) ? extents[k] : hi;
        }
        const float median = ex + ey + ez - lo - hi;
        const float cap = median * 4.0f;
        ex = (ex > cap) ? cap : ex;
        ey = (ey > cap) ? cap : ey;
        ez = (ez > cap) ? cap : ez;

        const float radius = sqrtf(ex * ex + ey * ey + ez * ez);

        m_sceneCenter = DirectX::XMFLOAT3(cx, cy, cz);
        m_sceneRadius = radius;

        const bool xLongest = (ex >= ez);
        const float longExtent = xLongest ? ex : ez;

        const float eyeY = mn[1] + ey * 0.25f;

        float px = cx, pz = cz;
        if (xLongest) { px = cx - longExtent * 0.70f; }
        else { pz = cz - longExtent * 0.70f; }

        m_camera.SetPosition(px, eyeY, pz);
        m_camera.LookAt(cx, eyeY, cz);
        m_camera.SetSpeed((radius > 1.0f ? radius : 10.0f) * 0.2f);

        char dbg[192];
        sprintf_s(dbg, "[BitForge] AABB min(%.1f,%.1f,%.1f) max(%.1f,%.1f,%.1f) "
                       "radius=%.2f longExtent=%.2f\n",
                  mn[0], mn[1], mn[2], mx[0], mx[1], mx[2], radius, longExtent);
        OutputDebugStringA(dbg);
    }

    SetupSceneLighting();
    ApplyTimeOfDay();

    m_taaHistoryValid = false;

    return objects;
}
Texture Dx12Renderer::CreateTextureFromMemory(const uint8_t* rgba, UINT width, UINT height)
{
    Texture tex;
    tex.width = width;
    tex.height = height;

    D3D12_RESOURCE_DESC td{};
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = width;
    td.Height = height;
    td.DepthOrArraySize = 1;
    td.MipLevels = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_HEAP_PROPERTIES defaultHeap{};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    HR(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &td,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&tex.resource)));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT   numRows = 0;
    UINT64 rowSizeBytes = 0;
    UINT64 totalBytes = 0;
    device->GetCopyableFootprints(&td, 0, 1, 0, &footprint, &numRows, &rowSizeBytes, &totalBytes);

    D3D12_HEAP_PROPERTIES uploadHeap{};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    HR(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &BufferDesc(totalBytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&tex.upload)));

    uint8_t* mapped = nullptr;
    HR(tex.upload->Map(0, nullptr, (void**)&mapped));
    const UINT srcRowPitch = width * 4;
    for (UINT row = 0; row < numRows; row++)
    {
        memcpy(mapped + footprint.Offset + (size_t)row * footprint.Footprint.RowPitch,
            rgba + (size_t)row * srcRowPitch,
            srcRowPitch);
    }
    tex.upload->Unmap(0, nullptr);

    allocators[0]->Reset();
    cmdList->Reset(allocators[0].Get(), nullptr);

    D3D12_TEXTURE_COPY_LOCATION dst{};
    dst.pResource = tex.resource.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src{};
    src.pResource = tex.upload.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = footprint;

    cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = tex.resource.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->Close();
    ID3D12CommandList* lists[] = { cmdList.Get() };
    queue->ExecuteCommandLists(1, lists);
    WaitForGpu();

    tex.srvIndex = srvCount++;

    D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += (size_t)tex.srvIndex * srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC sd{};
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sd.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(tex.resource.Get(), &sd, handle);

    return tex;
}

void Dx12Renderer::CreateSrvHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = MaxTextures;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap)));

    srvDescriptorSize =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

UINT Dx12Renderer::LoadTextureAbsolute(const std::string& absPath)
{
    int width = 0, height = 0, channels = 0;

    unsigned char* pixelBytes = stbi_load(absPath.c_str(), &width, &height, &channels, 4);

    if (!pixelBytes)
    {
        throw std::runtime_error("Failed to load image: " + absPath);
    }

    Texture texture = CreateTextureFromMemory(pixelBytes, (UINT)width, (UINT)height);

    stbi_image_free(pixelBytes);

    UINT index = texture.srvIndex;
    m_textures.push_back(std::move(texture));
    return index;
}

UINT Dx12Renderer::LoadTexture(const std::string& path)
{
    return LoadTextureAbsolute(ExeDirA() + path);
}

uint32_t Dx12Renderer::UploadTexturePixels(const uint8_t* rgba, uint32_t width, uint32_t height)
{
    Texture texture = CreateTextureFromMemory(rgba, (UINT)width, (UINT)height);
    UINT index = texture.srvIndex;
    m_textures.push_back(std::move(texture));
    return (uint32_t)index;
}
