#pragma once
#include "Graphics.h"
#include "DrawableBase.h"
#include "PhysicsBody.h"
#include "PhysConstants.h"

class PhysicsBody;

namespace InterForce {

	//calculate force based on pairs
	class PairForce {
	public:
		void CalcCForce(std::vector<PhysicsBody*>& bodies);
		void CalcVandForce(std::vector<PhysicsBody*>& bodies);
	private:
		DirectX::XMVECTOR Cforce = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	};

	//calculate force based on field
	class FieldForce {
	public:
		void CalcEPolar(std::vector<PhysicsBody*>& bodies, PhysicsBody* p);
	private:
	};
	class RadiationForce {
	public:
		void ApplyLarmorDamping(float dt, PhysicsBody* b) noexcept;
	};
};


