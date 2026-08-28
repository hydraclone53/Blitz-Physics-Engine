#include "PhysicsBody.h"
#include "PhysConstants.h"
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
	// read u directly from storage
	dx::XMVECTOR u = dx::XMVectorSet(di, dj, dk, 0.0f);

	// compute gamma from current u — stored for external use
	float u_sq = dx::XMVectorGetX(dx::XMVector3LengthSq(u));
	gamma = std::sqrt(1.0f + u_sq / (PhysConst::c * PhysConst::c));

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
		Ex_F = dx::XMVectorAdd(dx::XMVectorScale(externalF, 1.0f / (float)q),
			dx::XMLoadFloat3(&E_field));
		electric_push_step = dx::XMVectorScale(Ex_F, q_over_m * dt * 0.5f);
	}

	dx::XMVECTOR B = dx::XMLoadFloat3(&B_field);

	// --- STEP 1: FIRST ELECTRIC HALF-PUSH (in u space) ---
	dx::XMVECTOR u_minus = dx::XMVectorAdd(u, electric_push_step);

	// --- STEP 2: MAGNETIC ROTATION (divided by gamma) ---
	dx::XMVECTOR t = dx::XMVectorScale(B, q_over_m * dt / (2.0f * gamma));
	float t_len_sq = dx::XMVectorGetX(dx::XMVector3LengthSq(t));
	dx::XMVECTOR s = dx::XMVectorScale(t, 2.0f / (1.0f + t_len_sq));

	dx::XMVECTOR u_prime = dx::XMVectorAdd(u_minus, dx::XMVector3Cross(u_minus, t));
	dx::XMVECTOR u_plus = dx::XMVectorAdd(u_minus, dx::XMVector3Cross(u_prime, s));

	// --- STEP 3: SECOND ELECTRIC HALF-PUSH ---
	dx::XMVECTOR u_next = dx::XMVectorAdd(u_plus, electric_push_step);

	// compute acceleration before storing u_next
	acceleration = dx::XMVectorScale(
		dx::XMVectorSubtract(u_next, u),
		1.0f / dt
	);

	// --- STEP 4: POSITION UPDATE ---
	// compute gamma_next from u_next for velocity conversion
	float u_next_sq = dx::XMVectorGetX(dx::XMVector3LengthSq(u_next));
	float gamma_next = std::sqrt(1.0f + u_next_sq / (PhysConst::c * PhysConst::c));
	dx::XMVECTOR v_next = dx::XMVectorScale(u_next, 1.0f / gamma_next);

	dx::XMVECTOR pos_next = dx::XMVectorAdd(GetPosVector(),
		dx::XMVectorScale(v_next, dt));

	// store u_next (not v_next) into di,dj,dk
	dx::XMStoreFloat3((dx::XMFLOAT3*)&i, pos_next);
	dx::XMStoreFloat3((dx::XMFLOAT3*)&di, u_next);
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
	return dx::XMVectorZero();
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
		dx::XMVECTOR u = dx::XMVectorSet(di, dj, dk, 0.0f);
		return dx::XMVectorScale(u, 1.0f / gamma);
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
