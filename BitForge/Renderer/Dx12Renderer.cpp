#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include "Input/Input.h"
#include "Rendering/Graph/RenderPass.h"
#include "Rendering/Shadows/ShadowUtils.h"
#include "Rendering/Debug/Picking.h"
#include "Core/MathSimd.h"
#include "thirdparty/imgui/imgui.h"
#include <cstring>
#include <cstdint>

using namespace DirectX;

#pragma comment(lib,"d3d12")
#pragma comment(lib,"dxgi")
#pragma comment(lib,"d3dcompiler")

#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

void Dx12Renderer::AddRenderObject(const RenderObject& renderObject)
{
    m_renderObjects.push_back(renderObject);
}


void Dx12Renderer::PushConstants(const CBData& cb)
{
    const UINT idx = cbRingIndex++;
    memcpy(cbMapped + size_t(idx) * cbStride, &cb, sizeof(cb));
    cmdList->SetGraphicsRootConstantBufferView(
        0, constantBuffer->GetGPUVirtualAddress() + UINT64(idx) * cbStride);
}

void Dx12Renderer::BuildRenderGraph()
{
    m_renderGraph.Clear();
    m_renderGraph.AddPass("Shadow",    [this](const RenderContext& c) { PassShadow(c); });
    m_renderGraph.AddPass("GBuffer",   [this](const RenderContext& c) { PassGBuffer(c); });
    m_renderGraph.AddPass("SSAO",      [this](const RenderContext& c) { PassSsao(c); });
    m_renderGraph.AddPass("SSAOBlur",  [this](const RenderContext& c) { PassSsaoBlur(c); });
    m_renderGraph.AddPass("Lighting",  [this](const RenderContext& c) { PassLighting(c); });
    m_renderGraph.AddPass("RayRecon",  [this](const RenderContext& c) { PassRayReconstruction(c); });
    m_renderGraph.AddPass("DoF",       [this](const RenderContext& c) { PassDepthOfField(c); });
    m_renderGraph.AddPass("Fog",       [this](const RenderContext& c) { PassVolumetricFog(c); });
    m_renderGraph.AddPass("TAA",       [this](const RenderContext& c) { PassTaa(c); });
    m_renderGraph.AddPass("ToneMap",   [this](const RenderContext& c) { PassToneMapping(c); });
    m_renderGraph.AddPass("Gizmo",     [this](const RenderContext& c) { PassGizmo(c); });
    m_renderGraph.AddPass("UI",        [this](const RenderContext& c) { PassUI(c); });
}

void Dx12Renderer::BeginFrame(uint32_t& outFrameIndex)
{
    cbRingIndex = 0;

    const UINT fi = swapChain->GetCurrentBackBufferIndex();
    outFrameIndex = fi;

    allocators[fi]->Reset();
    cmdList->Reset(allocators[fi].Get(), nullptr);

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D12DescriptorHeap* heaps[] = { srvHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);
}

void Dx12Renderer::EndFrame(uint32_t frameIndex)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backBuffers[frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->Close();
    ID3D12CommandList* lists[] = { cmdList.Get() };
    queue->ExecuteCommandLists(1, lists);
    swapChain->Present(1, 0);

    WaitForGpu();
}

