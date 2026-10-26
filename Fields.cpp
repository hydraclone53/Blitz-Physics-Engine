//Add artificial fields for tests.

#include "Fields.h"
#include "BindableBase.h"
#include "Define.h"
namespace dx = DirectX;
Fields::Fields() noexcept{}

dx::XMFLOAT3 Fields::GetEfield(float X,float Y,float Z) noexcept {
	E = { 0.0f,0.0f,0.0f };
	return E;
}
 void Fields::CalcEforce(int charge, dx::XMFLOAT3 Ef) noexcept {
	EF = {
		charge * Ef.x,
		charge * Ef.y,
		charge * Ef.z
	};
	//return Ef;
}
 dx::XMFLOAT3 Fields::GetEforce() noexcept {
	 return EF;
 }

 dx::XMFLOAT3 Fields::GetMfield(float X, float Y, float Z) noexcept
 {
	 M = { 0.0f,10.0f,0.0f };
	 return M;
 }

 void Fields::CalcMforce(int charge, dx::XMFLOAT3 V, dx::XMFLOAT3 mF) noexcept
 {
	 MF = { charge*(V.y * mF.z - V.z * mF.y),
			charge*(V.z * mF.x - V.x * mF.z),
			charge*(V.x * mF.y - V.y * mF.x)
		 };
 }

 dx::XMFLOAT3 Fields::GetMforce() const noexcept
 {
	 return MF;
 }
