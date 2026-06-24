#include "VectorCalc.h"
#include "Graphics.h"
#include <sstream>

dx::XMVECTOR vector::CalcGrad(std::function<float(float, float, float)> field, float x, float y, float z, float h)
{
    float dfdx = (field(x + h, y, z) - field(x - h, y, z)) / (2.0f * h);
    float dfdy = (field(x, y + h, z) - field(x, y - h, z)) / (2.0f * h);
    float dfdz = (field(x, y, z + h) - field(x, y, z - h)) / (2.0f * h);

    dx::XMVECTOR vec= dx::XMVectorSet(dfdx, dfdy, dfdz,0.0f);
    return vec;
}

float vector::CalcDiv(dx::XMVECTOR vec)
{
    return 0.0f;
}
