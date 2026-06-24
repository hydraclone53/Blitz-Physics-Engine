#pragma once
#include "DrawableBase.h"
namespace dx = DirectX;
class Fields {
public:
	Fields() noexcept;
	dx::XMFLOAT3 GetEfield(float X, float Y, float Z) noexcept;
	void CalcEforce(int charge, dx::XMFLOAT3 E) noexcept;
	dx::XMFLOAT3 GetEforce() noexcept;
	dx::XMFLOAT3 GetMfield(float X, float Y, float Z) noexcept;
	void CalcMforce(int charge, dx::XMFLOAT3 V, dx::XMFLOAT3 M) noexcept;
	dx::XMFLOAT3 GetMforce() const noexcept;
private:
	float x, y, z;
	dx::XMFLOAT3 E;
	dx::XMFLOAT3 M;
	DirectX::XMFLOAT3 EF;
	DirectX::XMFLOAT3 MF;
};