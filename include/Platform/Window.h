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

	HWND hwnd;
	HINSTANCE hinstance;

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void updateMouse(int x, int y);
	void create(const std::string window_name, int window_width, int window_height);
	void processMessages();
};