void Dx12Renderer::Render(float deltaTime)
{
    if (deltaTime > 0.0f)
    {
        const float inst = 1.0f / deltaTime;
        m_fps = (m_fps <= 0.0f) ? inst : (m_fps * 0.95f + inst * 0.05f);
    }

    const bool f1 = Input::IsKeyDown(VK_F1);
    if (f1 && !m_f1Prev) { m_showUI = !m_showUI; }
    m_f1Prev = f1;

    m_imgui.BeginFrame();
    if (m_showUI)
    {
        DrawDebugUI();
        if (m_uiCallback) { m_uiCallback(); }
    }

    const bool uiCapturing = m_showUI &&
        (ImGuiLayer::WantCaptureMouse() || ImGuiLayer::WantCaptureKeyboard());
    if (!uiCapturing)
    {
        m_camera.Update(deltaTime);
    }

    if (m_renderGraph.PassCount() == 0)
    {
        BuildRenderGraph();
    }

    const float aspect = float(m_width) / float(m_height);
    RenderContext ctx{};
    ctx.view = m_camera.GetView();
    ctx.proj = m_camera.GetProjection(aspect);
    ctx.viewProj = XMMatrixMultiply(ctx.view, ctx.proj);
    ctx.cameraPos = m_camera.GetPosition();

    m_curUnjitteredVP = ctx.viewProj;
    if (m_taa.enabled)
    {
        float jx, jy;
        TaaHaltonJitter(m_taaFrame++, jx, jy);
        const float ndcX = jx * 2.0f / float(m_width);
        const float ndcY = jy * 2.0f / float(m_height);
        XMMATRIX projJ = ctx.proj;
        projJ.r[2] = XMVectorAdd(projJ.r[2], XMVectorSet(ndcX, ndcY, 0.0f, 0.0f));
        ctx.viewProj = XMMatrixMultiply(ctx.view, projJ);
    }

    const bool lmb = Input::IsMouseDown(VK_LBUTTON);
    const bool rmb = Input::IsMouseDown(VK_RBUTTON);
    if (m_showGizmos && lmb && !m_lmbPrev && !rmb &&
        !ImGuiLayer::WantCaptureMouse())
    {
        m_selectedLight = Picking::PickLight(
            Input::GetMouseX(), Input::GetMouseY(), (int)m_width, (int)m_height,
            ctx.view, ctx.proj, ctx.cameraPos,
            m_lightManager.GetLights(), GizmoRadius());
    }
    m_lmbPrev = lmb;

    m_assets.Pump(4);

    UploadLights();

    LARGE_INTEGER qf, q0, q1;
    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);

    BeginFrame(m_currentFrame);
    ctx.cmdList = cmdList.Get();
    m_renderGraph.Execute(ctx);

    QueryPerformanceCounter(&q1);
    const double ms = double(q1.QuadPart - q0.QuadPart) * 1000.0 / double(qf.QuadPart);
    m_cpuMs = (m_cpuMs <= 0.0) ? ms : (m_cpuMs * 0.9 + ms * 0.1);

    m_prevViewProj = m_curUnjitteredVP;

    EndFrame(m_currentFrame);
}

void Dx12Renderer::PassShadow(const RenderContext& ctx)
{
    auto* cl = ctx.cmdList;

    XMFLOAT3 sunDir{ 0.4f, 0.6f, 0.3f };
    for (const Light& lt : m_lightManager.GetLights())
    {
        if (lt.type == (int)LightType::Directional)
        {
            sunDir = lt.direction;
            break;
        }
    }
    m_lightViewProj = ShadowUtils::DirectionalLightViewProj(
        m_sceneCenter, m_sceneRadius, sunDir);

    if (!m_shadowsEnabled) { return; }

    m_shadowMap.ToDepthWrite(cl);

    auto dsv = m_shadowMap.Dsv();
    cl->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
    cl->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT vp{ 0, 0, (float)m_shadowMap.Size(), (float)m_shadowMap.Size(), 0, 1 };
    D3D12_RECT     sc{ 0, 0, (LONG)m_shadowMap.Size(), (LONG)m_shadowMap.Size() };
    cl->RSSetViewports(1, &vp);
    cl->RSSetScissorRects(1, &sc);

    cl->SetGraphicsRootSignature(shadowRootSig.Get());
    cl->SetPipelineState(shadowPso.Get());

    for (auto& renderObject : m_renderObjects)
    {
        XMMATRIX worldViewProj = XMMatrixTranspose(renderObject.transform * m_lightViewProj);
        CBData cb{};
        memcpy(cb.mvp, &worldViewProj, sizeof(cb.mvp));
        PushConstants(cb);

        cl->IASetVertexBuffers(0, 1, &renderObject.mesh.vbView);
        cl->IASetIndexBuffer(&renderObject.mesh.ibView);
        cl->DrawIndexedInstanced(renderObject.mesh.indexCount, 1, 0, 0, 0);
    }

    cl->RSSetViewports(1, &viewport);
    cl->RSSetScissorRects(1, &scissor);
}

