#pragma once
#include "Graphics.h"
class App;
class Parameters
{
public:
	void Initialize(App* parentApp);
	int ResetSim(int o);
	float Set;
private:
	
	App* app=nullptr;
};