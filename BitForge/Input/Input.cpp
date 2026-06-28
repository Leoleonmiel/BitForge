#include "Input.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace
{
    HWND  g_hwnd        = nullptr;
    float g_mouseDX     = 0.0f;
    float g_mouseDY     = 0.0f;
    int   g_mouseX      = 0;
    int   g_mouseY      = 0;
    bool  g_cursorLocked = false;
}

static POINT WindowCenter()
{
    RECT clientRect{};
    GetClientRect(g_hwnd, &clientRect);
    POINT center{ (clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2 };
    ClientToScreen(g_hwnd, &center);
    return center;
}

void Input::Initialize(void* hwnd)
{
    g_hwnd        = static_cast<HWND>(hwnd);
    g_mouseDX     = 0.0f;
    g_mouseDY     = 0.0f;
    g_cursorLocked = false;
}

void Input::Update()
{
    g_mouseDX = 0.0f;
    g_mouseDY = 0.0f;

    {
        POINT cursorPos{};
        GetCursorPos(&cursorPos);
        if (g_hwnd)
        {
            ScreenToClient(g_hwnd, &cursorPos);
        }
        g_mouseX = cursorPos.x;
        g_mouseY = cursorPos.y;
    }

    const bool rmbHeld = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    if (rmbHeld)
    {
        if (!g_cursorLocked)
        {
            ShowCursor(FALSE);
            g_cursorLocked = true;
            POINT center = WindowCenter();
            SetCursorPos(center.x, center.y);
            return;
        }

        POINT centre = WindowCenter();
        POINT cur{};
        GetCursorPos(&cur);

        g_mouseDX = float(cur.x - centre.x);
        g_mouseDY = float(cur.y - centre.y);

        SetCursorPos(centre.x, centre.y);
    }
    else
    {
        if (g_cursorLocked)
        {
            ShowCursor(TRUE);
            g_cursorLocked = false;
        }
    }
}

bool Input::IsKeyDown(int key)
{
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

bool Input::IsMouseDown(int vkButton)
{
    return (GetAsyncKeyState(vkButton) & 0x8000) != 0;
}

float Input::GetMouseDeltaX() { return g_mouseDX; }
float Input::GetMouseDeltaY() { return g_mouseDY; }
int   Input::GetMouseX() { return g_mouseX; }
int   Input::GetMouseY() { return g_mouseY; }
