#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Win32/Win32Window.h"
#include "Platform/Win32/Win32Application.h"

namespace LambdaEngine
{
	Win32Window::Win32Window()
		: m_hWnd(0)
	{
	}

	bool Win32Window::Init(uint32 width, uint32 height)
	{
		DWORD	dwStyle	= WS_OVERLAPPEDWINDOW;
		RECT	clientRect = { 0, 0, LONG(width), LONG(height) };
		::AdjustWindowRect(&clientRect, dwStyle, FALSE);

		INT nWidth	= clientRect.right - clientRect.left;
		INT nHeight = clientRect.bottom - clientRect.top;
		m_hWnd = ::CreateWindowEx(0, WINDOW_CLASS, L"Lambda Game Engine", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, NULL, NULL, Win32Application::GetInstanceHandle(), NULL);
		if (m_hWnd == NULL)
		{
			//TODO: Log this
			return false;
		}

		return true;
	}

	void Win32Window::Show()
	{
		::ShowWindow(m_hWnd, SW_NORMAL);
	}

	void Win32Window::Release()
	{
		::DestroyWindow(m_hWnd);
	}
}

#endif