#include "Parameters.h"
#include "imgui/imgui.h"
#include "App.h"
void Parameters::Initialize(App* parentApp) {
	app = parentApp;
}
int Parameters::ResetSim(int o)
{
	if (ImGui::Begin("Simulation Settings")) {
		ImGui::InputInt("No. of spheres", &o);
		if (ImGui::Button("Reset Simulation")) {
			app->ResetSimulation(o);
		}
	}
	ImGui::End();
	return o;
	
}
