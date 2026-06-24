#include "Cube.h"
#include "BindableBase.h"

namespace dx = DirectX;

Cube::Cube(Graphics& gfx,
    std::mt19937& rng,
    std::uniform_real_distribution<float>& adist,
    std::uniform_real_distribution<float>& ddist,
    std::uniform_real_distribution<float>& odist,
    std::uniform_real_distribution<float>& rdist,
    std::uniform_int_distribution<int>& ch)
    :

    pGfx(&gfx),
    q(ch(rng)),
    x(rdist(rng)),
    y(rdist(rng)),
    z(rdist(rng)),
    dx(0.0f),
    dy(0.0f),
    dz(0.0f),
    body(x, y, z, dx, dy, dz, q, mass)
{
    if (!IsStaticInitialized())
    {
        struct Vertex { dx::XMFLOAT3 pos; };

        // Define the 8 corners of a cube
        const float s = 0.5f; // half-size
        const std::vector<Vertex> vertices =
        {
            {{-s,-s,-s}}, {{ s,-s,-s}}, {{-s, s,-s}}, {{ s, s,-s}},
            {{-s,-s, s}}, {{ s,-s, s}}, {{-s, s, s}}, {{ s, s, s}}
        };

        AddStaticBind(std::make_unique<VertexBuffer>(gfx, vertices));

        // Define the 12 triangles (2 per face)
        const std::vector<unsigned short> indices =
        {
            0,2,1, 2,3,1, 1,3,5, 3,7,5, 2,6,3, 3,6,7,
            4,5,7, 4,7,6, 0,4,2, 2,4,6, 0,1,4, 1,5,4
        };
        AddStaticIndexBuffer(std::make_unique<IndexBuffer>(gfx, indices));

        auto pvs = std::make_unique<VertexShader>(gfx, L"VertexShader.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));
        AddStaticBind(std::make_unique<PixelShader>(gfx, L"PixelShader.cso"));

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

    float red = 0.0f;
    float blue = 0.0f;
    float green = 0.0f;
    if (q == 1) {
        red = 1.0f;
        blue = 0.0f;
        green = 0.0f;
    }
    else if (q == -1) {
        red = 0.0f;
        blue = 1.0f;
        green = 0.0f;
    }
    else {
        red = 0.0f;
        blue = 0.0f;
        green = 1.0f;
    }
    Cube::ConstantBuffer2 cb2 =
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
    AddBind(std::make_unique<PixelConstantBuffer<ConstantBuffer2>>(gfx, cb2, 0u));
    AddBind(std::make_unique<TransformCbuf>(gfx, *this));
}

void Cube::Update(float dt) noexcept {
    body.AddEtotal();
    body.StepBoris(dt);
    body.ResetAccumulators();


}

dx::XMMATRIX Cube::GetTransformXM() const noexcept
{
    return DirectX::XMMatrixTranslation(body.GetX(), body.GetY(), body.GetZ());
}