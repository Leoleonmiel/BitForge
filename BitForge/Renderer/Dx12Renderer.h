#pragma once
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include "Camera/CinematicCamera.h"
#include "Rendering/SSAO/Ssao.h"
#include "Rendering/VolumetricFog/VolumetricFog.h"
#include "Rendering/TAA/Taa.h"
#include "Assets/Core/AssetManager.h"
#include "Scene/TimeOfDay.h"
#include "UI/ImGuiLayer.h"
#include "Rendering/Graph/RenderGraph.h"
#include "Rendering/Deferred/GBuffer.h"
#include "Rendering/Shadows/ShadowMap.h"
#include "Rendering/Lighting/LightManager.h"
#include "Rendering/Materials/MaterialManager.h"
#include "Rendering/PostProcess/HdrTarget.h"
#include "Rendering/RayTracing/RayCommon.h"

using Microsoft::WRL::ComPtr;

struct MeshData;
struct GLTFPrimitive;

struct Mesh
{
    ComPtr<ID3D12Resource>   vb;
    ComPtr<ID3D12Resource>   ib;
    D3D12_VERTEX_BUFFER_VIEW vbView{};
    D3D12_INDEX_BUFFER_VIEW  ibView{};
    UINT                     indexCount = 0;
};

struct RenderObject
{
    Mesh              mesh;
    DirectX::XMMATRIX transform    = DirectX::XMMatrixIdentity();
    UINT              textureIndex = 0;
    uint32_t          materialID   = 0;
};

struct Texture
{
    ComPtr<ID3D12Resource> resource;
    ComPtr<ID3D12Resource> upload;
    UINT                   width    = 0;
    UINT                   height   = 0;
    UINT                   srvIndex = 0;
};

class Dx12Renderer
{
public:
    void Initialize(HWND hwnd, uint32_t width, uint32_t height);
    void Render(float deltaTime);
    void Shutdown();

    Mesh CreateCube();
    Mesh CreateGround();
    Mesh CreateLightSphere();

    Mesh LoadModel(const std::string& path);

    std::vector<RenderObject> LoadGLTFScene(const std::string& path);

    UINT LoadTexture(const std::string& path);

    void AddRenderObject(const RenderObject& renderObject);

    CinematicCamera&       Camera()       { return m_camera; }
    const CinematicCamera& Camera() const { return m_camera; }
    LightManager&          Lights()       { return m_lightManager; }
    const LightManager&    Lights() const { return m_lightManager; }
    FogSettings&           Fog()          { return m_fog; }
    const FogSettings&     Fog() const    { return m_fog; }
    SsaoSettings&          Ssao()         { return m_ssao; }
    const SsaoSettings&    Ssao() const   { return m_ssao; }
    TaaSettings&           Taa()          { return m_taa; }
    const TaaSettings&     Taa() const    { return m_taa; }

    void SetUICallback(std::function<void()> callback) { m_uiCallback = std::move(callback); }

private:
    static constexpr UINT FrameCount = 2;
    static constexpr UINT MaxCBPerFrame = 256;
    static constexpr UINT LightCount = 3;
    static constexpr UINT MaxTextures = 1024;

    struct alignas(256) CBData
    {
        float mvp[16];
        float world[16];

        float lightPosRadius[LightCount][4];
        float lightColor[LightCount][4];

        float cameraPosSpecPower[4];
        float specColor[4];

        float ambient;
        float pad[3];
    };

    void CreateDeferredPipeline();
    void CreateGBuffer();
    void CreateShadowResources();
    void CreatePostProcess();
    void CreateRayReconstruction();

    ComPtr<ID3DBlob> LoadShader(const std::wstring& path,
                                const std::string& entry,
                                const std::string& target);

    void CreateDepthBuffer();
    void WaitForGpu();

    Mesh CreateMeshFromData(const MeshData& meshData);
    Mesh CreateMeshFromData(const GLTFPrimitive& primitiveData);

    void    CreateSrvHeap();
    Texture CreateTextureFromMemory(const uint8_t* rgba, UINT width, UINT height);
    UINT    LoadTextureAbsolute(const std::string& absPath);
    UINT    LoadTextureCached(const std::string& absPath);

