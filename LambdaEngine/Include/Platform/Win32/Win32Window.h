#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Common/Window.h"
#include "Windows.h"

#define WINDOW_CLASS L"MainWindowClass"

namespace LambdaEngine
{
	class LAMBDA_API Win32Window : public Window
	{
	public:
		Win32Window();
		~Win32Window() = default;

		virtual bool Init(uint32 width, uint32 height) override;
		virtual void Release() override;

		virtual void Show() override;

		virtual void* GetHandle() const { return (void*)m_hWnd; }

	private:
		HWND m_hWnd;
	};
}

#endif