void Dx12Renderer::PassGBuffer(const RenderContext& ctx)
{
    if (m_useIndirect && m_indirectDrawCount > 0)
    {
        PassGBufferIndirect(ctx);
        return;
    }

    auto* cl = ctx.cmdList;
    const XMFLOAT3& camF = ctx.cameraPos;

    m_gbuffer.ToRenderTargets(cl);

    auto dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs = m_gbuffer.RtvHandles();
    cl->OMSetRenderTargets(GBuffer::kCount, rtvs, FALSE, &dsv);

    const float zero[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < GBuffer::kCount; i++)
    {
        cl->ClearRenderTargetView(rtvs[i], zero, 0, nullptr);
    }
    cl->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

    cl->SetGraphicsRootSignature(gbufferRootSig.Get());
    cl->SetPipelineState(
        gbufferPsoVariant[m_wireframe ? 1 : 0][m_backfaceCull ? 1 : 0].Get());

    for (auto& renderObject : m_renderObjects)
    {
        XMMATRIX world = renderObject.transform;
        XMMATRIX mvp = MathSimd::TransposedMvp(world, ctx.viewProj);
        XMMATRIX transposedWorld  = XMMatrixTranspose(world);

        CBData cb{};
        memcpy(cb.mvp, &mvp, sizeof(cb.mvp));
        memcpy(cb.world, &transposedWorld, sizeof(cb.world));
        cb.cameraPosSpecPower[0] = camF.x;
        cb.cameraPosSpecPower[1] = camF.y;
        cb.cameraPosSpecPower[2] = camF.z;

        const Material& material = m_materialManager.Get(renderObject.materialID);
        cb.specColor[0] = material.metallicFactor;
        cb.specColor[1] = material.roughnessFactor;
        cb.specColor[2] = material.aoFactor;

        PushConstants(cb);

        D3D12_GPU_DESCRIPTOR_HANDLE texHandle =
            srvHeap->GetGPUDescriptorHandleForHeapStart();
        texHandle.ptr += (size_t)renderObject.textureIndex * srvDescriptorSize;
        cl->SetGraphicsRootDescriptorTable(1, texHandle);

        cl->IASetVertexBuffers(0, 1, &renderObject.mesh.vbView);
        cl->IASetIndexBuffer(&renderObject.mesh.ibView);
        cl->DrawIndexedInstanced(renderObject.mesh.indexCount, 1, 0, 0, 0);
    }
}

void Dx12Renderer::PassLighting(const RenderContext& ctx)
{
    auto* cl = ctx.cmdList;
    const XMFLOAT3& camF = ctx.cameraPos;

    m_gbuffer.ToShaderResources(cl);
    if (m_shadowsEnabled)
    {
        m_shadowMap.ToShaderResource(cl);
    }

    m_hdr.ToRenderTarget(cl);
    auto hdrRtv = m_hdr.Rtv();
    cl->OMSetRenderTargets(1, &hdrRtv, FALSE, nullptr);

    cl->SetGraphicsRootSignature(lightingRootSig.Get());

    CBData cb{};
    XMMATRIX lvp = XMMatrixTranspose(m_lightViewProj);
    memcpy(cb.mvp, &lvp, sizeof(cb.mvp));

    XMMATRIX viewProj = ctx.viewProj;
    XMMATRIX invVP = XMMatrixTranspose(XMMatrixInverse(nullptr, viewProj));
    memcpy(cb.world, &invVP, sizeof(cb.world));

    XMFLOAT3 sun{ 0.5f, 1.0f, 0.4f };
    for (const Light& lt : m_lightManager.GetLights())
    {
        if (lt.type == (int)LightType::Directional)
        {
            sun = lt.direction;
            break;
        }
    }
    cb.lightPosRadius[0][0] = sun.x;
    cb.lightPosRadius[0][1] = sun.y;
    cb.lightPosRadius[0][2] = sun.z;

    cb.cameraPosSpecPower[0] = camF.x;
    cb.cameraPosSpecPower[1] = camF.y;
    cb.cameraPosSpecPower[2] = camF.z;
    cb.cameraPosSpecPower[3] = float(m_debugView);
    cb.specColor[0] = m_shadowsEnabled ? 1.0f : 0.0f;
    cb.specColor[1] = float(m_lightCount);
    cb.specColor[2] = m_iblIntensity;
    cb.specColor[3] = m_nightFactor;
    cb.ambient      = m_iblEnabled ? 1.0f : 0.0f;
    cb.pad[0]       = m_ssao.enabled ? 1.0f : 0.0f;
    PushConstants(cb);
    cl->SetGraphicsRootDescriptorTable(1, m_gbuffer.SrvTableStart());

    SsaoTarget& ao = (m_ssao.enabled && !m_ssao.blur) ? m_ssaoRaw : m_ssaoBlur;
    ao.ToShaderResource(cl);
    cl->SetGraphicsRootDescriptorTable(2, ao.Srv());

    cl->SetPipelineState(lightingPso.Get());
    cl->DrawInstanced(3, 1, 0, 0);
}

