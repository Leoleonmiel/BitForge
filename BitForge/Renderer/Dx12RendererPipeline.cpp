#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include <d3dcompiler.h>

using namespace DirectX;

ComPtr<ID3DBlob> Dx12Renderer::LoadShader(const std::wstring& path,
                                          const std::string& entry,
                                          const std::string& target)
{
    std::wstring fullPath = ExeDirW() + path;

    ComPtr<ID3DBlob> blob, errors;
    HRESULT hr = D3DCompileFromFile(
        fullPath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry.c_str(),
        target.c_str(),
        0, 0,
        &blob, &errors);

    if (FAILED(hr))
    {
        if (errors)
        {
            OutputDebugStringA((const char*)errors->GetBufferPointer());
        }
        __debugbreak();
    }
    return blob;
}