    Assets::AssetManager m_assets;
    uint32_t UploadTexturePixels(const uint8_t* rgba, uint32_t width, uint32_t height);

private:
    HWND m_hwnd = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swapChain;
    ComPtr<ID3D12CommandAllocator> allocators[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> cmdList;

    ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue = 0;
    HANDLE fenceEvent = nullptr;

    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    UINT rtvDescriptorSize = 0;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;

    ComPtr<ID3D12DescriptorHeap> srvHeap;
    UINT srvDescriptorSize = 0;
    UINT srvCount = 0;
    std::vector<Texture> m_textures;
    std::unordered_map<std::string, UINT> m_textureCache;

    ComPtr<ID3D12Resource> backBuffers[FrameCount];
    ComPtr<ID3D12Resource> depthBuffer;

    GBuffer m_gbuffer;
    ComPtr<ID3D12DescriptorHeap> gbufferRtvHeap;
    ComPtr<ID3D12RootSignature> gbufferRootSig;
    ComPtr<ID3D12PipelineState> gbufferPsoVariant[2][2];
    ComPtr<ID3D12RootSignature> lightingRootSig;
    ComPtr<ID3D12PipelineState> lightingPso;
    static constexpr UINT kLightingSrvCount = GBuffer::kCount + 2;
    static constexpr UINT kGBufferSrvBase   = MaxTextures - 1 - kLightingSrvCount;
    int m_debugView = 0;
    uint32_t m_currentFrame = 0;

    HdrTarget m_hdr;
    ComPtr<ID3D12RootSignature> tonemapRootSig;
    ComPtr<ID3D12PipelineState> tonemapPso;
    static constexpr UINT kHdrSrvIndex = kGBufferSrvBase - 1;
    int   m_toneMapOp = 2;

    HdrTarget m_ssr;
    ComPtr<ID3D12RootSignature> ssrRootSig;
    ComPtr<ID3D12PipelineState> ssrPso;
    static constexpr UINT kSsrSrvIndex = kGBufferSrvBase - 2;
    RayReconstructionSettings m_ray;

    ComPtr<ID3D12RootSignature> dofRootSig;
    ComPtr<ID3D12PipelineState> dofPso;
    void CreateDepthOfField();
    void PassDepthOfField(const struct RenderContext& ctx);
    bool m_showThirds    = false;
    bool m_showCrosshair = false;
    bool m_showSafeFrame = false;
    void DrawFramingOverlay();

    SsaoTarget m_ssaoRaw;
    SsaoTarget m_ssaoBlur;
    SsaoSettings m_ssao;
    ComPtr<ID3D12RootSignature> ssaoRootSig;
    ComPtr<ID3D12PipelineState> ssaoPso;
    ComPtr<ID3D12RootSignature> ssaoBlurRootSig;
    ComPtr<ID3D12PipelineState> ssaoBlurPso;
    ComPtr<ID3D12Resource>      ssaoKernelCB;
    D3D12_GPU_DESCRIPTOR_HANDLE m_ssaoNoiseSrv{};
    static constexpr UINT kSsaoSrvIndex     = kGBufferSrvBase - 3;
    static constexpr UINT kSsaoBlurSrvIndex = kGBufferSrvBase - 4;
    void CreateSsao();
    void PassSsao(const struct RenderContext& ctx);
    void PassSsaoBlur(const struct RenderContext& ctx);

    HdrTarget m_fogTarget;
    FogSettings m_fog;
    ComPtr<ID3D12RootSignature> fogRootSig;
    ComPtr<ID3D12PipelineState> fogPso;
    static constexpr UINT kFogSrvIndex = kGBufferSrvBase - 5;
    void CreateVolumetricFog();
    void PassVolumetricFog(const struct RenderContext& ctx);

    HdrTarget   m_taaHistory[2];
    TaaSettings m_taa;
    ComPtr<ID3D12RootSignature> taaRootSig;
    ComPtr<ID3D12PipelineState> taaPso;
    static constexpr UINT kTaaHistSrvA = kGBufferSrvBase - 6;
    static constexpr UINT kTaaHistSrvB = kGBufferSrvBase - 7;
    UINT m_taaWrite = 0;
    bool m_taaHistoryValid = false;
    unsigned m_taaFrame = 0;
    DirectX::XMMATRIX m_prevViewProj      = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_curUnjitteredVP   = DirectX::XMMatrixIdentity();
    void CreateTaa();
    void PassTaa(const struct RenderContext& ctx);

    bool  m_iblEnabled   = true;
    float m_iblIntensity = 1.0f;

    TimeOfDayState m_timeOfDay = TimeOfDayState::Day;
    float m_nightFactor = 0.0f;
    void  ApplyTimeOfDay();

    ComPtr<ID3D12Resource>        m_unifiedVB;
    ComPtr<ID3D12Resource>        m_unifiedIB;
    D3D12_VERTEX_BUFFER_VIEW      m_unifiedVBView{};
    D3D12_INDEX_BUFFER_VIEW       m_unifiedIBView{};
    ComPtr<ID3D12Resource>        m_instanceBuffer;
    ComPtr<ID3D12Resource>        m_indirectCmdBuffer;
    ComPtr<ID3D12RootSignature>   m_indirectRootSig;
    ComPtr<ID3D12PipelineState>   m_indirectGbufferPsoVariant[2][2];
    ComPtr<ID3D12CommandSignature> m_cmdSignature;
    UINT m_indirectDrawCount = 0;
    UINT m_totalInstances    = 0;
    UINT m_visibleInstances  = 0;
    bool m_useIndirect       = true;
    void CreateIndirectPipeline();
    void BuildGpuDrivenScene(const std::vector<struct Vertex>& verts,
                             const std::vector<uint32_t>& indices,
                             const std::vector<struct InstanceData>& instances,
                             const std::vector<struct IndirectDrawCommand>& cmds);
    void PassGBufferIndirect(const struct RenderContext& ctx);

    void CreateGizmoResources();
    Mesh m_gizmoMesh;
    ComPtr<ID3D12RootSignature> gizmoRootSig;
    ComPtr<ID3D12PipelineState> gizmoPso;
    bool m_showGizmos   = true;
    int  m_selectedLight = -1;
    bool m_lmbPrev = false;
    float GizmoRadius() const { return (m_sceneRadius > 1.0f ? m_sceneRadius : 50.0f) * 0.03f; }

    static constexpr UINT kShadowMapSize = 2048;
    static constexpr UINT kShadowSrvIndex = kGBufferSrvBase + GBuffer::kCount;
    ShadowMap m_shadowMap;
    ComPtr<ID3D12RootSignature> shadowRootSig;
    ComPtr<ID3D12PipelineState> shadowPso;
    DirectX::XMMATRIX m_lightViewProj = DirectX::XMMatrixIdentity();
    DirectX::XMFLOAT3 m_sceneCenter{ 0, 0, 0 };
    float m_sceneRadius = 50.0f;
    bool  m_shadowsEnabled = true;
    bool  m_showShadowMap  = false;

    struct GPULight
    {
        float position[3];  float radius;
        float color[3];     float intensity;
        float direction[3]; int   type;
    };
    static constexpr UINT kLightSrvIndex = kGBufferSrvBase + GBuffer::kCount + 1;
    LightManager m_lightManager;

    MaterialManager m_materialManager;
    bool  m_materialOverride  = false;
    float m_overrideMetallic  = 0.0f;
    float m_overrideRoughness = 0.5f;

    ComPtr<ID3D12Resource> lightBuffer;
    GPULight* lightMapped = nullptr;
    int m_lightCount = 0;
    void CreateLightBuffer();
    void UploadLights();
    void SetupSceneLighting();

    std::vector<RenderObject> m_renderObjects;

    ComPtr<ID3D12Resource> constantBuffer;
    uint8_t* cbMapped = nullptr;
    UINT cbStride = 0;
    UINT cbRingIndex = 0;

    D3D12_VIEWPORT viewport{};
    D3D12_RECT scissor{};

    CinematicCamera m_camera;

    ImGuiLayer m_imgui;
    std::function<void()> m_uiCallback;
    void DrawDebugUI();

    RenderGraph m_renderGraph;
    void BuildRenderGraph();
    void BeginFrame(uint32_t& outFrameIndex);
    void EndFrame(uint32_t frameIndex);
    void PassShadow(const struct RenderContext& ctx);
    void PassGBuffer(const struct RenderContext& ctx);
    void PassLighting(const struct RenderContext& ctx);
    void PassRayReconstruction(const struct RenderContext& ctx);
    void PassToneMapping(const struct RenderContext& ctx);
    void PassGizmo(const struct RenderContext& ctx);
    void PassUI(const struct RenderContext& ctx);

    void PushConstants(const CBData& cb);

    float  m_fps = 0.0f;
    double m_cpuMs = 0.0;

    bool m_showUI     = true;
    bool m_wireframe  = false;
    bool m_backfaceCull = false;
    bool m_f1Prev     = false;
};
