#include <Windows.h>
#include <Platform/Window.h>
#include <Graphics/Core.h>
#include <Graphics/Mesh.h>
#include <Graphics/Shader.h>
#include <Graphics/PSO.h>
#include <cstdio>
#include <chrono>

//ask hybrid-GPU laptops to use the dedicated GPU instead of the integrated one
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	Window win;
	win.create("My Window", 1920, 1080);

	Core core;
	core.init(win.hwnd, win.width, win.height);

	Shader shader;
	if (!shader.load(L"resources/shaders/shader.hlsl"))
		return 1;

	PSO pso;
	pso.init(&core, shader, Mesh::inputLayout());

	//same models and camera settings as the software rasterizer
		struct ModelInfo { const char* path; float radius; float targetY; };
	ModelInfo models[] =
	{
		{ "resources/Models/Bunny.gem",      0.5f,    0.075f },
		{ "resources/Models/Cube.gem",       5.0f,    0.5f },
		{ "resources/Models/TRex.gem",       1800.0f, 290.0f },
		{ "resources/Models/acacia_003.gem", 1200.0f, 265.0f },
	};

	Mesh meshes[4];
	for (int i = 0; i < 4; i++)
	{
		if (!meshes[i].loadGEM(&core, models[i].path))
			OutputDebugStringA("A model failed to load - check the path\n");
	}
	int current = 0;

	float aspect = (float)win.width / (float)win.height;
	Matrix projection = Matrix::perspective(45.0f, aspect, 0.01f, 10000.0f);
	Matrix world; //identity: model stays at the origin

	auto start = std::chrono::steady_clock::now();

	Color clearColor = { 0.1f, 0.1f, 0.12f, 1.0f }; //dark grey

	while (!win.shouldClose)
	{
		win.processMessages();
		if (win.keys[VK_ESCAPE]) break;

		for (int i = 0; i < 4; i++)
			if (win.keys['1' + i]) current = i;

		float time = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();

		//orbit the camera around the model
		float radius = models[current].radius;
		float targetY = models[current].targetY;
		Vec3 from(radius * cosf(time), targetY, radius * sinf(time));
		Matrix view = Matrix::lookAt(from, Vec3(0, targetY, 0), Vec3(0, 1, 0));

		Matrix wvp = projection * view * world;

		core.beginFrame(clearColor);
		pso.bind(&core);
		meshes[current].draw(&core, wvp);
		core.finishFrame();
	}
	core.flushGraphicsQueue();

	return 0;
}