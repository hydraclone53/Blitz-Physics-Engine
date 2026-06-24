#pragma once
#include "DrawableBase.h"
#include "Fields.h"
#include <algorithm>
#include "ConstantBuffers.h"
#include "Graphics.h"
#include "PhysicsBody.h"


class Cube : public DrawableBase<Cube>
{
public:
    Cube(Graphics& gfx, std::mt19937& rng,
        std::uniform_real_distribution<float>& adist,
        std::uniform_real_distribution<float>& ddist,
        std::uniform_real_distribution<float>& odist,
        std::uniform_real_distribution<float>& rdist,
        std::uniform_int_distribution<int>& ch);
    void Update(float dt) noexcept override;
    DirectX::XMMATRIX GetTransformXM() const noexcept override;
    PhysicsBody* GetPhysicsBody() noexcept { return &body; }
private:
    struct ConstantBufferQ {
        DirectX::XMFLOAT3 polarizationDirection; // The direction of the E-field
        float padding;
    };
    Cube::ConstantBufferQ qb;
    struct ConstantBuffer2{
        struct { float r, g, b, a; } face_colors[6];
        int renderMode;
        float padding[3];
    };
    Graphics* pGfx = nullptr;
    float x, y, z;
    float dx, dy, dz;
    float mass = 1.0f;
    int q = 1;
    PhysicsBody body;
    PixelConstantBuffer<ConstantBuffer2>* pColorCbuf = nullptr;
    PixelConstantBuffer<ConstantBufferQ>* pPolarCbuf = nullptr;
    };