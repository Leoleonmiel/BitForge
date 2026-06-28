import bf.Window;
import bf.Preset.Session;
#include "Dx12Renderer.h"
#include "Core/Timer.h"
#include "Input/Input.h"


extern "C" { __declspec(dllimport) int GetSystemMetricsForDpi(int, unsigned); }

static void CenterWindow(bf::Window& window, int width, int height)
{
    constexpr char DPI_UNAWARE = 96;
    const int posX = (GetSystemMetricsForDpi(SM_CXSCREEN, DPI_UNAWARE) - width) / 2;
    const int posY = (GetSystemMetricsForDpi(SM_CYSCREEN, DPI_UNAWARE) - height) / 2;
    window.SetPosition(posX, posY);
}

static void LoadScene(Dx12Renderer& renderer, const char* path)
{
    for (auto& renderObject : renderer.LoadGLTFScene(path))
    {
        renderer.AddRenderObject(renderObject);
    }
}

int main()
{
    constexpr int width = 1920;
    constexpr int height = 1080;

    bf::Window window;
    Dx12Renderer renderer;

    CenterWindow(window, width, height);
    window.Create(width, height, "BitForge");
    renderer.Initialize(window.Handle(), width, height);
    Input::Initialize(window.Handle());

    LoadScene(renderer, "Assets/Sponza/sponza.gltf");

    bf::preset::PresetSession presets(renderer);
    renderer.SetUICallback([&presets] { presets.DrawPanel(); });

    Timer timer;
    while (window.IsOpen())
    {
        window.PollEvents();
        Input::Update();
        if (Input::IsKeyDown(VK_ESCAPE)) { break; }
        presets.HandleHotkeys();
        timer.Tick();
        renderer.Render(timer.DeltaTime());
    }

    renderer.Shutdown();
    return 0;
}
