#include "Sphere.h"
#include "BindableBase.h"
#include "Define.h"
#include "Fields.h"
#include"ConstantBuffers.h"
namespace dx = DirectX;


Sphere::Sphere(Graphics& gfx,
	PhysicsWorld& world,
	int bodyIndex,
	std::uniform_real_distribution<float>& adist,
	std::uniform_real_distribution<float>& ddist,
	std::uniform_real_distribution<float>& odist,
	std::mt19937& rng,
	bool isTempPosition)
	:
	
	pGfx(&gfx),
	pWorld(&world),
	bodyIndex(bodyIndex),
	useTempPosition(isTempPosition),
	q(isTempPosition ? 0 : world.charge[bodyIndex]),
	r(1.0f),
	droll(ddist(rng)),
	dpitch(ddist(rng)),
	dyaw(ddist(rng)),
	dphi(odist(rng)),
	dtheta(odist(rng)),
	dchi(odist(rng))
	//chi(adist(rng)),
	//theta(adist(rng)),
	//phi(adist(rng)),
	
{

	if (!IsStaticInitialized())
	{
		struct Vertex
		{
			float x, y, z;   // position
			float nx, ny, nz;  // normal
		};
		static_assert(sizeof(Vertex) == 24u, "Vertex size is wrong");

		// --- 1. SPHERICAL GEOMETRY GENERATION ---
		const int latDiv = 20;
		const int longDiv = 20;
		const float radius = 0.5f;
		std::vector<Vertex> vertices;

		for (int i = 0; i <= latDiv; ++i)
		{
			float phi = dx::XM_PI * (float)i / (float)latDiv;

			for (int j = 0; j <= longDiv; ++j)
			{
				float theta = dx::XM_2PI * (float)j / (float)longDiv;

				// unit normal — direction from sphere center to surface point
				float nx = sinf(phi) * cosf(theta);
				float ny = sinf(phi) * sinf(theta);
				float nz = cosf(phi);

				Vertex v;
				v.x = radius * nx;   // position = normal * radius
				v.y = radius * ny;
				v.z = radius * nz;
				v.nx = nx;             // normal = unit direction
				v.ny = ny;
				v.nz = nz;
				vertices.push_back(v);
			}
		}
		AddStaticBind(std::make_unique<VertexBuffer>(gfx, vertices));

		auto pvs = std::make_unique<VertexShader>(gfx, L"VertexShader.cso");
		auto pvsbc = pvs->GetBytecode();
		AddStaticBind(std::move(pvs));

		AddStaticBind(std::make_unique<PixelShader>(gfx, L"PixelShader.cso"));

		// --- 3. SEW THE INDEX GRID INTO TRIANGLES ---
		std::vector<unsigned short> indices;
		const int numColumns = longDiv + 1; // Number of vertices per horizontal ring

		for (int i = 0; i < latDiv; ++i)
		{
			for (int j = 0; j < longDiv; ++j)
			{
				// Gather the index addresses of 4 neighboring vertices forming a grid square
				unsigned short topLeft = i * numColumns + j;
				unsigned short topRight = topLeft + 1;
				unsigned short bottomLeft = (i + 1) * numColumns + j;
				unsigned short bottomRight = bottomLeft + 1;

				// Triangle 1
				indices.push_back(topLeft);
				indices.push_back(topRight);
				indices.push_back(bottomLeft);

				// Triangle 2
				indices.push_back(topRight);
				indices.push_back(bottomRight);
				indices.push_back(bottomLeft);
			}
		}
		AddStaticIndexBuffer(std::make_unique<IndexBuffer>(gfx, indices));


		const std::vector<D3D11_INPUT_ELEMENT_DESC> ied =
		{
			{ "Position",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		AddStaticBind(std::make_unique<InputLayout>(gfx, ied, pvsbc));

		AddStaticBind(std::make_unique<Topology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
	}
	else
	{
		SetIndexFromStatic();
	}
	
	
	qb.polarizationDirection = { 1.0f,0.0f,0.0f };
	qb.padding = 0.0f;
	auto pCbuf = std::make_unique<PixelConstantBuffer<ConstantBufferQ>>(gfx, qb, 1u);
	pPolarCbuf = pCbuf.get();
	AddBind(std::move(pCbuf));
	ConstantBufferGlow glowInit = { 0.0f, {0.0f, 0.0f, 0.0f} };
	auto pGlowBuf = std::make_unique<PixelConstantBuffer<ConstantBufferGlow>>(gfx, glowInit, 2u);
	pGlowCbuf = pGlowBuf.get();
	AddBind(std::move(pGlowBuf));

	float red = 0.0f;
	float blue = 0.0f;
	float green = 0.0f;
	if (q == 1) {
		red = 1.0f;
		blue = 0.0f;
		green = 0.0f;
	}
	else if (q==-1) {
		red = 0.0f;
		blue = 1.0f;
		green = 0.0f;
	}
	else {
		red = 0.0f;
		blue = 0.0f;
		green =1.0f;
	}
	Sphere::ConstantBuffer2 cb2 =
	{
		{
			{ red,green,blue },
			{ red,green,blue },
			{ red,green,blue },
			{ red,green,blue },
			{ red,green,blue },
			{ red,green,blue },
			
		}
		
	};
	if (q == 0) {
		cb2.renderMode = 1; // Gradient for neutral
	}
	else {
		cb2.renderMode = 0; // Solid colors for charged
	}
	cb2.padding[0] = 0.0f;
	cb2.padding[1] = 0.0f;
	cb2.padding[2] = 0.0f;
	AddBind(std::make_unique<PixelConstantBuffer<ConstantBuffer2>>(gfx, cb2,0u));

	 // Save the pointer
	

	AddBind(std::make_unique<TransformCbuf>(gfx, *this));
};

void Sphere::Update(float dt) noexcept
{
	roll += 0.0f; // droll * dt;
	pitch += 0.0f; // dpitch* dt;
	yaw += 0.0f; // dyaw * dt;
	dtheta += 0.0f;// 1.0f * dt;
	theta += 0.0f;//  dtheta* dt;
	phi += 0.0f; //  dphi* dt;
	chi += 0.0f; // dchi* dt;

	namespace dx = DirectX;
	if (useTempPosition) return;
	// update polarization shader for neutral spheres
	if (q == 0)
		UpdatePixelColor(*pGfx);

	// update glow shader for charged spheres
	if (q != 0)
		UpdateGlow(*pGfx);

}
/*DirectX::XMMATRIX Sphere::GetTransformXM() const noexcept
{
	namespace dx = DirectX;

	// get velocity direction from physics body
	dx::XMVECTOR vel = body.GetVelVector();
	float v_sq = dx::XMVectorGetX(dx::XMVector3LengthSq(vel));

	// build contraction matrix along velocity direction
	dx::XMMATRIX contractionMatrix = dx::XMMatrixIdentity();

	if (v_sq > 1e-6f)
	{
		float gamma = body.GetGamma();
		float contraction = 1.0f / gamma;  // < 1.0 when moving fast

		// normalized velocity direction
		float v_mag = std::sqrt(v_sq);
		float vx = dx::XMVectorGetX(vel) / v_mag;
		float vy = dx::XMVectorGetY(vel) / v_mag;
		float vz = dx::XMVectorGetZ(vel) / v_mag;

		// contraction matrix: M = I + (1/gamma - 1)(v_hat x v_hat)
		// scales by 1/gamma along velocity, leaves perpendicular unchanged
		float f = contraction - 1.0f;  // negative — so it shrinks

		contractionMatrix.r[0] = dx::XMVectorSet(1.0f + f * vx * vx, f * vx * vy, f * vx * vz, 0.0f);
		contractionMatrix.r[1] = dx::XMVectorSet(f * vy * vx, 1.0f + f * vy * vy, f * vy * vz, 0.0f);
		contractionMatrix.r[2] = dx::XMVectorSet(f * vz * vx, f * vz * vy, 1.0f + f * vz * vz, 0.0f);
		contractionMatrix.r[3] = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	}

	// contraction applied first in model space, then rotate, then translate
	return contractionMatrix *
		dx::XMMatrixRotationRollPitchYaw(pitch, yaw, roll) *
		dx::XMMatrixRotationRollPitchYaw(theta, phi, chi) *
		dx::XMMatrixTranslation(body.GetX(), body.GetY(), body.GetZ());
}
DirectX::XMMATRIX Sphere::GetTransformXM() const noexcept
{
	return DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll) *
		DirectX::XMMatrixRotationRollPitchYaw(theta, phi, chi) * 
		DirectX::XMMatrixTranslation(body.GetX(), body.GetY(), body.GetZ());
} 
DirectX::XMMATRIX Sphere::GetTransformXM() const noexcept
{
	namespace dx = DirectX;

	float vx, vy, vz;
	pWorld->GetVelocity(bodyIndex, vx, vy, vz);
	float v_sq = vx * vx + vy * vy + vz * vz;

	dx::XMMATRIX contractionMatrix = dx::XMMatrixIdentity();

	if (v_sq > 1e-6f)
	{
		float gamma = pWorld->gamma[bodyIndex];
		float contraction = 1.0f / gamma;
		float v_mag = std::sqrt(v_sq);
		float nvx = vx / v_mag;
		float nvy = vy / v_mag;
		float nvz = vz / v_mag;
		float f = contraction - 1.0f;

		contractionMatrix.r[0] = dx::XMVectorSet(1.0f + f * nvx * nvx, f * nvx * nvy, f * nvx * nvz, 0.0f);
		contractionMatrix.r[1] = dx::XMVectorSet(f * nvy * nvx, 1.0f + f * nvy * nvy, f * nvy * nvz, 0.0f);
		contractionMatrix.r[2] = dx::XMVectorSet(f * nvz * nvx, f * nvz * nvy, 1.0f + f * nvz * nvz, 0.0f);
		contractionMatrix.r[3] = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	}

	return contractionMatrix *
		dx::XMMatrixRotationRollPitchYaw(pitch, yaw, roll) *
		dx::XMMatrixRotationRollPitchYaw(theta, phi, chi) *
		dx::XMMatrixTranslation(pWorld->posX[bodyIndex],
			pWorld->posY[bodyIndex],
			pWorld->posZ[bodyIndex]);
}*/
DirectX::XMMATRIX Sphere::GetTransformXM() const noexcept
{
	namespace dx = DirectX;

	float px, py, pz;
	if (useTempPosition)
	{
		px = pWorld->tempPosX[bodyIndex];
		py = pWorld->tempPosY[bodyIndex];
		pz = pWorld->tempPosZ[bodyIndex];
	}
	else
	{
		px = pWorld->posX[bodyIndex];
		py = pWorld->posY[bodyIndex];
		pz = pWorld->posZ[bodyIndex];
	}

	dx::XMMATRIX contractionMatrix = dx::XMMatrixIdentity();

	if (!useTempPosition)
	{
		float vx, vy, vz;
		pWorld->GetVelocity(bodyIndex, vx, vy, vz);
		float v_sq = vx * vx + vy * vy + vz * vz;

		if (v_sq > 1e-6f)
		{
			float gamma = pWorld->gamma[bodyIndex];
			float contraction = 1.0f / gamma;
			float v_mag = std::sqrt(v_sq);
			float nvx = vx / v_mag;
			float nvy = vy / v_mag;
			float nvz = vz / v_mag;
			float f = contraction - 1.0f;

			contractionMatrix.r[0] = dx::XMVectorSet(1.0f + f * nvx * nvx, f * nvx * nvy, f * nvx * nvz, 0.0f);
			contractionMatrix.r[1] = dx::XMVectorSet(f * nvy * nvx, 1.0f + f * nvy * nvy, f * nvy * nvz, 0.0f);
			contractionMatrix.r[2] = dx::XMVectorSet(f * nvz * nvx, f * nvz * nvy, 1.0f + f * nvz * nvz, 0.0f);
			contractionMatrix.r[3] = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}

	return contractionMatrix *
		dx::XMMatrixRotationRollPitchYaw(pitch, yaw, roll) *
		dx::XMMatrixRotationRollPitchYaw(theta, phi, chi) *
		dx::XMMatrixTranslation(px, py, pz);
}
void Sphere::UpdatePixelColor(Graphics& gfx) noexcept
{
	if (!pPolarCbuf) return;
	ConstantBufferQ cb = {};
	cb.polarizationDirection = {
		pWorld->eTotalX[bodyIndex],
		pWorld->eTotalY[bodyIndex],
		pWorld->eTotalZ[bodyIndex]
	};
	cb.padding = 0.0f;
	pPolarCbuf->Update(gfx, cb);
}

void Sphere::UpdateGlow(Graphics& gfx) noexcept
{
	if (!pGlowCbuf) return;
	ConstantBufferGlow cb = {};
	cb.glowIntensity = pWorld->GetRadiationIntensity(bodyIndex);
	pGlowCbuf->Update(gfx, cb);
}