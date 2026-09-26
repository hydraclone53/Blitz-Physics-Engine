#pragma once
#include "Graphics.h"

class Camera
{
public:
	Camera() noexcept;
	DirectX::XMMATRIX GetMatrix() const noexcept;
	void SpawnControlWindow() noexcept;
	void Reset() noexcept;
	void Rotate(float dx, float dy) noexcept;
	void Translate(DirectX::XMFLOAT3 translation) noexcept;
	float GetX() const noexcept { return pos.x; }
	float GetY() const noexcept { return pos.y; }
	float GetZ() const noexcept { return pos.z; }
	void GetForwardVector(float& fx, float& fy, float& fz) const noexcept;
	void GetRightVector(float& rx, float& ry, float& rz) const noexcept;
	void GetUpVector(float& ux, float& uy, float& uz) const noexcept;
private:
	DirectX::XMFLOAT3 pos;
	float pitch;
	float yaw;
	static constexpr float travelSpeed = 0.2f;
	static constexpr float rotationSpeed = 0.004f;

};