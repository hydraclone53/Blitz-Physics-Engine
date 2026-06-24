#include "Box.h"
#include "BindableBase.h"
#include "Define.h"
namespace dx = DirectX;
Box::Box(Graphics& gfx,
	std::mt19937& rng,
	std::uniform_real_distribution<float>& adist,
	std::uniform_real_distribution<float>& ddist,
	std::uniform_real_distribution<float>& odist,
	std::uniform_real_distribution<float>& rdist)
	:
	r(rdist(rng)),
	droll(ddist(rng)),
	dpitch(ddist(rng)),
	dyaw(ddist(rng)),
	dphi(odist(rng)),
	dtheta(odist(rng)),
	dchi(odist(rng)),
	//chi(adist(rng)),
	//theta(adist(rng)),
	//phi(adist(rng)),
	i(rdist(rng)),
	//j(rdist(rng)),
	j(5.0f),
	k(rdist(rng)),
	di(odist(rng)),
	//dj(odist(rng)),
	dk(odist(rng))

{
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

		struct ConstantBuffer2
		{
			struct
			{
				float r;
				float g;
				float b;
				float a;
			} face_colors[6];
		};
		const ConstantBuffer2 cb2 =
		{
			{
				{ 1.0f,0.0f,1.0f },
				{ 1.0f,0.0f,0.0f },
				{ 0.0f,1.0f,0.0f },
				{ 0.0f,0.0f,1.0f },
				{ 1.0f,1.0f,0.0f },
				{ 0.0f,1.0f,1.0f },
			}
		};
		AddStaticBind(std::make_unique<PixelConstantBuffer<ConstantBuffer2>>(gfx, cb2));

		const std::vector<D3D11_INPUT_ELEMENT_DESC> ied =
		{
			{ "Position",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
		};
		AddStaticBind(std::make_unique<InputLayout>(gfx, ied, pvsbc));

		AddStaticBind(std::make_unique<Topology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
	}
	else
	{
		SetIndexFromStatic();
	}

	AddBind(std::make_unique<TransformCbuf>(gfx, *this));
};

void Box::Update(float dt) noexcept
{
	roll += 0.0f; // droll * dt;
	pitch += 0.0f; // dpitch* dt;
	yaw += 0.0f; // dyaw * dt;
	dtheta += 0.0f;// 1.0f * dt;
	theta += 0.0f;//  dtheta* dt;
	phi += 0.0f; //  dphi* dt;
	chi += 0.0f; // dchi* dt;
	i += 0.0f;// di* dt;
	float accel = -1.0f;
	j += dj * dt+0.5f* accel *dt*dt;
	dj += accel * dt*0.5f;
	
	
	if (j < -10.0f)
	{
		j = -10.0f;      
		dj = -dj*1.01f;      
	}
	dj += 0.5f * accel * dt;
	k += 0.0f;//0.0f; // dk * dt;

	

}

DirectX::XMMATRIX Box::GetTransformXM() const noexcept
{
	return DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll) *
		//DirectX::XMMatrixTranslation(i,j,k) *
		DirectX::XMMatrixRotationRollPitchYaw(theta, phi, chi) *
		DirectX::XMMatrixTranslation(i,j, k+20.0f);
}

float Box::GetMomentum() noexcept
{
	return mass * dj; 
}
