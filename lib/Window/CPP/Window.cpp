module;
#include <windows.h>
#include <iostream>
#include <concepts>

module bf.Window;
struct VECTOR2i {
	int x;
	int y;
};

extern "C" {
	HWND InitWindowHandle(uint32_t width, uint32_t height);
	void InitWindowValue(const int width, const int height);
	void InitWindowRef(const int& width, const int& height);
	bool IsWindowOpen();
	void ProcessMessages();
	void DrawWindow();
	void SetWindowName(const wchar_t* newName);
	void SetWindowPosition(const VECTOR2i* pos);
	HWND GetWindowHandle();
}

bf::Window::Window(const Window& newWindow) noexcept
{
	const wchar_t* src = newWindow.m_name;
	if (src) {
		size_t len = wcslen(src) + 1;
		m_name = new wchar_t[len];
		std::wmemcpy(const_cast<wchar_t*>(m_name), src, len);
	}
	else {
		m_name = nullptr;
	}
}

bf::Window::Window(Window&& newWindow) noexcept
{
	m_name = newWindow.m_name;
	newWindow.m_name = nullptr;
}

bf::Window::~Window()
{
	if (m_name != nullptr)
	{
		delete m_name;
		m_name = nullptr;
	}
}

bf::Window& bf::Window::operator=(const Window& newWindow) noexcept
{
	if (this != &newWindow)
	{
		delete[] m_name;
		m_name = nullptr;
		const wchar_t* src = newWindow.m_name;
		if (src) {
			size_t len = wcslen(src) + 1;
			m_name = new wchar_t[len];
			std::wmemcpy(const_cast<wchar_t*>(m_name), src, len);
		}
		else {
			m_name = nullptr;
		}
	}
	return *this;
}

bf::Window& bf::Window::operator=(Window&& newWindow) noexcept
{
	if (this != &newWindow)
	{
		delete[] m_name;

		m_name = newWindow.m_name;
		newWindow.m_name = nullptr;
	}
	return *this;
}

void bf::Window::Create(uint32_t width, uint32_t height, const char* name)
{
	windowHandle = InitWindowHandle(width, height);
	if (m_name == nullptr)
	{
		SetName(name);
	}
}

bool bf::Window::IsOpen()
{
	return IsWindowOpen() != false;
}

void bf::Window::PollEvents()
{
	ProcessMessages();
}

void bf::Window::WindowDraw()
{
	DrawWindow();
}

void bf::Window::SetPosition(const int x, const int y) noexcept
{
	VECTOR2i pos = { .x = x, .y = y };
	SetWindowPosition(&pos);
}
/*
* TODO : Move this into helper function
*/
const wchar_t* ConvertToConstWChar(const char& name)
{
	if (!name) { return nullptr; }

	size_t length = strlen(&name) + 1;
	wchar_t* newWideChar = new wchar_t[length];

	size_t converted = 0;
	errno_t err = mbstowcs_s(&converted, newWideChar, length, &name, _TRUNCATE);

	if (err != 0) {
		delete[] newWideChar;
		return nullptr;
	}

	return newWideChar;
}

void bf::Window::SetName(const char* name)
{
	if (name != nullptr)
	{
		m_name = ConvertToConstWChar(*name);
		SetWindowName(m_name);
	}
}