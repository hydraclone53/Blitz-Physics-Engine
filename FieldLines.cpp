#include "FieldLines.h"
#include <cmath>
#include <algorithm>
#include "imgui/imgui.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <iterator>
#include <cstring>
#include "Define.h"
namespace dx = DirectX;
namespace wrl = Microsoft::WRL;

void FieldLines::Controls()
{
    ImGui::Begin("Field lines");
    ImGui::SliderInt("lines", &nLines, 4, 128);
    ImGui::SliderFloat("seed radius", &seedRadius, 1.2f, 4.0f);
    ImGui::SliderFloat("step", &stepLen, 0.1f, 1.0f);
    ImGui::SliderInt("max steps", &maxSteps, 20, 600);
    ImGui::SliderFloat("max radius", &maxRadius, 5.0f, 200.0f);
    ImGui::Text("lines: %d", (int)lines.size());
    ImGui::Text("farthest point: %.2f", farthest);
    ImGui::Checkbox("show", &show);
    ImGui::ColorEdit3("color", color);
    ImGui::End();
}

void FieldLines::Update(const PhysicsWorld& world, const InterForce::RadiationForce& rad, int src)
{
    lines.clear();
    farthest = 0.0f;
    if (src < 0 || src >= world.count || world.charge[src] == 0) return;

    const double T = world.total_time;        // newest row is stamped at total_time
    const float sx = world.posX[src], sy = world.posY[src], sz = world.posZ[src];
    const float sign = (world.charge[src] > 0) ? 1.0f : -1.0f;   // lines always run away from the sphere
    const float golden = 2.39996323f;

    // unit direction of (sign * E) at a point; false if there is no field there yet
    auto dirAt = [&](float px, float py, float pz,
        float& dx, float& dy, float& dz, float& mag) -> bool
        {
            float Ex, Ey, Ez;
            if (!rad.ProbeE(world, src, px, py, pz, T, Ex, Ey, Ez)) return false;
            mag = std::sqrt(Ex * Ex + Ey * Ey + Ez * Ez);
            if (mag < eMin) return false;
            dx = sign * Ex / mag;
            dy = sign * Ey / mag;
            dz = sign * Ez / mag;
            return true;
        };

    lines.reserve(nLines);
    for (int i = 0; i < nLines; i++)
    {
        // seed: evenly spread point on a small sphere around the charge
        float y = 1.0f - 2.0f * (i + 0.5f) / (float)nLines;
        float rho = std::sqrt((std::max)(0.0f, 1.0f - y * y));
        float ang = i * golden;
        float px = sx + seedRadius * rho * std::cos(ang);
        float py = sy + seedRadius * y;
        float pz = sz + seedRadius * rho * std::sin(ang);

        std::vector<Point> line;
        line.reserve(maxSteps + 1);

        for (int s = 0; s < maxSteps; s++)
        {
            float d1x, d1y, d1z, m1;
            if (!dirAt(px, py, pz, d1x, d1y, d1z, m1)) break;
            if (s == 0) line.push_back({ px, py, pz, m1 });

            // RK2 (midpoint): look at the field half a step ahead, then take the full step
            float d2x, d2y, d2z, m2;
            if (!dirAt(px + 0.5f * stepLen * d1x,
                py + 0.5f * stepLen * d1y,
                pz + 0.5f * stepLen * d1z,
                d2x, d2y, d2z, m2)) break;

            px += stepLen * d2x;
            py += stepLen * d2y;
            pz += stepLen * d2z;

            float rx = px - sx, ry = py - sy, rz = pz - sz;
            float dist = std::sqrt(rx * rx + ry * ry + rz * rz);
            if (dist > maxRadius) break;

            line.push_back({ px, py, pz, m2 });
            if (dist > farthest) farthest = dist;
        }
        if (line.size() >= 2) lines.push_back(std::move(line));
    }
}
static const char* kLineVS = R"(
cbuffer CBuf : register(b0) { matrix transform; };
struct VSOut { float4 color : Color; float4 pos : SV_Position; };
VSOut main(float3 pos : Position, float4 color : Color)
{
    VSOut o;
    o.pos = mul(float4(pos, 1.0f), transform);
    o.color = color;
    return o;
}
)";

static const char* kLinePS = R"(
float4 main(float4 color : Color) : SV_Target { return color; }
)";

