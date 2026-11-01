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
	wnd(800, 800, L"epik"),
	coulombHash(100.0f, 2048),
	vdwHash(10.0f, 4096)
	
{
	
	DrawableBase<Sphere>::ClearStaticBinds();
	par.Initialize(this);//imgui stuff

	std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution<float> adist(0.0f, 3.1415f * 2.0f);
	std::uniform_real_distribution<float> ddist(0.0f, 3.1415f * 2.0f);
	std::uniform_real_distribution<float> odist(0.0f, 3.1415f * 0.3f);
	std::uniform_real_distribution<float> rdist(-10.0f, 10.0f);
	std::uniform_int_distribution<int> ch( -1,1);

	wnd.DisableCursor();
	
	for (auto i = 0; i < 2;i++)
	{
		float x = rdist(rng), y = rdist(rng), z = rdist(rng);
		int q = ch(rng);
		int idx = world.Add(x, y, z, 0.0f, 0.0f, 0.0f, q, 1.0f);
		auto pSphere = std::make_unique<Sphere>(wnd.Gfx(), world, idx,
			adist, ddist, odist, rng,false);
		drawables.push_back(std::move(pSphere));
	}
	whip.Init(world);
	for (int idx : whip.GetTrailIndices())
	{
		auto pSphere = std::make_unique<Sphere>(wnd.Gfx(), world, idx,
			adist, ddist, odist, rng, true);
		drawables.push_back(std::move(pSphere));
	}

	/*wall.Spawn(world, 10.0f, 0.0f, 0.0f, 10, 10, 3, 1.0f);
	wall.BuildBonds(world, 1.5f);  // 1.5 captures immediate + diagonal neighbors at spacing=1.0
	for (int idx : wall.GetParticleIndices())
	{
		auto pSphere = std::make_unique<Sphere>(wnd.Gfx(), world, idx,
			adist, ddist, odist, rng, false);
		drawables.push_back(std::move(pSphere));
	}
	*/
	ImGui_ImplWin32_Init(wnd.GetHandle()); // Ensure you have a way to get the HWND
	ImGui_ImplDX11_Init(wnd.Gfx().GetDevice(), wnd.Gfx().GetContext()); 
	wnd.Gfx().SetProjection(dx::XMMatrixPerspectiveLH(1.0f, 1.0f, 0.5f, 4000.0f)); //The box


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

	world.Clear();
	drawables.clear();


	std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution<float> adist(0.0f, 3.1415f * 2.0f);
	std::uniform_real_distribution<float> ddist(0.0f, 3.1415f * 2.0f);
	std::uniform_real_distribution<float> odist(0.0f, 3.1415f * 0.3f);
	std::uniform_real_distribution<float> rdist(-5.0f, 5.0f);
	std::uniform_int_distribution<int> ch(-1, 1);

	//for (auto i = 0; i < obj; i++)
	//{
		float x = rdist(rng), y = rdist(rng), z = rdist(rng);
		int q = ch(rng);
		int idx = world.Add(-8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1, 1.0f);
		int idx2 = world.Add(8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1, 1.0f);
		/*
		int idx = world.Add(-3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1, 1.0f);
		 int idx2 = world.Add(3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1, 1.0f);
		int idx3 = world.Add(3.0f, 6.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1, 1.0f);
		int idx4 = world.Add(-3.0f, 6.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1, 1.0f);
		auto pSphere3 = std::make_unique<Sphere>(wnd.Gfx(), world, idx3,adist, ddist, odist, rng, false);
		auto pSphere4 = std::make_unique<Sphere>(wnd.Gfx(), world, idx4, adist, ddist, odist, rng, false); */
		auto pSphere = std::make_unique<Sphere>(wnd.Gfx(), world, idx,

			adist, ddist, odist, rng,false);
		auto pSphere2 = std::make_unique<Sphere>(wnd.Gfx(), world, idx2,
			adist, ddist, odist, rng, false);
		
		drawables.push_back(std::move(pSphere));
		drawables.push_back(std::move(pSphere2));
		//drawables.push_back(std::move(pSphere3));
		//drawables.push_back(std::move(pSphere4));
	//}
		whip.Init(world);
		for (int idx : whip.GetTrailIndices())
		{
			auto pSphere = std::make_unique<Sphere>(wnd.Gfx(), world, idx,
				adist, ddist, odist, rng, true);
			drawables.push_back(std::move(pSphere));
		}
}

