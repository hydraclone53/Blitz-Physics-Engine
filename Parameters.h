#pragma once
#include "Graphics.h"
class App;
class Parameters
{
public:
	void Initialize(App* parentApp);
	int ResetSim(int o);
private:
	
	App* app=nullptr;
};