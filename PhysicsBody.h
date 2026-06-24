#pragma once
#include "Fields.h"
#include <DirectXMath.h>
#include <vector>
#include <random>
#include <algorithm>

namespace dx = DirectX;

class PhysicsBody
{
public:
	// --- CONSTRUCTION ---
	// Initializes position, velocity, charge, and mass
	PhysicsBody(
		float x, float y, float z,         // initial position
		float dx, float dy, float dz,       // initial velocity
		int charge,                          // -1, 0, or 1
		float mass = 1.0f
	) noexcept;
	~PhysicsBody() = default;
	PhysicsBody() = default;
	// --- INTEGRATION ---
	// Runs one full Boris integration step
	// Queries internal Fields for background E and B, combines with accumulated external forces
	void StepBoris(float dt) noexcept;

	// --- POSITION & VELOCITY ---
	dx::XMVECTOR GetPosVector() const noexcept;
	dx::XMVECTOR GetVelVector() const noexcept;
	dx::XMVECTOR GetAccel() const noexcept { return acceleration; }

	float GetRadiationIntensity() noexcept;

	//(needed by GetTransformXM in rendering layer)
	float GetX() const noexcept { return i; }
	float GetY() const noexcept { return j; }
	float GetZ() const noexcept { return k; }

	// --- POLARIZATION ---
	// Computes the local-space dipole direction for neutral spheres
	// Takes the sphere's current rotation matrices so the world-space E field
	// can be transformed into model space for the pixel shader
	dx::XMVECTOR GetPolarizedVector(
		dx::XMVECTOR Etot,
		float pitch, float yaw, float roll,
		float theta, float phi, float chi
	) noexcept;

	// --- FORCE & FIELD ACCUMULATION ---
	// Adds external interaction force (Coulomb, polarization back-reaction)
	void AddInterForce(dx::XMVECTOR force) noexcept;

	// Adds a local E field contribution from a nearby charged body (needed for neutral bodies)
	void AddElocal(dx::XMVECTOR field) noexcept;

	// Combines LocalExternalE with background field to get total E
	// Also updates internal externalE — call once per frame before StepBoris
	dx::XMVECTOR AddEtotal() noexcept;

	dx::XMVECTOR GetFtotal() const;

	// Returns the total E field computed by the last AddEtotal() call
	dx::XMVECTOR GetEtotal() const noexcept;
	
	// Resets all force and field accumulators to zero
	// Call at end of each frame after StepBoris
	void ResetAccumulators() noexcept;

	// --- PROPERTIES ---
	int GetCharge() const noexcept { return q; }
	float GetMass() const noexcept { return mass; }
	dx::XMVECTOR GetMomentum() noexcept;
	

	// --- ENERGY ---
	// Calculates total kinetic + potential energy across all bodies
	// Note: currently has double-counting bug in nested loop — see Known Issues
	static float CalculateTotalEnergy(const std::vector<PhysicsBody*>& bodies) noexcept;
	float smoothedIntensity = 0.0f;

private:
	// --- POSITION (world space) ---
	float i = 0.0f;   
	float j = 0.0f;   
	float k = 0.0f;   

	// --- VELOCITY ---
	float di = 0.0f;
	float dj = 0.0f;
	float dk = 0.0f;
	dx::XMVECTOR prevVel = dx::XMVectorZero();
	dx::XMVECTOR acceleration = dx::XMVectorZero();
	// --- PROPERTIES ---
	float mass = 1.0f;
	int q = 0;         // charge: -1, 0, or 1

	// --- FIELD QUERIES ---
	Fields field;

	// --- FORCE ACCUMULATORS ---
	// Accumulated inter-particle forces (Coulomb, polarization)
	dx::XMVECTOR externalF = dx::XMVectorZero();

	// Accumulated local E field contributions from nearby charged spheres
	dx::XMVECTOR LocalExternalE = dx::XMVectorZero();

	// Total E field — computed by AddEtotal()
	dx::XMVECTOR externalE = dx::XMVectorZero();

	void UpdatePixelColor(Graphics& gfx, DirectX::XMVECTOR Etot) noexcept;


};
