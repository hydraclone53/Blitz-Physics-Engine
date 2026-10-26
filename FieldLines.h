#pragma once
#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include "Graphics.h"
#include "SystemForce.h"
#include "PhysicsWorld.h"      

class FieldLines
{
public:
    struct Point { float x, y, z, mag; };   // mag = |E| there, used for color

    void Update(const PhysicsWorld& world, const InterForce::RadiationForce& rad, int src);
    void Controls();                         // ImGui sliders + stats
    void Draw(Graphics& gfx);                // builds vertices, uploads, draws

    const std::vector<std::vector<Point>>& GetLines() const noexcept { return lines; }

    int   nLines = 32;
    float seedRadius = 1.5f;
    float stepLen = 0.4f;
    int   maxSteps = 200;
    float maxRadius = 60.0f;
    float eMin = 1e-3f;

    bool  show = true;
    float color[3] = { 1.0f, 0.75f, 0.2f };
    float eFar = 0.02f;                    // |E| shown dimmest
    float eNear = 25.0f;                    // |E| shown brightest

private:
    struct Vertex { float x, y, z; float r, g, b, a; };

    void CreateResources(Graphics& gfx);
    void EnsureCapacity(Graphics& gfx, UINT vertexCount);
    void BuildVertices();

    std::vector<std::vector<Point>> lines;
    float farthest = 0.0f;

    std::vector<Vertex> verts;
    UINT vbCapacity = 0;
    bool resourcesReady = false;

    Microsoft::WRL::ComPtr<ID3D11Buffer>            pVB;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            pCB;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>      pVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>       pPS;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       pLayout;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> pDSS;
    Microsoft::WRL::ComPtr<ID3D11BlendState>        pBlend;
};