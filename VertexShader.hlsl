

cbuffer CBuf
{
    matrix modelViewProj; // full MVP
    matrix world; // world only — for normals

   
};


struct VSOut 
{
    float4 pos : SV_Position;
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL;
    // Add this new field for the E-field calculation
    float3 localNormal : LOCAL_NORMAL; 
    // ... any other fields you have
};

VSOut main(float3 pos : Position, float3 n : NORMAL) 
{
    VSOut vso = (VSOut) 0;
    vso.pos = mul(float4(pos, 1.0f), modelViewProj);
    vso.normal = n;
    vso.localNormal = n;
    vso.worldPos = float3(0.3f, 0.0f, 0.0f);

    return vso;
}