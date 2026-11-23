#include <Windows.h>
#include <Window.h>

//ask hybrid-GPU laptops to use the dedicated GPU instead of the integrated one
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	Window win;
	win.create("My Window", 1024, 1024);
	while (true)
	{
		win.processMessages();
	}

	return 0;
}