#include <Windows.h>
#include "Engine/Platform/Window.h"
#include "Engine/Graphics/Core.h"
#include "Engine/Graphics/Mesh.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/ShaderManager.h"
#include "Engine/Graphics/PSOManager.h"
#include "Game/Game.h"
#include <vector>
#include <string>
#include <chrono>

//ask laptops with two gpus to use the dedicated one instead of the integrated one
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

//the sun. one directional light shared by everything in the scene
struct Lighting
{
	Vec3 direction = Vec3(0.4f, -1.0f, 0.6f).normalize(); //pointing down and away from the player
	Vec3 colour = Vec3(1.0f, 0.95f, 0.85f);               //slightly warm
	float ambient = 0.25f;
	float specularStrength = 0.5f;
	float shininess = 32.0f;
};

//draws every object in the list with the default shader
static void drawScene(Core& core, PSOManager& psos, Shader& shader, const Lighting& light,
	const Camera& camera, const Matrix& projection, const std::vector<const GameObject*>& objects)
{
	psos.bind(&core, "Default"); //this sets the root signature, which clears the root arguments, so constants go after it

	//per frame values, the same for every object
	shader.updateConstantPS("lightDirection", &light.direction);
	shader.updateConstantPS("ambient", &light.ambient);
	shader.updateConstantPS("lightColour", &light.colour);
	shader.updateConstantPS("cameraPosition", &camera.position);
	shader.updateConstantPS("specularStrength", &light.specularStrength);
	shader.updateConstantPS("shininess", &light.shininess);

	Matrix viewProjection = projection * camera.getView();

	for (const GameObject* object : objects)
	{
		if (!object->mesh)
			continue;

		Matrix world = object->getWorld();
		Matrix wvp = viewProjection * world;
		shader.updateConstantVS("WVP", wvp.m);
		shader.updateConstantVS("World", world.m);
		shader.updateConstantVS("objectColour", &object->colour);
		shader.apply(&core); //uploads into the next ring buffer slot, so each draw keeps its own copy
		object->mesh->draw(&core);
	}
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	Window win;
	win.create("Aimlock", 1920, 1080);

	Core core;
	core.init(win.hwnd, win.width, win.height);

	//compiled once (or loaded from the .cso cache), constant buffers come from reflection
	ShaderManager shaders;
	Shader* defaultShader = shaders.load(&core, "Default", L"resources/shaders/shader.hlsl");
	if (!defaultShader)
		return 1;

	PSOManager psos;
	psos.createPSO(&core, "Default", *defaultShader, Mesh::inputLayout());

	Mesh cubeMesh, sphereMesh;
	if (!cubeMesh.loadGEM(&core, "resources/Models/Cube.gem"))
		OutputDebugStringA("Cube.gem failed to load\n");
	sphereMesh.createSphere(&core, 16, 32);

	Game game;
	game.init(win, &cubeMesh, &sphereMesh);

	Lighting light;
	Color clearColour = { 0.1f, 0.1f, 0.12f, 1.0f };
	float aspect = (float)win.width / (float)win.height;

	std::vector<const GameObject*> drawList;
	std::string windowTitle;
	auto lastFrame = std::chrono::steady_clock::now();

	while (!win.shouldClose && !game.wantsToQuit())
	{
		win.processMessages();

		//delta time so timers and movement don't depend on the frame rate
		auto now = std::chrono::steady_clock::now();
		float dt = std::chrono::duration<float>(now - lastFrame).count();
		lastFrame = now;

		game.update(win, dt);

		//no ui yet, so the window title shows what's going on. only set it when it changes
		std::string status = game.getStatusText();
		if (status != windowTitle)
		{
			windowTitle = status;
			win.setTitle(windowTitle);
		}

		drawList.clear();
		game.getDrawList(drawList);

		core.beginFrame(clearColour);
		drawScene(core, psos, *defaultShader, light, game.getCamera(), game.getCamera().getProjection(aspect), drawList);
		core.finishFrame();
	}

	core.flushGraphicsQueue(); //wait for the gpu before freeing anything it could still be reading

	cubeMesh.release();
	sphereMesh.release();
	psos.release();
	shaders.release();
	core.release(); //last, since everything else was made from the device

	return 0;
}
