#include "App.h"
#include "Box.h"
#include <memory>
#include <sstream>
#include <vector>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

App::App()
	:
	wnd(800, 800, L"epik")
	
{
	par.Initialize(this);//imgui stuff
	std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution<float> adist(0.0f, 3.1415f * 2.0f);
	std::uniform_real_distribution<float> ddist(0.0f, 3.1415f * 2.0f);
	std::uniform_real_distribution<float> odist(0.0f, 3.1415f * 0.3f);
	std::uniform_real_distribution<float> rdist(0.0f,1.0f);
	std::uniform_int_distribution<int> ch( -1,1);
	for (auto i = 0; i < 1; i++)
	{
		auto pSphere = std::make_unique<Sphere>(wnd.Gfx(), rng, adist, ddist, odist, rdist, ch);
		bodies.push_back(pSphere->GetPhysicsBody());
		drawables.push_back(std::move(pSphere));
	}
	for (auto i = 0; i < 0; i++)
	{
		auto pCube = std::make_unique<Cube>(wnd.Gfx(), rng, adist, ddist, odist, rdist, ch);
		bodies.push_back(pCube->GetPhysicsBody()); // Save raw tracking address
		drawables.push_back(std::move(pCube));
	}

	ImGui_ImplWin32_Init(wnd.GetHandle()); // Ensure you have a way to get the HWND
	ImGui_ImplDX11_Init(wnd.Gfx().GetDevice(), wnd.Gfx().GetContext()); 
	wnd.Gfx().SetProjection(dx::XMMatrixPerspectiveLH(1.0f, 1.0f, 0.5f, 400.0f)); //The box


}
App::~App() {
	ImGui_ImplWin32_Shutdown();
	ImGui_ImplDX11_Shutdown();
}
int App::Go()
{
	while (true)
	{
		if (const auto ecode = Window::ProcessMessages())
		{
			return *ecode;
		}
		DoFrame();
	}
}

void App::ResetSimulation(int obj)
{
	// 1. Clear existing spheres
	bodies.clear();
	drawables.clear();

	
	std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution<float> adist(0.0f, 3.1415f * 2.0f);
	std::uniform_real_distribution<float> ddist(0.0f, 3.1415f * 2.0f);
	std::uniform_real_distribution<float> odist(0.0f, 3.1415f * 0.3f);
	std::uniform_real_distribution<float> rdist(-10.0f, 10.0f);
	std::uniform_int_distribution<int> ch(-1,1);

	for (auto i = 0; i < obj; i++)
	{
		auto pSphere = std::make_unique<Sphere>(wnd.Gfx(), rng, adist, ddist, odist, rdist, ch);
		bodies.push_back(pSphere->GetPhysicsBody());
		drawables.push_back(std::move(pSphere));
	}
	for (auto i = 0; i < 0; i++)
	{
		auto pCube = std::make_unique<Cube>(wnd.Gfx(), rng, adist, ddist, odist, rdist, ch);
		bodies.push_back(pCube->GetPhysicsBody()); // Save raw tracking address
		drawables.push_back(std::move(pCube));
	}
}

void App::DoFrame()
{	
	auto dt = timer.Mark() * speed_factor;
	constexpr float MAX_DT = 0.005f;      // physics ceiling
	constexpr int MAX_SUBSTEPS = 10;

	float physicsDt = std::min(dt, MAX_DT * MAX_SUBSTEPS); // don't spiral if frame hiccups
	int nSubsteps = static_cast<int>(std::ceil(physicsDt / MAX_DT));
	float subDt = physicsDt / nSubsteps;

	wnd.Gfx().BeginFrame(0.07f, 0.0f, 0.12f);
	wnd.Gfx().SetCamera(cam.GetMatrix());
	float timeStep = wnd.kbd.KeyIsPressed(VK_SPACE) ? 0.0f : dt;
	dx::XMVECTOR TotalMomentum = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);


	float energy = bodies[0]->CalculateTotalEnergy(bodies);

	for (int step = 0; step < nSubsteps; ++step)
	{
		pair.CalcCForce(bodies);
		pair.CalcVandForce(bodies);
		for (PhysicsBody* b : bodies)
		{
			field.CalcEPolar(bodies, b);
			rad.ApplyLarmorDamping(subDt,b);
		}

		for (auto& d : drawables)
		{
			d->Update(subDt);   // this calls StepBoris + ResetAccumulators internally
		}
	}

	// momentum readout after all substeps
	for (PhysicsBody* b : bodies)
	{
		TotalMomentum = dx::XMVectorAdd(TotalMomentum, b->GetMomentum());
	}
	float TotalMomentumMag = dx::XMVectorGetX(dx::XMVector3Length(TotalMomentum));
	for (auto& d : drawables)
	{

		d->Draw(wnd.Gfx());
	}
	obj = par.ResetSim(obj);
	static char buffer[1024];
	if (ImGui::Begin("Simulation Speed")) {
		ImGui::SliderFloat("Speed Factor", &speed_factor, 0.0f, 4.0f);
	
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::InputText("woah", buffer, sizeof(buffer));
			}
	ImGui::End();
	cam.SpawnControlWindow();
		

	std::wostringstream oss;
	oss << L"Total Momentum: " << TotalMomentumMag<<L", Total Energy: "<< energy;
	wnd.SetTitle(oss.str());
	
	

	wnd.Gfx().EndFrame();
	
}