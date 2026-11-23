#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <vector>
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler.lib")

class Core
{
private:


public:
	IDXGIAdapter1* adapter;

	void init(HWND hwnd, int _width, int _height);
};