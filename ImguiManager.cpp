#include "ImguiManager.h"
#include "imgui/imgui.h"
#include <filesystem>
#include "imgui/imgui_impl_win32.h"
#include "Window.h"

ImguiManager::ImguiManager()
{
	/*namespace fs = std::filesystem;
	if (!fs::exists("imgui.ini") && fs::exists("imgui_default.ini"))
	{
		fs::copy_file("imgui_default.ini", "imgui.ini");
	}*/

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
}

ImguiManager::~ImguiManager()
{
	
	ImGui::DestroyContext();
}