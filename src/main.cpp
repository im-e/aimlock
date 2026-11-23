#include <Windows.h>
#include <Window.h>
#include <Core.h>
#include <Mesh.h>
#include <Shader.h>
#include <PSO.h>

//ask hybrid-GPU laptops to use the dedicated GPU instead of the integrated one
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	Window win;
	win.create("My Window", 1920, 1080);

	int aspectRatio = win.width / win.height;
	
	Core core;
	core.init(win.hwnd, win.width, win.height);

	Vertex vertices[3] =
	{
		{ {  0.0f,  0.5f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },  // top: red
		{ {  0.5f, -0.5f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },  // bottom right: green
		{ { -0.5f, -0.5f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },  // bottom left: blue
	}; 
	Mesh triangle;
	triangle.init(&core, vertices, 3);

	Shader shader;
	shader.load(L"shaders/triangle.hlsl");

	PSO pso;
	pso.init(&core, shader, Mesh::inputLayout());

	while (1) {

		win.processMessages();
		if (win.keys[VK_ESCAPE]) break;

		core.beginFrame();
		pso.bind(&core);
		triangle.draw(&core);
		core.finishFrame();
	}
	core.flushGraphicsQueue();

	return 0;
}