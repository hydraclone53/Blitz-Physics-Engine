cbuffer ConstantBuffer2 : register(b0)
{
    float4 face_colors[6];
    int renderMode; // 0 = Face Colors, 1 = Polarization, 2 = Both
    float3 padding;
};

cbuffer ConstantBufferQ : register(b1)
{
    float3 polarDir;
    float padding2; // Ensure 16-byte alignment
};
cbuffer ConstantBufferGlow : register(b2)
{
    float glowIntensity;
    float3 glowPadding;
};

struct PSIn
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float3 localNormal : LOCAL_NORMAL;
};


float4 main(PSIn input, uint tid : SV_PrimitiveID) : SV_Target
{
    
    float4 finalColor;
    
    if (renderMode == 0)
    {
        finalColor = face_colors[(tid / 2) % 6];
    }
    else if (renderMode == 1)
    {
       
       // 1. Measure the strength of the polarization field
        float fieldStrength = length(polarDir);
        
        if (fieldStrength < 0.01f)
        {
            finalColor = float4(0.0f, 1.0f, 0.0f, 1.0f); // Solid Green
        }
        else
        {
            // 2. Polarized state: Multi-band gradient (Blue -> Green -> Red)
            float3 normalizedPolarDir = polarDir / fieldStrength;
            float dotProduct = dot(normalize(input.normal), normalizedPolarDir);
            
            // lerpVal goes from 0.0 (opposite tip) to 1.0 (aligned tip). 0.5 is the middle equator.
            float lerpVal = dotProduct * 0.5f + 0.5f;
            
            float4 blue = float4(0.0f, 0.0f, 1.0f, 1.0f);
            float4 green = float4(0.0f, 1.0f, 0.0f, 1.0f);
            float4 red = float4(1.0f, 0.0f, 0.0f, 1.0f);
            
            if (lerpVal < 0.5f)
            {
                // First half: Remap [0.0, 0.5] to [0.0, 1.0] for blending Blue to Green
                float t = lerpVal * 10.0f; 
                finalColor = lerp(blue, green, t);
            }
            else
            {
                // Second half: Remap [0.5, 1.0] to [0.0, 1.0] for blending Green to Red
                float t = (lerpVal - 0.5f) * 1.2f;
                finalColor = lerp(green, red, t);
            }
        }
    }
    else
    {
        // Blend both
        float4 face = face_colors[(tid / 2) % 6];
        float dotProduct = dot(normalize(input.normal), normalize(polarDir));
        float lerpVal = dotProduct * 0.5f + 0.5f;
        finalColor = face * float4(lerpVal, 0.0f, 1.0f - lerpVal, 1.0f);
    }  
    float4 glowColor = float4(1.0f, 0.9f, 0.6f, 1.0f); // warm white, like a hot filament
    finalColor = lerp(finalColor, glowColor, glowIntensity);
    return finalColor;
}