#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include <cstring>

using namespace DirectX;

void Dx12Renderer::CreateLightBuffer()
{
    const UINT count = LightManager::kMaxLights;
    const UINT stride = sizeof(GPULight);
    const UINT64 bytes = UINT64(stride) * count;

    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_UPLOAD;

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(bytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&lightBuffer)));

    lightBuffer->Map(0, nullptr, (void**)&lightMapped);
    memset(lightMapped, 0, (size_t)bytes);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
    cpu.ptr += (SIZE_T)kLightSrvIndex * srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC sd{};
    sd.Format = DXGI_FORMAT_UNKNOWN;
    sd.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sd.Buffer.FirstElement = 0;
    sd.Buffer.NumElements = count;
    sd.Buffer.StructureByteStride = stride;
    sd.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    device->CreateShaderResourceView(lightBuffer.Get(), &sd, cpu);
}

void Dx12Renderer::UploadLights()
{
    const auto& lights = m_lightManager.GetLights();

    int uploadedCount = 0;
    for (const Light& light : lights)
    {
        if (!light.enabled) { continue; }
        if (uploadedCount >= LightManager::kMaxLights) { break; }
        GPULight& gpuLight = lightMapped[uploadedCount++];
        gpuLight.position[0] = light.position.x; gpuLight.position[1] = light.position.y; gpuLight.position[2] = light.position.z;
        gpuLight.radius = light.radius;
        gpuLight.color[0] = light.color.x; gpuLight.color[1] = light.color.y; gpuLight.color[2] = light.color.z;
        gpuLight.intensity = light.intensity;
        gpuLight.direction[0] = light.direction.x; gpuLight.direction[1] = light.direction.y; gpuLight.direction[2] = light.direction.z;
        gpuLight.type = light.type;
    }
    m_lightCount = uploadedCount;
}
