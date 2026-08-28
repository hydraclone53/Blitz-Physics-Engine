#pragma once
#include "DrawableBase.h"
#include "Fields.h"
#include <algorithm>
#include "ConstantBuffers.h"
#include "Graphics.h"

#include "PhysicsWorld.h"

class Sphere : public DrawableBase<Sphere>
{
public:
	Sphere(Graphics& gfx,
		PhysicsWorld& world,
		int bodyIndex,
		std::uniform_real_distribution<float>& adist,
		std::uniform_real_distribution<float>& ddist,
		std::uniform_real_distribution<float>& odist,
		std::mt19937& rng);

	void Update(float dt) noexcept override;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	void Draw(Graphics& gfx) const noexcept(!IS_DEBUG);



private:
	void UpdatePixelColor(Graphics& gfx) noexcept;
	void UpdateGlow(Graphics& gfx) noexcept;

	PhysicsWorld* pWorld = nullptr;
	int bodyIndex = -1;

	float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;
	float theta = 0.0f, phi = 0.0f, chi = 0.0f;
	float droll, dpitch, dyaw, dphi, dtheta, dchi;
	float r;
	float mass = 1.0f;
	int q = 0;

	Graphics* pGfx = nullptr;
	int check = 1;
	struct ConstantBufferQ {
		DirectX::XMFLOAT3 polarizationDirection; // The direction of the E-field
		float padding;
	};
	Sphere::ConstantBufferQ qb;
	struct ConstantBuffer2 {
		struct { float r, g, b, a; } face_colors[6];
		int renderMode;
		float padding[3];
	};
	struct ConstantBufferGlow {
		float glowIntensity;
		float padding[3];
	};
	PixelConstantBuffer<ConstantBufferGlow>* pGlowCbuf = nullptr;
	

	
	PixelConstantBuffer<ConstantBuffer2>* pColorCbuf = nullptr;
	PixelConstantBuffer<ConstantBufferQ>* pPolarCbuf = nullptr;
	
};
