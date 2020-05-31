struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float2 uv : TEXCOORD;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    float4 colour : SV_Target0;
};

PSInput VSMain(VSInput input)
{
    PSInput result;

    result.position = float4(input.position.xyz, 1);
    result.uv = input.uv;

    return result;
}

Texture2D<float4> lightDiffuseBuffer : register(t0);
Texture2D<float4> lightSpecularBuffer : register(t1);

//SamplerState SamplerLinear : register(s0);


PSOutput PSMain(PSInput input)
{
    PSOutput output = (PSOutput) 0;

    float3 diffuse = lightDiffuseBuffer[input.position.xy].rgb;
    float3 specular = lightSpecularBuffer[input.position.xy].rgb;

    output.colour.rgb = diffuse + specular;

    return output;
}
