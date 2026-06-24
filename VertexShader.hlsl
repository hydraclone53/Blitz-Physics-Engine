

cbuffer CBuf
{
    matrix modelViewProj; // full MVP
    matrix world; // world only — for normals

   
};


struct VSOut 
{
    float4 pos : SV_Position;
    float3 normal : Normal;
    // Add this new field for the E-field calculation
    float3 localNormal : LOCAL_NORMAL; 
    // ... any other fields you have
};

VSOut main(float3 pos : Position, float3 n : Normal) 
{
    VSOut vso = (VSOut) 0;
    // ... your existing position math ...
    vso.pos = mul(float4(pos, 1.0f), modelViewProj);
    vso.normal = mul(n, (float3x3) world);  

    // ADD THIS: Pass the raw, unmodified Model Space normal
    vso.localNormal = n; 

    return vso;
}