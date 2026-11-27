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

	HWND hwnd;
	HINSTANCE hinstance;

	bool shouldClose = false;
	bool focused = false; //false while alt+tabbed or minimised

	//keyboard
	bool keys[256] = {};        //held down right now
	bool keysPressed[256] = {}; //only true on the frame the key went down (ignores key repeat)
	bool keyPressed(int key) const { return keysPressed[key]; }

	//mouse
	int mousex = 0; //cursor position in client pixels, for the ui later
	int mousey = 0;
	bool mouseButtons[3] = {};
	int leftClicks = 0;  //clicks since the last processMessages(), so fast clicks don't get lost between frames
	int mouseDeltaX = 0; //raw mouse movement since the last processMessages(), used for aiming
	int mouseDeltaY = 0;

	//when locked the cursor is hidden and kept inside the window
	//the game decides when it's allowed (menus = free cursor, playing = locked)
	//alt+tab releases it, clicking back in locks it again if it's still allowed
	bool allowMouseLock = true;
	bool mouseLocked = false;
	void lockMouse(bool lock);

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void create(const std::string window_name, int window_width, int window_height);
	void processMessages(); //call once at the start of every frame
	void setTitle(const std::string& title);
	void updateMouse(int x, int y);
};