void Dx12Renderer::PassRayReconstruction(const RenderContext& ctx)
{
    auto* cl = ctx.cmdList;

    m_hdr.ToShaderResource(cl);
    m_ssr.ToRenderTarget(cl);
    auto rtv = m_ssr.Rtv();
    cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    cl->SetGraphicsRootSignature(ssrRootSig.Get());

    CBData cb{};
    XMMATRIX vp = XMMatrixTranspose(ctx.viewProj);
    memcpy(cb.mvp, &vp, sizeof(cb.mvp));
    cb.cameraPosSpecPower[0] = ctx.cameraPos.x;
    cb.cameraPosSpecPower[1] = ctx.cameraPos.y;
    cb.cameraPosSpecPower[2] = ctx.cameraPos.z;
    cb.cameraPosSpecPower[3] = float(m_debugView);
    float step = (m_ray.stepSize > 0.0f) ? m_ray.stepSize
                                         : (m_sceneRadius > 1.0f ? m_sceneRadius : 50.0f) * 0.01f;
    cb.specColor[0] = m_ray.enabled ? 1.0f : 0.0f;
    cb.specColor[1] = step;
    cb.specColor[2] = float(m_ray.maxSteps);
    cb.specColor[3] = m_ray.intensity;
    PushConstants(cb);

    cl->SetGraphicsRootDescriptorTable(1, m_hdr.Srv());

    cl->SetPipelineState(ssrPso.Get());
    cl->DrawInstanced(3, 1, 0, 0);
}

void Dx12Renderer::PassToneMapping(const RenderContext& ctx)
{
    auto* cl = ctx.cmdList;

    HdrTarget& src = m_taa.enabled
        ? m_taaHistory[1 - m_taaWrite]
        : (m_fog.enabled ? m_fogTarget
                         : (m_camera.dofEnabled ? m_hdr : m_ssr));
    src.ToShaderResource(cl);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backBuffers[m_currentFrame].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    cl->ResourceBarrier(1, &barrier);

    auto rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += m_currentFrame * rtvDescriptorSize;
    cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    cl->SetGraphicsRootSignature(tonemapRootSig.Get());

    CBData cb{};
    cb.cameraPosSpecPower[0] = m_camera.GetExposure();
    cb.cameraPosSpecPower[1] = float(m_toneMapOp);
    PushConstants(cb);
    cl->SetGraphicsRootDescriptorTable(1, src.Srv());

    cl->SetPipelineState(tonemapPso.Get());
    cl->DrawInstanced(3, 1, 0, 0);
}

void Dx12Renderer::PassGizmo(const RenderContext& ctx)
{
    if (!m_showGizmos) { return; }

    auto* cl = ctx.cmdList;

    auto rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += m_currentFrame * rtvDescriptorSize;
    auto dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    cl->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    cl->SetGraphicsRootSignature(gizmoRootSig.Get());
    cl->SetPipelineState(gizmoPso.Get());
    cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const XMMATRIX viewProj = ctx.viewProj;
    const float gr = GizmoRadius();
    const auto& lights = m_lightManager.GetLights();

    for (int i = 0; i < (int)lights.size(); i++)
    {
        const Light& lt = lights[i];
        if (lt.type == (int)LightType::Directional) { continue; }
        if (!lt.enabled) { continue; }

        XMMATRIX world = XMMatrixScaling(gr * 2, gr * 2, gr * 2) *
                         XMMatrixTranslation(lt.position.x, lt.position.y, lt.position.z);

        CBData cb{};
        XMMATRIX mvp = XMMatrixTranspose(world * viewProj);
        memcpy(cb.mvp, &mvp, sizeof(cb.mvp));

        if (i == m_selectedLight)
        { cb.specColor[0] = 1.0f; cb.specColor[1] = 0.6f; cb.specColor[2] = 0.05f; }
        else
        { cb.specColor[0] = lt.color.x; cb.specColor[1] = lt.color.y; cb.specColor[2] = lt.color.z; }

        PushConstants(cb);
        cl->IASetVertexBuffers(0, 1, &m_gizmoMesh.vbView);
        cl->IASetIndexBuffer(&m_gizmoMesh.ibView);
        cl->DrawIndexedInstanced(m_gizmoMesh.indexCount, 1, 0, 0, 0);
    }
}

