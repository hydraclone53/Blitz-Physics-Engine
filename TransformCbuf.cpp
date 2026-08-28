#include "TransformCbuf.h"

TransformCbuf::TransformCbuf(Graphics& gfx, const Drawable& parent)
	:
	parent(parent)
{
	
    pVcbuf = std::make_unique<VertexConstantBuffer<Transforms>>(gfx);
	
}

void TransformCbuf::Bind(Graphics& gfx) noexcept
{
    const auto model = parent.GetTransformXM();

    Transforms t;
    // full MVP — used for vertex position
    t.modelViewProj = DirectX::XMMatrixTranspose(
        model * gfx.GetCamera() * gfx.GetProjection()
    );
    // world matrix only — used for normal transform, no camera/projection
    // inverse transpose handles non-uniform scaling (length contraction)
    t.world = model; ;

    pVcbuf->Update(gfx, t);
    pVcbuf->Bind(gfx);
}
