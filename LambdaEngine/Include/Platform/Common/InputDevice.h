#pragma once

#include <vector>

#include "Input/InputCodes.h"
#include "IApplicationMessageHandler.h"

namespace LambdaEngine
{
	class IKeyboardHandler;
	class IMouseHandler;

	struct KeyboardState
	{
		bool KeyStates[EKey::KEY_LAST];

		bool IsKeyDown(EKey key)
		{
			return KeyStates[key];
		}

		bool IsKeyUp(EKey key)
		{
			return !KeyStates[key];
		}
	};

	struct MouseState
	{
		int32 X;
		int32 Y;
		int32 Scroll;

		bool ButtonStates[EMouseButton::MOUSE_BUTTON_COUNT];

		bool IsButtonPressed(EMouseButton button)
		{
			return ButtonStates[button];
		}

		bool IsButtonReleased(EMouseButton button)
		{
			return !ButtonStates[button];
		}
	};

	class InputDevice : public IApplicationMessageHandler
	{
	public:
		DECL_ABSTRACT_CLASS(InputDevice);

		KeyboardState   GetKeyboardState()	{ return m_KeyboardState; }
		MouseState      GetMouseState()		{ return m_MouseState; }

		void OnKeyDown(EKey key);
		void OnKeyUp(EKey key);

		void OnMouseMove(int32 x, int32 y);
		void OnMouseButtonPressed(EMouseButton button);
		void OnMouseButtonReleased(EMouseButton button);
		void OnMouseScrolled(int32 delta);

		void AddKeyboardHandler(IKeyboardHandler* pHandler);
		void AddMouseHandler(IMouseHandler* pHandler);

		void RemoveKeyboardHandler(IKeyboardHandler* pHandler);
		void RemoveMouseHandler(IMouseHandler* pHandler);

	private:
		KeyboardState	m_KeyboardState;
		MouseState		m_MouseState;

		std::vector<IKeyboardHandler*>	m_KeyboardHandlers;
		std::vector<IMouseHandler*>		m_MouseHandlers;

	};
}