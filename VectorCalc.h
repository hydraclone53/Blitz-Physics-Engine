#pragma once
#include "Graphics.h"
#include "DrawableBase.h"
#include "PhysicsBody.h"
#include <sstream>
#include <functional>
class vector {
	dx::XMVECTOR CalcGrad(std::function<float(float, float, float)> field, float x, float y, float z, float h);
	float CalcDiv(dx::XMVECTOR vec);
	void CaldCurl(dx::XMVECTOR vec);
};