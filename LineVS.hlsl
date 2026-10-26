cbuffer CBuf : register(b0)
{
    matrix transform;
};

struct VSOut
{
    float4 color : Color;
    float4 pos : SV_Position;
};

VSOut main(float3 pos : Position, float4 color : Color)
{
    VSOut o;
    o.pos = mul(float4(pos, 1.0f), transform);
    o.color = color;
    return o;
}