#pragma once
#include <Windows.h>
#include <string>
#define WINDOW_GET_X_LPARAM(lp) ((int)(short) LOWORD(lp))
#define WINDOW_GET_Y_LPARAM(lp) ((int)(short) HIWORD(lp))

class Window
{
public:
	std::string name;
	int width;
	int height;

	int window_x;
	int window_y;

	bool keys[256] = {};
	int mousex = 0;
	int mousey = 0;
	bool mouseButtons[3] = {};
	bool shouldClose = false;

	int mouseDeltaX = 0; //how far the mouse moved since the last processMessages()
	int mouseDeltaY = 0;

	//fps style: while the window is active the cursor is hidden and kept inside it
	//alt+tab away (or minimise) and it's released, click back in and it locks again
	bool mouseLocked = false;
	void lockMouse(bool lock);

	HWND hwnd;
	HINSTANCE hinstance;

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void updateMouse(int x, int y);
	void create(const std::string window_name, int window_width, int window_height);
	void processMessages();
};