void FieldLines::CreateResources(Graphics& gfx)
{
    HRESULT hr;
    ID3D11Device* dev = gfx.GetDevice();

    // vertex shader + input layout
    wrl::ComPtr<ID3DBlob> pBlob, pErr;
    hr = D3DCompile(kLineVS, strlen(kLineVS), "LineVS", nullptr, nullptr,
        "main", "vs_5_0", 0, 0, &pBlob, &pErr);
    if (FAILED(hr))
        throw std::runtime_error(pErr ? (const char*)pErr->GetBufferPointer() : "VS compile failed");
    GFX_THROW_NOINFO(dev->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVS));

    const D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        { "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "Color",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    GFX_THROW_NOINFO(dev->CreateInputLayout(ied, (UINT)std::size(ied),
        pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &pLayout));

    // pixel shader
    hr = D3DCompile(kLinePS, strlen(kLinePS), "LinePS", nullptr, nullptr,
        "main", "ps_5_0", 0, 0, &pBlob, &pErr);
    if (FAILED(hr))
        throw std::runtime_error(pErr ? (const char*)pErr->GetBufferPointer() : "PS compile failed");
    GFX_THROW_NOINFO(dev->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPS));

    // constant buffer: one matrix
    D3D11_BUFFER_DESC cbd = {};
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.ByteWidth = sizeof(dx::XMMATRIX);
    GFX_THROW_NOINFO(dev->CreateBuffer(&cbd, nullptr, &pCB));

    // depth: lines hide behind spheres but don't write depth themselves
    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsd.DepthFunc = D3D11_COMPARISON_LESS;
    GFX_THROW_NOINFO(dev->CreateDepthStencilState(&dsd, &pDSS));

    // standard alpha blending
    D3D11_BLEND_DESC bld = {};
    auto& rt = bld.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rt.BlendOp = D3D11_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D11_BLEND_ONE;
    rt.DestBlendAlpha = D3D11_BLEND_ZERO;
    rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    GFX_THROW_NOINFO(dev->CreateBlendState(&bld, &pBlend));

    resourcesReady = true;
}

// step 6: the dynamic vertex buffer grows when more vertices are needed
void FieldLines::EnsureCapacity(Graphics& gfx, UINT need)
{
    if (pVB && need <= vbCapacity) return;

    UINT cap = vbCapacity + vbCapacity / 2;
    if (cap < need)   cap = need;
    if (cap < 1024u)  cap = 1024u;

    HRESULT hr;
    D3D11_BUFFER_DESC bd = {};
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.ByteWidth = cap * (UINT)sizeof(Vertex);
    pVB.Reset();
    GFX_THROW_NOINFO(gfx.GetDevice()->CreateBuffer(&bd, nullptr, &pVB));
    vbCapacity = cap;
}

// step 5: polylines -> line-list vertices, color from |E|
void FieldLines::BuildVertices()
{
    verts.clear();
    const float logFar = std::log10(eFar);
    const float logNear = std::log10(eNear);

    auto setColor = [&](Vertex& v, float mag)
        {
            float t = (std::log10(mag) - logFar) / (logNear - logFar);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            float shade = 0.35f + 0.65f * t;
            v.r = color[0] * shade;
            v.g = color[1] * shade;
            v.b = color[2] * shade;
            v.a = 0.25f + 0.75f * t;
        };

    for (const auto& line : lines)
    {
        for (size_t i = 0; i + 1 < line.size(); i++)
        {
            Vertex a, b;
            a.x = line[i].x;     a.y = line[i].y;     a.z = line[i].z;
            b.x = line[i + 1].x; b.y = line[i + 1].y; b.z = line[i + 1].z;
            setColor(a, line[i].mag);
            setColor(b, line[i + 1].mag);
            verts.push_back(a);
            verts.push_back(b);
        }
    }
}

// step 7: upload + draw
void FieldLines::Draw(Graphics& gfx)
{
    if (!show) return;
    BuildVertices();
    if (verts.empty()) return;

    if (!resourcesReady) CreateResources(gfx);
    EnsureCapacity(gfx, (UINT)verts.size());

    HRESULT hr;
    ID3D11DeviceContext* ctx = gfx.GetContext();

    // vertices
    D3D11_MAPPED_SUBRESOURCE msr;
    GFX_THROW_NOINFO(ctx->Map(pVB.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr));
    std::memcpy(msr.pData, verts.data(), verts.size() * sizeof(Vertex));
    ctx->Unmap(pVB.Get(), 0u);

    // transform: points are already in world space, so just camera * projection
    GFX_THROW_NOINFO(ctx->Map(pCB.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr));
    const dx::XMMATRIX m = dx::XMMatrixTranspose(gfx.GetCamera() * gfx.GetProjection());
    std::memcpy(msr.pData, &m, sizeof(m));
    ctx->Unmap(pCB.Get(), 0u);

    // remember the states we change, so spheres and ImGui are unaffected
    wrl::ComPtr<ID3D11DepthStencilState> oldDSS;
    UINT oldRef = 0u;
    ctx->OMGetDepthStencilState(&oldDSS, &oldRef);
    wrl::ComPtr<ID3D11BlendState> oldBlend;
    FLOAT oldFactor[4] = {};
    UINT oldMask = 0xFFFFFFFFu;
    ctx->OMGetBlendState(&oldBlend, oldFactor, &oldMask);

    const UINT stride = sizeof(Vertex);
    const UINT offset = 0u;
    ctx->IASetVertexBuffers(0u, 1u, pVB.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(pLayout.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    ctx->VSSetShader(pVS.Get(), nullptr, 0u);
    ctx->VSSetConstantBuffers(0u, 1u, pCB.GetAddressOf());
    ctx->PSSetShader(pPS.Get(), nullptr, 0u);
    ctx->OMSetDepthStencilState(pDSS.Get(), 1u);
    const FLOAT blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->OMSetBlendState(pBlend.Get(), blendFactor, 0xFFFFFFFFu);

    ctx->Draw((UINT)verts.size(), 0u);

    ctx->OMSetDepthStencilState(oldDSS.Get(), oldRef);
    ctx->OMSetBlendState(oldBlend.Get(), oldFactor, oldMask);
}