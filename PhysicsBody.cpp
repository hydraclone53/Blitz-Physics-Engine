#include "PhysicsBody.h"
namespace dx = DirectX;
PhysicsBody::PhysicsBody(float x, float y, float z, float dx, float dy, float dz, int charge, float mass) noexcept
	:
	i(x),
	j(y),
	k(z),
	di(dx),
	dj(dy),
	dk(dz),
	q(charge),
	mass(mass)
{

}
void PhysicsBody::StepBoris(float dt) noexcept
{

	dx::XMFLOAT3 E_field = field.GetEfield(i, j, k);
	dx::XMFLOAT3 B_field = field.GetMfield(i, j, k);
	float q_over_m = (float)q / mass;
	dx::XMVECTOR Ex_F;
	dx::XMVECTOR electric_push_step;
	if (q == 0) {
		Ex_F = externalF;
		electric_push_step = dx::XMVectorScale(Ex_F, 1.0f / mass * dt * 0.5f);
	}
	else {
		Ex_F = dx::XMVectorAdd(dx::XMVectorScale(externalF, 1.0f / (float)q), dx::XMLoadFloat3(&E_field));
		electric_push_step = dx::XMVectorScale(Ex_F, q_over_m * dt * 0.5f);
	}

	dx::XMVECTOR B = dx::XMLoadFloat3(&B_field);


	// --- STEP 1: FIRST ELECTRIC HALF-PUSH ---
	dx::XMVECTOR v_minus = dx::XMVectorAdd(GetVelVector(), electric_push_step);

	// --- STEP 2: PURE MAGNETIC ROTATION ---
	// Calculate t vector: (q*B/m) * (dt/2)
	dx::XMVECTOR t = dx::XMVectorScale(B, q_over_m * dt * 0.5f);

	// Calculate Magnitude Squared of t: |t|^2
	dx::XMVECTOR t_mag_sq = dx::XMVector3LengthSq(t);
	float t_len_sq = dx::XMVectorGetX(t_mag_sq);

	// Calculate s vector: 2 * t / (1 + |t|^2)
	dx::XMVECTOR s = dx::XMVectorScale(t, 2.0f / (1.0f + t_len_sq));

	// Boris cross product rotation operations
	dx::XMVECTOR v_minus_cross_t = dx::XMVector3Cross(v_minus, t);
	dx::XMVECTOR v_prime = dx::XMVectorAdd(v_minus, v_minus_cross_t);

	dx::XMVECTOR v_prime_cross_s = dx::XMVector3Cross(v_prime, s);
	dx::XMVECTOR v_plus = dx::XMVectorAdd(v_minus, v_prime_cross_s);

	// --- STEP 3: SECOND ELECTRIC HALF-PUSH ---
	dx::XMVECTOR v_next = dx::XMVectorAdd(v_plus, electric_push_step);

	//store accel.
	acceleration = dx::XMVectorScale(
		dx::XMVectorSubtract(v_next, GetVelVector()),
		1.0f / dt
	);

	// --- STEP 4: POSITION UPDATE (Leapfrog step) ---
	dx::XMVECTOR pos_next = dx::XMVectorAdd(GetPosVector(), dx::XMVectorScale(v_next, dt));

	// 4. Save results 
	dx::XMStoreFloat3((dx::XMFLOAT3*)&i, pos_next); // Stores to i, j, k sequentially
	dx::XMStoreFloat3((dx::XMFLOAT3*)&di, v_next);
}
dx::XMVECTOR PhysicsBody::GetPolarizedVector(
	dx::XMVECTOR Etot,
	float pitch, float yaw, float roll,
	float theta, float phi, float chi
) noexcept
{
	if (q == 0) {
		// Capture the exact rotation applied to this sphere in the world
		dx::XMMATRIX rotLocal = dx::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
		dx::XMMATRIX rotGlobal = dx::XMMatrixRotationRollPitchYaw(theta, phi, chi);
		dx::XMMATRIX totalRotation = rotLocal * rotGlobal;

		// Invert the rotation matrix (World Space -> Local Space)
		dx::XMMATRIX inverseRotation = dx::XMMatrixInverse(nullptr, totalRotation);

		// Transform the E-field vector into the sphere's Local Space
		// (XMVector3TransformNormal ignores translation)
		dx::XMVECTOR localEtot = dx::XMVector3TransformNormal(Etot, inverseRotation);

		return localEtot;
	}
}

