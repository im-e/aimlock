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
	
	//raw input device for the mouse
	RAWINPUTDEVICE rid = {};
	rid.usUsagePage = 0x01; 
	rid.usUsage = 0x02; 
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
			window->shouldClose = true; //ask the loop to stop for safe exit
			return 0;
		}
		case WM_KEYDOWN:
		{
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
			return 0;
		}
		case WM_LBUTTONUP:
		{
			window->updateMouse(WINDOW_GET_X_LPARAM(lParam), WINDOW_GET_Y_LPARAM(lParam));
			window->mouseButtons[0] = false;
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
		case WM_ACTIVATE:
		{
			//this can arrive while CreateWindowEx is still running, before the hwnd member is set
			window->hwnd = hwnd;
			bool active = LOWORD(wParam) != WA_INACTIVE && !HIWORD(wParam); //HIWORD = minimised
			if (!active)
			{
				//forget held keys, otherwise W stays "down" if you alt+tab while walking
				memset(window->keys, 0, sizeof(window->keys));
				memset(window->mouseButtons, 0, sizeof(window->mouseButtons));
			}
			window->lockMouse(active);
			return 0;
		}
		case WM_SIZE:
		case WM_MOVE:
		{
			//the window moved or changed size, so the area the cursor is kept in has to follow
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
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	mouseDeltaX = 0;
	mouseDeltaY = 0;

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
		//keep the cursor inside the window's drawable area (in screen coordinates)
		RECT rect;
		GetClientRect(hwnd, &rect);
		MapWindowPoints(hwnd, NULL, (POINT*)&rect, 2);
		ClipCursor(&rect);
		//ShowCursor keeps a counter, so loop until it's actually hidden
		while (ShowCursor(FALSE) >= 0) {}
	}
	else
	{
		ClipCursor(NULL);
		while (ShowCursor(TRUE) < 0) {}
	}
}

void Window::updateMouse(int x, int y) 
{
	mousex = x;
	mousey = y;
}