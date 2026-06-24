#include "SystemForce.h"
namespace dx = DirectX;

// F = KQ^2/r^3 (magnitude)
void InterForce::PairForce::CalcCForce(std::vector<PhysicsBody*>& bodies) {
	
	for (int j = 0; j < bodies.size();++j) {
		dx::XMVECTOR currentpos = bodies[j]->GetPosVector();
		for (int i = j+1; i < bodies.size();++i) {

				dx::XMVECTOR bufferpos = bodies[i]->GetPosVector();
				dx::XMVECTOR dist = dx::XMVectorSubtract(currentpos, bufferpos);
				dx::XMVECTOR buffer_dist_sq = dx::XMVector3LengthSq(dist);
				float dist_sq = dx::XMVectorGetX(buffer_dist_sq)+1.5f; // adds buffer to prevent 1/r divergence
				float dist_cb = dist_sq * std::sqrt(dist_sq);

				// If body is neutral, add the field to the location where nuetral body is
				if (bodies[i]->GetCharge() == 0) {
					dx::XMVECTOR CfieldBuffer = dx::XMVectorScale(dist, -1.0f * PhysConst::GetK() / (dist_cb) * (float)bodies[j]->GetCharge());
					bodies[i]->AddElocal(CfieldBuffer);
				}
				else if (bodies[j]->GetCharge() == 0) {
					dx::XMVECTOR CfieldBuffer = dx::XMVectorScale(dist, PhysConst::GetK() / (dist_cb) * (float)bodies[i]->GetCharge());
					bodies[j]->AddElocal(CfieldBuffer);
				}

				dx::XMVECTOR CforceBuffer = dx::XMVectorScale(dist, PhysConst::GetK() / (dist_cb) * (float)bodies[j]->GetCharge() * (float)bodies[i]->GetCharge());
				// Only apply Coulomb force if both are charged
				if (bodies[i]->GetCharge() != 0 && bodies[j]->GetCharge() != 0) {
					bodies[i]->AddInterForce(dx::XMVectorScale(CforceBuffer, -1.0f));
					bodies[j]->AddInterForce(CforceBuffer);
				}
				
			
		}
	}
		
}

// Dipole force on charged body from induced dipole p at neutral sphere:
// F = k*q/r^3 * [3(p.r_hat)r_hat - p]
void InterForce::FieldForce::CalcEPolar(std::vector<PhysicsBody*>& bodies, PhysicsBody* p) {
	// 0.05 is susceptibility
	// where p = chi * E_total (induced dipole moment)
	dx::XMVECTOR polar = dx::XMVectorScale(p->GetEtotal(),0.05f);
	dx::XMVECTOR currentpos = p->GetPosVector();
	if (p->GetCharge() == 0) {
		for (int i = 0; i < bodies.size();++i) {
			if (p != bodies[i] && bodies[i]->GetCharge() != 0) {
				dx::XMVECTOR bufferpos = bodies[i]->GetPosVector();
				dx::XMVECTOR dist = dx::XMVectorSubtract(bufferpos, currentpos);
				dx::XMVECTOR buffer_dist_sq = dx::XMVector3LengthSq(dist);
				float dist_sq = dx::XMVectorGetX(buffer_dist_sq) + 1.5f; // same buffer in coloumb
				float dist_cb = dist_sq * std::sqrt(dist_sq);
				dx::XMVECTOR polarForce = dx::XMVectorScale(dx::XMVectorSubtract(dx::XMVectorScale(dx::XMVectorMultiply(dx::XMVector3Dot(dist, polar), dist), 3.0f / dist_sq), polar), PhysConst::GetK() * bodies[i]->GetCharge() / dist_cb);
				bodies[i]->AddInterForce(polarForce);
				p->AddInterForce(dx::XMVectorScale(polarForce, -1.0f));
			}
		}
		
	}

}

// F = 24 * epsilon / r * (2*(sigma/r)^12 - (sigma/r)^6),  repulsive-only term: 24 * epsilon / r * (2*(sigma/r)^12
void InterForce::PairForce::CalcVandForce(std::vector<PhysicsBody*>& bodies)
{
	constexpr float sigma = 1.0f;
	constexpr float epsilon = 2.0f;
	constexpr float r_min = 0.6f;            // clamp factor
	constexpr float r_min_sq = r_min * r_min;
	constexpr float sigma_sq = sigma * sigma;

	for (size_t j = 0; j < bodies.size(); ++j)
	{
		dx::XMVECTOR posJ = bodies[j]->GetPosVector();
		for (size_t i = j + 1; i < bodies.size(); ++i)
		{
			dx::XMVECTOR dist = dx::XMVectorSubtract(posJ, bodies[i]->GetPosVector());
			float r_sq = dx::XMVectorGetX(dx::XMVector3LengthSq(dist));

			// clamp the *effective* distance, never let r_sq go below r_min_sq
			float r_sq_clamped = (r_sq < r_min_sq) ? r_min_sq : r_sq;

			float sr2 = sigma_sq / r_sq_clamped;     // (sigma/r)^2
			float sr6 = sr2 * sr2 * sr2;             // (sigma/r)^6
			float sr12 = sr6 * sr6;                  // (sigma/r)^12

			// magnitude of repulsive LJ force / r  (so we can scale `dist` vector directly)
			//float forceScale = 24.0f * epsilon * sr12 / r_sq_clamped;
			float forceScale = 24.0f * epsilon * (2.0f * sr12 - sr6) / r_sq_clamped;

			dx::XMVECTOR CforceBuffer = dx::XMVectorScale(dist, forceScale);
			bodies[i]->AddInterForce(dx::XMVectorScale(CforceBuffer, -1.0f));
			bodies[j]->AddInterForce(CforceBuffer);
		}
	}
}

//using Abraham-Lorentz approx. (accel. over jerk)
void InterForce::RadiationForce::ApplyLarmorDamping(float dt, PhysicsBody* body) noexcept
{
	if (body->GetCharge() == 0) return;  // neutral bodies don't radiate

	dx::XMVECTOR radiationForce = dx::XMVectorScale(body->GetAccel(), -(float)(body->GetCharge() * body->GetCharge()) * PhysConst::GetB());
	body->AddInterForce(radiationForce);
}