void Dx12Renderer::PassUI(const RenderContext& ctx)
{
    m_imgui.EndFrame(ctx.cmdList);
}

void Dx12Renderer::DrawDebugUI()
{
    using namespace DirectX;

    const ImGuiCond once = ImGuiCond_FirstUseEver;

    ImGui::SetNextWindowPos(ImVec2(20, 20), once);
    ImGui::SetNextWindowSize(ImVec2(340, 470), once);
    ImGui::Begin("Cinematic Camera");

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat3("Position", m_camera.PtrPosition(), -3000.0f, 3000.0f);
        ImGui::SliderFloat("Yaw",   m_camera.PtrYaw(),   -XM_PI, XM_PI);
        ImGui::SliderFloat("Pitch", m_camera.PtrPitch(), -XM_PIDIV2, XM_PIDIV2);
        ImGui::SliderFloat("Speed", m_camera.PtrSpeed(), 0.1f, 2000.0f);
        ImGui::Checkbox("Smooth Motion (inertia)", &m_camera.smoothing);
        if (ImGui::Button("Shake"))
        {
            m_camera.Shake(1.0f);
        }
        ImGui::SameLine(); ImGui::TextDisabled("(explosions / impacts)");
    }

    if (ImGui::CollapsingHeader("Lens", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Focal Length (mm)", &m_camera.focalLength, 14.0f, 200.0f);
        ImGui::SliderFloat("Sensor (mm)",       &m_camera.sensorSize,  8.0f, 36.0f);
        ImGui::Text("FOV: %.1f deg", XMConvertToDegrees(m_camera.PhysicalFovY()));
    }

    if (ImGui::CollapsingHeader("Exposure", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Aperture (f)",   &m_camera.aperture, 1.0f, 22.0f);
        ImGui::SliderFloat("Shutter (s)",    &m_camera.shutterSpeed, 1.0f/2000.0f, 1.0f/15.0f, "%.4f");
        ImGui::SliderFloat("ISO",            &m_camera.ISO,      50.0f, 6400.0f);
        ImGui::SliderFloat("Exposure Comp",  &m_camera.exposureComp, 0.1f, 5.0f);
        ImGui::Text("Exposure: %.3f", m_camera.GetExposure());
    }

    if (ImGui::CollapsingHeader("Depth of Field", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable DOF", &m_camera.dofEnabled);
        ImGui::Checkbox("Auto Focus (centre)", &m_camera.autoFocus);
        ImGui::SliderFloat("Focus Distance", &m_camera.focusDistance, 1.0f, 3000.0f);
        ImGui::SliderFloat("DOF Strength",   &m_camera.depthOfFieldStrength, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Wide Shot"))
        {
            m_camera.ApplyPreset(CameraPreset::WideShot);
        }
        ImGui::SameLine();
        if (ImGui::Button("Close-Up"))
        {
            m_camera.ApplyPreset(CameraPreset::CloseUp);
        }
        ImGui::SameLine();
        if (ImGui::Button("Landscape"))
        {
            m_camera.ApplyPreset(CameraPreset::Landscape);
        }
    }

    if (ImGui::CollapsingHeader("Framing (Part I)"))
    {
        ImGui::Checkbox("Rule of Thirds", &m_showThirds);
        ImGui::Checkbox("Centre Crosshair", &m_showCrosshair);
        ImGui::Checkbox("Safe Frame", &m_showSafeFrame);
    }
    ImGui::End();

    DrawFramingOverlay();

    ImGui::SetNextWindowPos(ImVec2(20, 185), once);
    ImGui::SetNextWindowSize(ImVec2(340, 420), once);
    ImGui::Begin("Lighting");
    ImGui::Text("Lights: %d / %d", m_lightManager.Count(), LightManager::kMaxLights);
    ImGui::TextDisabled("Click a sphere in the scene to select.");
    if (ImGui::Button("+ Add Point Light"))
    {
        Light p;
        p.type = (int)LightType::Point;
        p.position = m_sceneCenter;
        p.radius = (m_sceneRadius > 1.0f ? m_sceneRadius : 10.0f) * 0.3f;
        p.color = { 1, 1, 1 };
        p.intensity = 1.0f;
        m_selectedLight = m_lightManager.AddLight(p);
    }

    if (m_selectedLight >= 0 && m_selectedLight < m_lightManager.Count())
    {
        Light& selectedLight = m_lightManager.GetLight(m_selectedLight);
        ImGui::Separator();
        ImGui::Text("Selected: Light %d", m_selectedLight);
        ImGui::SliderFloat3("Pos##sel", &selectedLight.position.x, -3000.0f, 3000.0f);
        ImGui::ColorEdit3("Color##sel", &selectedLight.color.x);
        ImGui::SliderFloat("Intensity##sel", &selectedLight.intensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Radius##sel", &selectedLight.radius, 0.0f, 5000.0f);
        if (ImGui::Button("Deselect")) { m_selectedLight = -1; }
        ImGui::SameLine();
        if (ImGui::Button("Delete Light"))
        {
            m_lightManager.RemoveLight(m_selectedLight);
            m_selectedLight = -1;
        }
    }
    ImGui::Separator();

    const char* typeNames[] = { "Directional", "Point", "Spot" };
    for (int i = 0; i < m_lightManager.Count(); i++)
    {
        Light& lt = m_lightManager.GetLight(i);
        ImGui::PushID(i);
        ImGui::Checkbox("##en", &lt.enabled); ImGui::SameLine();
        ImGui::Text("Light %d (%s)", i, typeNames[lt.type & 3]);
        ImGui::Combo("Type", &lt.type, typeNames, 3);
        if (lt.type == (int)LightType::Directional)
        {
            ImGui::SliderFloat3("Direction", &lt.direction.x, -1.0f, 1.0f);
        }
        else
        {
            ImGui::SliderFloat3("Position", &lt.position.x, -3000.0f, 3000.0f);
            ImGui::SliderFloat("Radius", &lt.radius, 0.0f, 5000.0f);
        }
        ImGui::ColorEdit3("Color", &lt.color.x);
        ImGui::SliderFloat("Intensity", &lt.intensity, 0.0f, 10.0f);
        ImGui::Separator();
        ImGui::PopID();
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(370, 20), once);
    ImGui::SetNextWindowSize(ImVec2(280, 150), once);
    ImGui::Begin("Environment");
    bool isNight = (m_timeOfDay == TimeOfDayState::Night);
    ImGui::Text("Time of Day: %s", isNight ? "Night" : "Day");
    if (ImGui::Checkbox("Night Mode", &isNight))
    {
        m_timeOfDay = isNight ? TimeOfDayState::Night : TimeOfDayState::Day;
        ApplyTimeOfDay();
    }
    if (ImGui::Button("Toggle Day/Night"))
    {
        m_timeOfDay = isNight ? TimeOfDayState::Day : TimeOfDayState::Night;
        ApplyTimeOfDay();
    }
    ImGui::Separator();
    ImGui::SliderFloat("Exposure##env", &m_camera.exposureComp, 0.1f, 5.0f);
    ImGui::SliderFloat("IBL Intensity##env", &m_iblIntensity, 0.0f, 3.0f);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(370, 180), once);
    ImGui::SetNextWindowSize(ImVec2(280, 200), once);
    ImGui::Begin("SSAO");
    ImGui::Checkbox("Enable SSAO", &m_ssao.enabled);
    ImGui::Checkbox("Blur (edge-aware)", &m_ssao.blur);
    ImGui::SliderFloat("Radius",    &m_ssao.radius,    0.2f, 1.5f);
    ImGui::SliderFloat("Bias",      &m_ssao.bias,      0.01f, 0.1f);
    ImGui::SliderFloat("Intensity", &m_ssao.intensity, 0.5f, 2.0f);
    ImGui::SliderInt("Kernel Size", &m_ssao.kernelSize, 16, 64);
    ImGui::Separator();
    if (ImGui::Button("View: Final")) { m_debugView = 0; }
    ImGui::SameLine();
    if (ImGui::Button("View: SSAO")) { m_debugView = 9; }
    ImGui::TextDisabled("Toggle Blur to compare raw/blurred.");
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(370, 390), once);
    ImGui::SetNextWindowSize(ImVec2(280, 230), once);
    ImGui::Begin("Volumetric Fog");
    ImGui::Checkbox("Enable Fog", &m_fog.enabled);
    ImGui::SliderFloat("Density",        &m_fog.density,      0.01f, 0.2f);
    ImGui::SliderFloat("Height Falloff", &m_fog.heightFactor, 0.1f, 2.0f);
    ImGui::SliderFloat("Anisotropy",     &m_fog.anisotropy,   0.0f, 0.9f);
    ImGui::SliderInt("Step Count",       &m_fog.stepCount,    16, 128);
    ImGui::SliderFloat("Max Distance",   &m_fog.maxDistance,  10.0f, 200.0f);
    ImGui::SliderFloat("Ambient",        &m_fog.ambientIntensity, 0.0f, 1.0f);
    ImGui::ColorEdit3("Fog Color",       &m_fog.fogColor.x);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(370, 630), once);
    ImGui::SetNextWindowSize(ImVec2(280, 150), once);
    ImGui::Begin("TAA");
    if (ImGui::Checkbox("Enable TAA", &m_taa.enabled)) { m_taaHistoryValid = false; }
    ImGui::SliderFloat("Blend Factor", &m_taa.blend, 0.02f, 0.3f);
    const char* taaDbg[] = { "Off", "Motion Vectors", "History" };
    ImGui::Combo("Debug View", &m_taa.debug, taaDbg, 3);
    if (ImGui::Button("Reset History")) { m_taaHistoryValid = false; }
    ImGui::TextDisabled("Lower blend = steadier, higher = sharper.");
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(float(m_width) - 250.0f, 350.0f), once);
    ImGui::SetNextWindowSize(ImVec2(240, 150), once);
    ImGui::Begin("Assets");
    const Assets::AssetStats as = m_assets.Stats();
    ImGui::Text("Models:   %u", as.models);
    ImGui::Text("Textures: %u", as.textures);
    ImGui::Separator();
    ImGui::Text("Loaded:  %u", as.loaded);
    ImGui::Text("Loading: %u", as.loading);
    ImGui::Text("Failed:  %u", as.failed);
    ImGui::Text("Queued jobs: %u", as.queued);
    ImGui::Separator();
    ImGui::Text("GPU textures: %u", as.gpuTextures);
    ImGui::Text("CPU in-flight: %.1f KB", as.cpuBytesInFlight / 1024.0);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(20, 615), once);
    ImGui::SetNextWindowSize(ImVec2(340, 130), once);
    ImGui::Begin("Material (PBR)");
    ImGui::Checkbox("Override all materials", &m_materialOverride);
    ImGui::SliderFloat("Metallic",  &m_overrideMetallic,  0.0f, 1.0f);
    ImGui::SliderFloat("Roughness", &m_overrideRoughness, 0.04f, 1.0f);
    if (m_materialOverride)
    {
        for (uint32_t mi = 0; mi < m_materialManager.Count(); mi++)
        {
            Material& mm = m_materialManager.Get(mi);
            mm.metallicFactor  = m_overrideMetallic;
            mm.roughnessFactor = m_overrideRoughness;
        }
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(float(m_width) - 250.0f, 20.0f), once);
    ImGui::SetNextWindowSize(ImVec2(240, 320), once);
    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f", m_fps);
    ImGui::Text("CPU record: %.3f ms", m_cpuMs);
    ImGui::Text("Objects: %d", (int)m_renderObjects.size());
    const int geomCalls = (m_useIndirect && m_indirectDrawCount > 0)
                          ? 1 : (int)m_renderObjects.size();
    ImGui::Text("Draw Calls: %d", geomCalls + 1);
    ImGui::Text("Textures: %u", srvCount);
    ImGui::Separator();
    ImGui::Text("GPU-Driven Rendering:");
    ImGui::Checkbox("Enable Indirect Rendering", &m_useIndirect);
    ImGui::Text("Total Instances:   %u", m_totalInstances);
    ImGui::Text("Visible Instances: %u", m_visibleInstances);
    ImGui::TextDisabled(m_useIndirect ? "1 ExecuteIndirect (GPU-driven)"
                                      : "%d CPU draw calls", (int)m_renderObjects.size());
    ImGui::Separator();
    ImGui::Checkbox("Wireframe", &m_wireframe);
    ImGui::Checkbox("Back-face Cull", &m_backfaceCull);
    ImGui::Separator();
    ImGui::Text("G-Buffer View:");
    const char* modes[] = { "Lit (final)", "Position", "Normal", "Albedo",
                            "Metallic", "Roughness", "IBL Irradiance",
                            "Ray Hits", "Reflection Buffer", "SSAO",
                            "Shadow Map" };
    ImGui::Combo("##gbufview", &m_debugView, modes, 11);
    ImGui::Separator();
    ImGui::Text("Image-Based Lighting:");
    ImGui::Checkbox("Enable IBL", &m_iblEnabled);
    ImGui::SliderFloat("IBL Intensity", &m_iblIntensity, 0.0f, 3.0f);
    ImGui::Separator();
    ImGui::Text("Ray Reconstruction (SSR):");
    ImGui::Checkbox("Enable SSR", &m_ray.enabled);
    ImGui::SliderFloat("Ray Step Size", &m_ray.stepSize, 0.0f, 50.0f);
    ImGui::SliderInt("Max Steps", &m_ray.maxSteps, 8, 128);
    ImGui::SliderFloat("Reflection Intensity", &m_ray.intensity, 0.0f, 2.0f);
    ImGui::Separator();
    ImGui::Checkbox("Shadows", &m_shadowsEnabled);
    ImGui::Checkbox("Show Light Gizmos", &m_showGizmos);
    ImGui::Separator();
    ImGui::Text("HDR Tone Mapping:");
    const char* tm[] = { "None", "Reinhard", "ACES", "AgX" };
    ImGui::Combo("Operator", &m_toneMapOp, tm, 4);
    ImGui::SliderFloat("Exposure", &m_camera.exposureComp, 0.1f, 5.0f);
    ImGui::TextDisabled("F1: toggle UI");
    ImGui::End();
}

