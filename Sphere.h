#pragma once
#include "DrawableBase.h"
#include "Fields.h"
#include <algorithm>
#include "ConstantBuffers.h"
#include "Graphics.h"
#include "PhysicsBody.h"

class Sphere : public DrawableBase<Sphere>
{
public:
	Sphere(Graphics& gfx, std::mt19937& rng,
		std::uniform_real_distribution<float>& adist,
		std::uniform_real_distribution<float>& ddist,
		std::uniform_real_distribution<float>& odist,
		std::uniform_real_distribution<float>& rdist,
		std::uniform_int_distribution<int>& ch);
	void Update(float dt) noexcept override;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	dx::XMVECTOR GetMomentum() noexcept;
	DirectX::XMVECTOR GetPos() const noexcept;
	int GetCharge() const noexcept;
	void AddInterForce(dx::XMVECTOR inter) noexcept;
	void AddElocal(dx::XMVECTOR inter) noexcept;
	dx::XMVECTOR AddEtotal() noexcept;
	dx::XMVECTOR GetEtotal() noexcept;
	float CalculateTotalEnergy(const std::vector<Sphere*>& spheres);
	void UpdatePixelColor(Graphics& gfx, DirectX::XMVECTOR Etot) noexcept;
	void UpdateGlow(Graphics& gfx) noexcept;
	PhysicsBody* GetPhysicsBody() noexcept { return &body; }

private:
	Graphics* pGfx = nullptr;
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
	
	// positional
	float r;
	float roll = 0.0f;
	float pitch = 0.0f;
	float yaw = 0.0f;
	float theta;
	float phi;
	float chi;
	// speed (delta/s)
	float droll;
	float dpitch;
	float dyaw;
	float dtheta;
	float dphi;
	float dchi;
	float mass = 1.0f;
	int q = 1;
	PhysicsBody body;
	PixelConstantBuffer<ConstantBuffer2>* pColorCbuf = nullptr;
	PixelConstantBuffer<ConstantBufferQ>* pPolarCbuf = nullptr;
	
};
