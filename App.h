#pragma once
#include "Window.h"
#include "Cube.h"
#include "PhysicsBody.h"
#include "SystemForce.h"
#include "Sphere.h"
#include "ChiliTimer.h"
#include "Camera.h"
#include "ImguiManager.h"
#include "Parameters.h"
#include "SpatialHash.h"
#include "PhysicsWorld.h"
#include "CuboidWall.h"

class App
{
public:
	App();
	~App();
	int Go();
	void ResetSimulation(int obj);
private:
	void DoFrame();
	void ShowRawInputWindow();
	void HandleCamera() noexcept;
private:
	int x = 0, y = 0;
	int obj = 1; // for imgui
	std::mt19937 rng(std::random_device);
	std::uniform_real_distribution<float> adist;
	std::uniform_real_distribution<float> ddist;
	std::uniform_real_distribution<float> odist;
	std::uniform_real_distribution<float> rdist;
	std::uniform_int_distribution<int> ch;
	Window wnd;
	Camera cam;
	Parameters par;
	CuboidWall wall;

	SpatialHash coulombHash;
	SpatialHash vdwHash;

	ImguiManager  imgui;
	bool show_demo_window = true;
	float speed_factor = 1.0f;
	ChiliTimer timer;
	InterForce::PairForce pair;
	InterForce::FieldForce field;
	InterForce::RadiationForce rad;


	std::vector<class PhysicsBody*> bodies;
	std::vector<float> posX, posY, posZ;
	std::vector<std::unique_ptr<class Drawable>> drawables;

	PhysicsWorld world;
};