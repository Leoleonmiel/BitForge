module;
#include <concepts>
#include <windows.h>
export module bf.Window;

export namespace bf {

    class Window {
    public:
        Window() = default;
        Window(const Window& newWindow) noexcept;
        Window(Window&& newWindow) noexcept;
        ~Window();
        Window& operator=(const Window& newWindow) noexcept;
        Window& operator=(Window&& newWindow) noexcept;

        void Create(unsigned int width, unsigned int height, const char* name = "MyWindow");
        [[nodiscard]] bool IsOpen();
        void PollEvents();
        void WindowDraw();
        void SetPosition(const int x, const int y) noexcept;
        void SetName(const char* name);

        [[nodiscard]]constexpr HWND Handle() const noexcept{ return windowHandle;}
    private :
        const wchar_t* m_name{ nullptr };
        HWND windowHandle{ nullptr };
        
    };
}