#include "Camera.h"
#include "imgui/imgui.h"
#include "PhysConstants.h"  // for PhysConst::pi

namespace dx = DirectX;
static constexpr float PI = PhysConst::pi;
constexpr double PI_D = 3.1415926535897932;

template<typename T>
T wrap_angle(T theta)
{
	const T modded = fmod(theta, (T)2.0 * (T)PI_D);
	return (modded > (T)PI_D) ?
		(modded - (T)2.0 * (T)PI_D) :
		modded;
}



Camera::Camera() noexcept
{
	Reset();
}

DirectX::XMMATRIX Camera::GetMatrix() const noexcept
{
	using namespace dx;

	const dx::XMVECTOR forwardBaseVector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	// apply the camera rotations to a base vector
	const auto lookVector = XMVector3Transform(forwardBaseVector,
		XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f)
	);
	// generate camera transform (applied to all objects to arrange them relative
	// to camera position/orientation in world) from cam position and direction
	// camera "top" always faces towards +Y (cannot do a barrel roll)
	const auto camPosition = dx::XMVectorSet(pos.x, pos.y, pos.z, 0.0f);

	const auto camTarget = camPosition + lookVector;
	return XMMatrixLookAtLH(camPosition, camTarget, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}

void Camera::SpawnControlWindow() noexcept
{
	if (ImGui::Begin("Camera"))
	{
		ImGui::Text("Position");
		ImGui::SliderFloat("X", &pos.x, -80.0f, 80.0f, "%.1f");
		ImGui::SliderFloat("Y", &pos.y, -80.0f, 80.0f, "%.1f");
		ImGui::SliderFloat("Z", &pos.z, -80.0f, 80.0f, "%.1f");
		ImGui::Text("Orientation");
		ImGui::SliderAngle("Pitch", &pitch, 0.995f * -90.0f, 0.995f * 90.0f);
		ImGui::SliderAngle("Yaw", &yaw, -180.0f, 180.0f);
		if (ImGui::Button("Reset"))
		{
			Reset();
		}
	}
	ImGui::End();
}

void Camera::Reset() noexcept
{
	pos = { 0.0f,7.5f,-18.0f };
	pitch = 0.0f;
	yaw = 0.0f;
}

void Camera::Rotate(float dx, float dy) noexcept
{
	yaw = wrap_angle(yaw + dx * rotationSpeed);
	pitch = std::clamp(pitch + dy * rotationSpeed, 0.995f * -PI / 2.0f, 0.995f * PI / 2.0f);
}

void Camera::Translate(DirectX::XMFLOAT3 translation) noexcept
{
	dx::XMStoreFloat3(&translation, dx::XMVector3Transform(
		dx::XMLoadFloat3(&translation),
		dx::XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f) *
		dx::XMMatrixScaling(travelSpeed, travelSpeed, travelSpeed)
	));
	pos = {
		pos.x + translation.x,
		pos.y + translation.y,
		pos.z + translation.z
	};
}

void Camera::GetForwardVector(float& fx, float& fy, float& fz) const noexcept
{
	namespace dx = DirectX;

	const dx::XMVECTOR forwardBaseVector = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	const auto lookVector = dx::XMVector3Transform(forwardBaseVector,
		dx::XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f)
	);

	dx::XMFLOAT3 result;
	dx::XMStoreFloat3(&result, lookVector);
	fx = result.x;
	fy = result.y;
	fz = result.z;
}

void Camera::GetRightVector(float& rx, float& ry, float& rz) const noexcept
{
	namespace dx = DirectX;

	float fx, fy, fz;
	GetForwardVector(fx, fy, fz);

	// right = forward x worldUp
	dx::XMVECTOR forward = dx::XMVectorSet(fx, fy, fz, 0.0f);
	dx::XMVECTOR worldUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	dx::XMVECTOR right = dx::XMVector3Cross(worldUp, forward);
	right = dx::XMVector3Normalize(right);

	dx::XMFLOAT3 result;
	dx::XMStoreFloat3(&result, right);
	rx = result.x; ry = result.y; rz = result.z;
}

void Camera::GetUpVector(float& ux, float& uy, float& uz) const noexcept
{
	namespace dx = DirectX;

	float fx, fy, fz, rx, ry, rz;
	GetForwardVector(fx, fy, fz);
	GetRightVector(rx, ry, rz);

	// up = forward x right (or right x forward depending on handedness — verify visually)
	dx::XMVECTOR forward = dx::XMVectorSet(fx, fy, fz, 0.0f);
	dx::XMVECTOR right = dx::XMVectorSet(rx, ry, rz, 0.0f);
	dx::XMVECTOR up = dx::XMVector3Cross(forward, right);
	up = dx::XMVector3Normalize(up);

	dx::XMFLOAT3 result;
	dx::XMStoreFloat3(&result, up);
	ux = result.x; uy = result.y; uz = result.z;
}