void App::DoFrame()
{	
	wnd.Gfx().SetCamera(cam.GetMatrix());
	auto dt = timer.Mark() * speed_factor;
	constexpr float MAX_DT = 0.005f;      // physics ceiling
	constexpr int MAX_SUBSTEPS = 10;


	float physicsDt = std::min(dt, MAX_DT * MAX_SUBSTEPS); // don't spiral if frame hiccups
	int nSubsteps = static_cast<int>(std::ceil(physicsDt / MAX_DT));
	float subDt = physicsDt / nSubsteps;

	wnd.Gfx().BeginFrame(0.07f, 0.0f, 0.12f);
	HandleCamera();

	float timeStep = wnd.kbd.KeyIsPressed(VK_SPACE) ? 0.0f : dt;
	
	for (int step = 0; step < nSubsteps; ++step)
	{
		world.total_time += subDt;
		auto t0 = std::chrono::high_resolution_clock::now();
		
		if (step % 2 == 0) {
			coulombHash.BuildHash(world.posX.data(), world.posY.data(),
				world.posZ.data(), world.count);
		
			vdwHash.BuildHash(world.posX.data(), world.posY.data(),
				world.posZ.data(), world.count);
			coulombHash.BuildHashNoPairs(world.posX.data(), world.posY.data(),
				world.posZ.data(), world.count);
			vdwHash.BuildHash(world.posX.data(), world.posY.data(),
				world.posZ.data(), world.count);  
		}
		auto t1 = std::chrono::high_resolution_clock::now();
		//pair.CalcCForce(world, coulombHash); //use when polarization is required
		//wall.ApplyBondForces(world,1.5f);
		//int testIdx = wall.GetParticleIndices()[150]; 
		//wall.ApplyTestImpulse(world, testIdx, 0.0f, 0.0f, 5.0f);  // strong push in Z direction
		pair.CalcVandForce(world, vdwHash);
		rad.RetardationFields(world,subDt);
		if (step == 0)
		{
			ImGui::Begin("Field debug");
			ImGui::Text("count: %d", world.count);

			if (world.count >= 2)
			{
				float dx = world.posX[1] - world.posX[0];
				float dy = world.posY[1] - world.posY[0];
				float dz = world.posZ[1] - world.posZ[0];
				float r = std::sqrt(dx * dx + dy * dy + dz * dz);
				ImGui::Text("sep: %.3f", r);

				if (r > 1e-6f)
				{
					float Erad = (world.E_FieldX[0] * dx + world.E_FieldY[0] * dy + world.E_FieldZ[0] * dz) / r;
					float Frad = world.charge[0] * Erad;
					float s = r * r + 1.5f;
					float Fcoul = -PhysConst::GetK() * world.charge[0] * world.charge[1] * r / (s * std::sqrt(s));
					ImGui::Text("retarded F toward other: %.4f", Frad);
					ImGui::Text("coulomb  F toward other: %.4f", Fcoul);
					ImGui::Text("t: %.3f", world.total_time);
					ImGui::Text("accelX[0]: %.4f   ratio: %.4f", world.accelX[0], std::fabs(world.accelX[0]) / std::fabs(Fcoul));
					ImGui::Text("Bz[0]: %.5f", world.M_FieldZ[0]);
				}
			}
			ImGui::End();
		}
		world.AddEtotalAll();
		
		field.CalcEPolar(world, coulombHash);
		float Mx = 0.0f, My = 0.0f, Mz = 0.0f,K = 0.0f, k, mx, my, mz;
		for (int i = 0; i < world.count; i++) {
			world.GetMomentum(i, mx, my, mz);
			world.GetKE(i, k);
			Mx += mx;
			My += my;
			Mz += mz;
			K += k;
		}
		float Msq = Mx * Mx + My * My + Mz * Mz;
		M_total = std::sqrt(Msq);
		KE_total = K;
		
			//rad.ApplyLarmorDamping(subDt, world, i);
		auto t2 = std::chrono::high_resolution_clock::now();
		world.StepAllBoris(subDt);
		world.ResetAccumulators();
		
		auto t3 = std::chrono::high_resolution_clock::now();
		whip.Update(world, subDt);
		whip.ApplyPolarizabilityBoost(world, 2.0f);

		if (step == 0)
		{
			ImGui::Text("Hash: %.3f ms x%d substeps",
				std::chrono::duration<float, std::milli>(t1 - t0).count(), nSubsteps);
			ImGui::Text("Force Calc: %.3f ms",
				std::chrono::duration<float, std::milli>(t2 - t1).count());
			ImGui::Text("Borris: %.3f ms",
				std::chrono::duration<float, std::milli>(t3 - t2).count());
			ImGui::Text("Bonds: %d", wall.GetBondCount());
			ImGui::Text("Fill: %.3f ms", vdwHash.lastFillMs);
			ImGui::Text("Insert: %.3f ms", vdwHash.lastInsertMs);
			ImGui::Text("Pair: %.3f ms", vdwHash.lastPairMs);
			ImGui::Text("Entries capacity: %d", vdwHash.lastCapacity);
			ImGui::Text("vdW total neighbors: %d", pair.totalNeighbors);
		}
		auto t00 = std::chrono::high_resolution_clock::now();
		for (auto& d : drawables)
		d->Update(subDt);
		auto t11 = std::chrono::high_resolution_clock::now();
		if (step == 0) 
		{
		ImGui::Text("Draw: %.3f ms x%d substeps",
				std::chrono::duration<float, std::milli>(t11 - t00).count(), nSubsteps);
		}
	}

	float ex, ey, ez;
	bool ok = rad.ProbeE(world, 1, world.posX[1] + 4.0f, world.posY[1], world.posZ[1],
						 world.total_time, ex, ey, ez);
	float den = 4.0f * 4.0f + 1.5f;
	ImGui::Text("probe ok=%d  Ex=%.4f  coulomb=%.4f", ok, ex,
				PhysConst::GetK() * world.charge[1] * 4.0f / (den * std::sqrt(den)));
	fieldLines.Update(world, rad, 1);     // sphere idx 1
	fieldLines.Controls();

	
	for (auto& d : drawables)
	{

		d->Draw(wnd.Gfx());
	}
	fieldLines.Draw(wnd.Gfx());
	obj = par.ResetSim(obj);

	static char buffer[1024];
	if (ImGui::Begin("Simulation Speed")) {
		ImGui::SliderFloat("Speed Factor", &speed_factor, 0.0f, 4.0f);
		ImGui::Text("Substeps: %d", nSubsteps);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::InputText("woah", buffer, sizeof(buffer));
			}
	ImGui::End();
	cam.SpawnControlWindow();
		

	std::wostringstream oss;
	oss << L"Total momentum " << M_total<< L", Kinetic Energy: " << KE_total;
	wnd.SetTitle(oss.str());
	
	while (!wnd.kbd.KeyIsEmpty())
	{
		const auto e = wnd.kbd.ReadKey();
		if (e.isPress() && e.GetCode() == VK_INSERT)
		{
			if (wnd.CursorEnabled())
			{
				wnd.DisableCursor();
				wnd.mouse.EnableRaw();
			}
			else
			{
				wnd.EnableCursor();
				wnd.mouse.DisableRaw();
			}
		}
	}
	
	ShowRawInputWindow();
	wnd.Gfx().EndFrame();
	
}
void App::ShowRawInputWindow()
{
	while (const auto d = wnd.mouse.ReadRawDelta())
	{
		x += d->x;
		y += d->y;
	}
	if (ImGui::Begin("Raw Input"))
	{
		ImGui::Text("Tally: (%d,%d)", x, y);
		ImGui::Text("Cursor: %s", wnd.CursorEnabled() ? "enabled" : "disabled");
	}
	ImGui::End();
}
void App::HandleCamera() noexcept
{
	// WASD translation — only when cursor is captured
	if (!wnd.CursorEnabled())
	{
		if (wnd.kbd.KeyIsPressed('W')) cam.Translate({ 0.0f,  0.0f,  1.0f });
		if (wnd.kbd.KeyIsPressed('S')) cam.Translate({ 0.0f,  0.0f, -1.0f });
		if (wnd.kbd.KeyIsPressed('A')) cam.Translate({ -1.0f,  0.0f,  0.0f });
		if (wnd.kbd.KeyIsPressed('D')) cam.Translate({ 1.0f,  0.0f,  0.0f });
		if (wnd.kbd.KeyIsPressed('R')) cam.Translate({ 0.0f,  1.0f,  0.0f });
		if (wnd.kbd.KeyIsPressed('F')) cam.Translate({ 0.0f, -1.0f,  0.0f });
	}
	if (wnd.kbd.KeyIsPressed('T'))  // T for "test impulse"
	{
		static bool alreadyTriggered = false;
		if (!alreadyTriggered)
		{
			int testIdx = wall.GetParticleIndices()[166];
			wall.ApplyTestImpulse(world, testIdx, 0.0f, 0.0f, 150.0f);
			alreadyTriggered = true;
		}
	}

	// mouse look — consume all pending raw deltas
	while (const auto delta = wnd.mouse.ReadRawDelta())
	{
		if (!wnd.CursorEnabled())
			cam.Rotate((float)delta->x, (float)delta->y);
	}

	
	float cx = cam.GetX(), cy = cam.GetY(), cz = cam.GetZ();
	float fx, fy, fz, rx, ry, rz, ux, uy, uz;
	cam.GetForwardVector(fx, fy, fz);
	cam.GetRightVector(rx, ry, rz);
	cam.GetUpVector(ux, uy, uz);

	// tune these three offsets to position the whip anywhere on screen
	float forwardOffset = 2.0f;   // how far in front
	float rightOffset = 1.5f;     // positive = right side of screen, negative = left
	float upOffset = -1.5f;       // positive = higher, negative = lower

	float handleOffsetX = cx + fx * forwardOffset + rx * rightOffset + ux * upOffset;
	float handleOffsetY = cy + fy * forwardOffset + ry * rightOffset + uy * upOffset;
	float handleOffsetZ = cz + fz * forwardOffset + rz * rightOffset + uz * upOffset;

	float initialTiltAngle = 0.3491f;  // 20 degrees, tune this
	float cosT = std::cos(initialTiltAngle);
	float sinT = std::sin(initialTiltAngle);

	// rotate (fx, fz) around the Y (up) axis by initialTiltAngle
	float tiltedFx = fx * cosT + fz * sinT;
	float tiltedFz = -fx * sinT + fz * cosT;
	float tiltedFy = fy;  // unchanged — this is a horizontal-plane rotation only

	whip.UpdateAim(handleOffsetX, handleOffsetY, handleOffsetZ, tiltedFx, tiltedFy, tiltedFz,rx,ry,rz);
	

	// trigger only starts the swing — direction/position already tracked by UpdateAim
	if (wnd.kbd.KeyIsPressed('E') && !whip.IsActive())
	{
		whip.Trigger();
	}
	// upload final camera matrix to GPU
	wnd.Gfx().SetCamera(cam.GetMatrix());
}