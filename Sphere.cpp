#include "Sphere.h"
#include "BindableBase.h"
#include "Define.h"
#include "Fields.h"
#include"ConstantBuffers.h"
namespace dx = DirectX;


Sphere::Sphere(Graphics& gfx,
	std::mt19937& rng,
	std::uniform_real_distribution<float>& adist,
	std::uniform_real_distribution<float>& ddist,
	std::uniform_real_distribution<float>& odist,
	std::uniform_real_distribution<float>& rdist,
	std::uniform_int_distribution<int>& ch)
	:
	
	pGfx(&gfx),
	q(ch(rng)),
	r(rdist(rng)),
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
	float x = rdist(rng);
	float y = rdist(rng);
	float z = rdist(rng);
	float di = 0.0f;
	float dj = 0.0f;
	float dk = 0.0f;
	body = PhysicsBody(x, y, z, di, dj, dk, q, mass);

	if (!IsStaticInitialized())
	{
		struct Vertex
		{
			struct
			{
				float x;
				float y;
				float z;
			} pos;
		};
		// --- 1. SPHERICAL GEOMETRY GENERATION ---
		const int latDiv = 20;   // Vertical resolution (tessellation)
		const int longDiv = 20;  // Horizontal resolution
		float radius = 0.5f;
		const int radDiv = 20;
		std::vector<Vertex> vertices;

		// Latitude loop (Phi goes from 0 to PI, North Pole to South Pole)
		for (int i = 0; i <= latDiv; ++i)
		{
			float phi = dx::XM_PI * (float)i / (float)latDiv;

			// Longitude loop (Theta goes from 0 to 2*PI, wrapping around the equator)
			for (int j = 0; j <= longDiv; ++j)
			{


				float theta = dx::XM_2PI * (float)j / (float)longDiv;
				Vertex v;
				// Convert Spherical Coordinates to Cartesian (X, Y, Z)

				v.pos.x = radius * sinf(phi) * cosf(theta);
				v.pos.y = radius * sinf(phi) * sinf(theta);
				v.pos.z = radius * cosf(phi);
				vertices.push_back(v);


			}
		};
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
	
	
	qb.polarizationDirection = { 0.0f,0.0f,0.0f };
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

	float ex = dx::XMVectorGetX(body.GetEtotal());
	float ey = dx::XMVectorGetY(body.GetEtotal());
	float ez = dx::XMVectorGetZ(body.GetEtotal());


	body.AddEtotal();
	UpdatePixelColor(*pGfx, body.GetEtotal());
	body.StepBoris(dt);
	UpdateGlow(*pGfx);
	body.ResetAccumulators();
}

DirectX::XMMATRIX Sphere::GetTransformXM() const noexcept
{
	return DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll) *
		DirectX::XMMatrixRotationRollPitchYaw(theta, phi, chi) * 
		DirectX::XMMatrixTranslation(body.GetX(), body.GetY(), body.GetZ());
}

void Sphere::UpdatePixelColor(Graphics& gfx, DirectX::XMVECTOR Etot) noexcept
{
	DirectX::XMVECTOR v = Etot;
	DirectX::XMFLOAT3 f3;
	DirectX::XMStoreFloat3(&f3, v);
	if (pPolarCbuf) {
		ConstantBufferQ cb1 = {};
		
		cb1.polarizationDirection = f3;
		cb1.padding=0.0f; // Ensure 16-byte alignment
		
		pPolarCbuf->Update(gfx, cb1);
	}
}

void Sphere::UpdateGlow(Graphics& gfx) noexcept
{
	if (pGlowCbuf)
	{
		ConstantBufferGlow cb = {};
		cb.glowIntensity = body.GetRadiationIntensity();
		cb.padding[0] = cb.padding[1] = cb.padding[2] = 0.0f;
		pGlowCbuf->Update(gfx, cb);
	}
}