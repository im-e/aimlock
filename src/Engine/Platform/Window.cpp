#include "Engine/Platform/Window.h"
#include <cstring> //memset

Window* window;

void Window::create(const std::string window_name, int window_width, int window_height) {
	window = this;

	WNDCLASSEX wc;
	hinstance = GetModuleHandle(NULL);
	name = window_name;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	std::wstring wname = std::wstring(name.begin(), name.end());
	wc.lpszClassName = wname.c_str();
	wc.cbSize = sizeof(WNDCLASSEX);
	RegisterClassEx(&wc);

	window_x = 0;
	window_y = 0;

	width = window_width;
	height = window_height;
	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	hwnd = CreateWindowEx(WS_EX_APPWINDOW, wname.c_str(), wname.c_str(), style,
		window_x, window_y, width, height, NULL, NULL, hinstance, this);

	//register the mouse for raw input, so aiming gets the real sensor counts with no windows acceleration
	RAWINPUTDEVICE rid = {};
	rid.usUsagePage = 0x01; //generic desktop
	rid.usUsage = 0x02;     //mouse
	rid.dwFlags = 0;
	rid.hwndTarget = hwnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_CLOSE:
		{
			window->shouldClose = true; //let the main loop finish the frame and shut down properly
			return 0;
		}
		case WM_KEYDOWN:
		{
			//bit 30 is set if the key was already down, this is just windows' key repeat
			bool repeat = (lParam & (1 << 30)) != 0;
			if (!repeat)
				window->keysPressed[(unsigned int)wParam] = true;
			window->keys[(unsigned int)wParam] = true;
			return 0;
		}
		case WM_KEYUP:
		{
			window->keys[(unsigned int)wParam] = false;
			return 0;
		}
		case WM_LBUTTONDOWN:
		{
			window->updateMouse(WINDOW_GET_X_LPARAM(lParam), WINDOW_GET_Y_LPARAM(lParam));
			window->mouseButtons[0] = true;
			window->leftClicks++;
			return 0;
		}
		case WM_LBUTTONUP:
		{
			window->updateMouse(WINDOW_GET_X_LPARAM(lParam), WINDOW_GET_Y_LPARAM(lParam));
			window->mouseButtons[0] = false;
			return 0;
		}
		case WM_RBUTTONDOWN:
		{
			window->mouseButtons[1] = true;
			return 0;
		}
		case WM_RBUTTONUP:
		{
			window->mouseButtons[1] = false;
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			window->updateMouse(WINDOW_GET_X_LPARAM(lParam), WINDOW_GET_Y_LPARAM(lParam));
			return 0;
		}
		case WM_INPUT:
		{
			RAWINPUT raw = {};
			UINT size = sizeof(raw);
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
			if (raw.header.dwType == RIM_TYPEMOUSE && !(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
			{
				window->mouseDeltaX += raw.data.mouse.lLastX;
				window->mouseDeltaY += raw.data.mouse.lLastY;
			}
			return DefWindowProc(hwnd, msg, wParam, lParam); //windows needs this to clean up the input
		}
		case WM_ACTIVATE:
		{
			//this can come in while CreateWindowEx is still running, before the hwnd member is set
			window->hwnd = hwnd;
			bool active = LOWORD(wParam) != WA_INACTIVE && !HIWORD(wParam); //HIWORD = minimised
			window->focused = active;
			if (!active)
			{
				//forget anything held, otherwise a key stays "down" if you alt+tab while holding it
				memset(window->keys, 0, sizeof(window->keys));
				memset(window->mouseButtons, 0, sizeof(window->mouseButtons));
			}
			window->lockMouse(active && window->allowMouseLock);
			return 0;
		}
		case WM_SIZE:
		case WM_MOVE:
		{
			//the window moved or resized, so the cursor clip rect needs to move with it
			if (window->mouseLocked)
			{
				window->mouseLocked = false;
				window->lockMouse(true);
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		default:
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}
}

void Window::processMessages()
{
	//anything "per frame" gets reset here, then the messages below fill it back in
	mouseDeltaX = 0;
	mouseDeltaY = 0;
	leftClicks = 0;
	memset(keysPressed, 0, sizeof(keysPressed));

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void Window::lockMouse(bool lock)
{
	if (lock == mouseLocked)
		return;
	mouseLocked = lock;

	if (lock)
	{
		//clip the cursor to the client area (needs screen coordinates)
		RECT rect;
		GetClientRect(hwnd, &rect);
		MapWindowPoints(hwnd, NULL, (POINT*)&rect, 2);
		ClipCursor(&rect);
		//ShowCursor is a counter rather than an on/off switch, so keep going until it's actually hidden
		while (ShowCursor(FALSE) >= 0) {}
	}
	else
	{
		ClipCursor(NULL);
		while (ShowCursor(TRUE) < 0) {}
	}
}

void Window::setTitle(const std::string& title)
{
	SetWindowTextA(hwnd, title.c_str());
}

void Window::updateMouse(int x, int y)
{
	mousex = x;
	mousey = y;
}
