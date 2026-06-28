#pragma once

class Input
{
public:
    static void Initialize(void* hwnd);

    static void Update();

    static bool  IsKeyDown(int key);
    static float GetMouseDeltaX();
    static float GetMouseDeltaY();
    static int   GetMouseX();
    static int   GetMouseY();

    static bool  IsMouseDown(int vkButton);
};
