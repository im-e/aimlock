#include <Windows.h>
#include "Engine/Platform/Window.h"
#include "Engine/Graphics/Core.h"
#include "Engine/Graphics/Mesh.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/ShaderManager.h"
#include "Engine/Graphics/PSOManager.h"
#include "Game/Camera.h"
#include "Game/GameObject.h"
#include <vector>
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

	//shaders are compiled once (or loaded from their .cso cache) and kept by name
	//their constant buffers are created automatically by reflection
	ShaderManager shaders;
	Shader* defaultShader = shaders.load(&core, "Default", L"resources/shaders/shader.hlsl");
	if (!defaultShader)
		return 1;

	//one pso per shader + vertex layout combination, also kept by name
	PSOManager psos;
	psos.createPSO(&core, "Default", *defaultShader, Mesh::inputLayout());

	//meshes are the gpu data, loaded once each
	Mesh bunnyMesh, cubeMesh, trexMesh, treeMesh;
	if (!bunnyMesh.loadGEM(&core, "resources/Models/Bunny.gem")) OutputDebugStringA("Bunny.gem failed to load\n");
	if (!cubeMesh.loadGEM(&core, "resources/Models/Cube.gem")) OutputDebugStringA("Cube.gem failed to load\n");
	if (!trexMesh.loadGEM(&core, "resources/Models/TRex.gem")) OutputDebugStringA("TRex.gem failed to load\n");
	if (!treeMesh.loadGEM(&core, "resources/Models/acacia_003.gem")) OutputDebugStringA("acacia_003.gem failed to load\n");

	//objects are things placed in the world - 1 unit = 1 metre, y = 0 is the ground
	//the models were made at very different sizes, so each gets scaled to something sensible:
	//  bunny 0.15 tall -> x3 = 0.5m,  cube 1 tall -> 1m,  trex 577 tall -> x0.007 = 4m,  tree 530 tall -> x0.012 = 6.4m
	Vec3 noRotation(0.0f, 0.0f, 0.0f);
	std::vector<GameObject> objects;
	objects.push_back(GameObject("Cube",  &cubeMesh,  Vec3(-3.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f), noRotation, Color(0.8f, 0.3f, 0.2f, 1.0f)));
	objects.push_back(GameObject("Bunny", &bunnyMesh, Vec3(-1.0f, 0.0f, 0.0f), Vec3(3.0f, 3.0f, 3.0f), noRotation, Color(0.9f, 0.9f, 0.85f, 1.0f)));
	objects.push_back(GameObject("TRex",  &trexMesh,  Vec3(5.0f, 0.0f, 6.0f), Vec3(0.007f, 0.007f, 0.007f), Vec3(0.0f, -PI / 2.0f, 0.0f), Color(0.45f, 0.55f, 0.35f, 1.0f)));
	objects.push_back(GameObject("Tree1", &treeMesh,  Vec3(-8.0f, 0.0f, 10.0f), Vec3(0.012f, 0.012f, 0.012f), noRotation, Color(0.35f, 0.5f, 0.25f, 1.0f)));
	objects.push_back(GameObject("Tree2", &treeMesh,  Vec3(12.0f, 0.0f, 16.0f), Vec3(0.012f, 0.012f, 0.012f), Vec3(0.0f, 1.3f, 0.0f), Color(0.35f, 0.5f, 0.25f, 1.0f))); //same mesh, different place
	//ground: the cube squashed flat and stretched out, sunk so its top sits at y = 0
	objects.push_back(GameObject("Ground", &cubeMesh, Vec3(0.0f, -0.1f, 10.0f), Vec3(60.0f, 0.1f, 60.0f), noRotation, Color(0.4f, 0.45f, 0.35f, 1.0f)));

	//the sun: one directional light shared by everything
	Vec3 lightDirection = Vec3(0.4f, -1.0f, 0.6f).normalize(); //pointing down and away, so light comes from above-behind the start view
	Vec3 lightColour(1.0f, 0.95f, 0.85f); //slightly warm
	float ambient = 0.25f;
	float specularStrength = 0.5f;
	float shininess = 32.0f;

	float aspect = (float)win.width / (float)win.height;
	//near/far in metres - a small far/near ratio keeps the depth buffer precise
	Matrix projection = Matrix::perspective(45.0f, aspect, 0.1f, 1000.0f);

	//delta time: how long the last frame took, so movement doesn't depend on frame rate
	auto lastFrame = std::chrono::steady_clock::now();
	float time = 0.0f;

	//fps camera, standing on the ground looking towards the objects
	Camera camera;
	camera.position = Vec3(0.0f, camera.eyeHeight, -8.0f);

	Color clearColor = { 0.1f, 0.1f, 0.12f, 1.0f }; //dark grey

	while (!win.shouldClose)
	{
		win.processMessages();
		if (win.keys[VK_ESCAPE]) break;

		auto now = std::chrono::steady_clock::now();
		float dt = std::chrono::duration<float>(now - lastFrame).count();
		lastFrame = now;
		time += dt;

		//mouse to look, WASD to walk, shift to sprint, F to fly (space/ctrl up/down)
		camera.update(win, dt);
		Matrix view = camera.getView();
		Matrix viewProjection = projection * view; //the same for every object this frame

		//spin the cube so you can see transforms working
		objects[0].rotation.y += dt;

		Shader* shader = defaultShader;

		core.beginFrame(clearColor);
		psos.bind(&core, "Default"); //sets the root signature, which clears root arguments - so apply after this

		//light values are the same for every object this frame
		shader->updateConstantPS("lightDirection", &lightDirection);
		shader->updateConstantPS("ambient", &ambient);
		shader->updateConstantPS("lightColour", &lightColour);
		shader->updateConstantPS("cameraPosition", &camera.position);
		shader->updateConstantPS("specularStrength", &specularStrength);
		shader->updateConstantPS("shininess", &shininess);

		//each object gets its own WVP - apply() moves the ring buffer on, so every draw keeps its own copy
		for (GameObject& object : objects)
		{
			if (!object.mesh)
				continue;
			Matrix world = object.getWorld();
			Matrix wvp = viewProjection * world;
			shader->updateConstantVS("WVP", wvp.m);
			shader->updateConstantVS("World", world.m);
			shader->updateConstantVS("objectColour", &object.colour);
			shader->apply(&core); //uploads + binds the constant buffers for this draw
			object.mesh->draw(&core);
		}
		core.finishFrame();
	}
	core.flushGraphicsQueue(); //gpu must be finished before we free anything it might read

	bunnyMesh.release();
	cubeMesh.release();
	trexMesh.release();
	treeMesh.release();
	psos.release();
	shaders.release();
	core.release(); //last, everything else was created from the device

	return 0;
}