void PhysicsBody::AddInterForce(dx::XMVECTOR force) noexcept
{
	externalF = dx::XMVectorAdd(externalF, force);
}
void PhysicsBody::AddElocal(dx::XMVECTOR field) noexcept
{
	LocalExternalE = dx::XMVectorAdd(LocalExternalE, field);
}
dx::XMVECTOR PhysicsBody::AddEtotal() noexcept
{
	dx::XMFLOAT3 E_field = field.GetEfield(i, j, k);
	externalE = dx::XMVectorAdd(LocalExternalE, dx::XMLoadFloat3(&E_field));
	return externalE;;
}

dx::XMVECTOR PhysicsBody::GetFtotal() const { return externalF; }
dx::XMVECTOR PhysicsBody::GetEtotal() const noexcept
{
	return externalE;
}
void PhysicsBody::ResetAccumulators() noexcept
{
	externalF = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	LocalExternalE = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	externalE = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
}
float PhysicsBody::CalculateTotalEnergy(const std::vector<PhysicsBody*>& bodies) noexcept
{
	float kineticEnergy = 0.0f;
	float potentialEnergy = 0.0f;

	for (const auto& s : bodies) {
		float vSq = dx::XMVectorGetX(dx::XMVector3LengthSq(s->GetMomentum()));
		kineticEnergy += 0.5f * s->mass * vSq;
	}
	for (int j = 0; j < bodies.size();++j) {
		dx::XMVECTOR currentpos = bodies[j]->GetPosVector();
		for (int i = j + 1; i < bodies.size();++i) {
			if (bodies[i] != bodies[j]) {
				dx::XMVECTOR bufferpos = bodies[i]->GetPosVector();
				dx::XMVECTOR dist = dx::XMVectorSubtract(currentpos, bufferpos);
				float dist_sq = dx::XMVectorGetX(dx::XMVector3LengthSq(dist));
				float r = std::sqrt(dist_sq + 1.5f);
				potentialEnergy += 100.0f * bodies[i]->GetCharge() * bodies[j]->GetCharge() / (r);
			}
		}

	}
	for (const auto& s : bodies) {
		if (s->GetCharge() == 0) {
			dx::XMVECTOR E = s->AddEtotal();
			float E_sq = dx::XMVectorGetX(dx::XMVector3Dot(E, E));
			potentialEnergy += -0.5f * 0.05f * E_sq;
		}
	}
	return kineticEnergy + potentialEnergy;
}
dx::XMVECTOR PhysicsBody::GetMomentum() noexcept
{

	return DirectX::XMVectorSet(di, dj, dk, 0.0f);
}
dx::XMVECTOR PhysicsBody::GetPosVector() const noexcept
{
	return dx::XMVectorSet(i, j, k, 0.0f);
}
dx::XMVECTOR PhysicsBody::GetVelVector() const noexcept
{
	return dx::XMVectorSet(di, dj, dk, 0.0f);
}
float PhysicsBody::GetRadiationIntensity() noexcept
{
	if (q == 0) return 0.0f;
	float a_sq = dx::XMVectorGetX(dx::XMVector3LengthSq(acceleration));
	// normalize — tune this reference value to what looks good
	constexpr float referenceAccel = 15.0f;
	float raw = min(a_sq / (referenceAccel * referenceAccel), 1.0f);
	smoothedIntensity = 0.6f * smoothedIntensity + 0.4f * raw;  // decay constant
	return smoothedIntensity;
}