void Dx12Renderer::DrawFramingOverlay()
{
    if (!m_showThirds && !m_showCrosshair && !m_showSafeFrame) { return; }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const float width = float(m_width), height = float(m_height);
    const ImU32 line  = IM_COL32(255, 255, 255, 90);
    const ImU32 cross = IM_COL32(255, 80, 80, 180);

    if (m_showThirds)
    {
        for (int i = 1; i < 3; i++)
        {
            const float lineX = width * i / 3.0f, lineY = height * i / 3.0f;
            drawList->AddLine(ImVec2(lineX, 0), ImVec2(lineX, height), line);
            drawList->AddLine(ImVec2(0, lineY), ImVec2(width, lineY), line);
        }
    }

    if (m_showSafeFrame)
    {
        const float actionX = width * 0.05f, actionY = height * 0.05f;
        drawList->AddRect(ImVec2(actionX, actionY), ImVec2(width - actionX, height - actionY), line);
        const float titleX = width * 0.10f, titleY = height * 0.10f;
        drawList->AddRect(ImVec2(titleX, titleY), ImVec2(width - titleX, height - titleY), line);
    }

    if (m_showCrosshair)
    {
        const float centerX = width * 0.5f, centerY = height * 0.5f, crosshairSize = 12.0f;
        drawList->AddLine(ImVec2(centerX - crosshairSize, centerY), ImVec2(centerX + crosshairSize, centerY), cross, 1.5f);
        drawList->AddLine(ImVec2(centerX, centerY - crosshairSize), ImVec2(centerX, centerY + crosshairSize), cross, 1.5f);
    }
}

void Dx12Renderer::WaitForGpu()
{
    ++fenceValue;
    queue->Signal(fence.Get(), fenceValue);

    if (fence->GetCompletedValue() < fenceValue)
    {
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

void Dx12Renderer::Shutdown()
{
    m_assets.Shutdown();
    if (queue && fence)
    {
        WaitForGpu();
    }
    m_imgui.Shutdown();
    if (constantBuffer && cbMapped)
    {
        constantBuffer->Unmap(0, nullptr);
        cbMapped = nullptr;
    }
    if (fenceEvent)
    {
        CloseHandle(fenceEvent);
        fenceEvent = nullptr;
    }
}
