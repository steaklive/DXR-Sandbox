struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

struct PSInput
{
    float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float4 position : SV_POSITION;
};

cbuffer GPrepassCB : register(b0)
{
	float4x4 ViewProjection;
	float4x4 InverseViewProjection;
	float4  CameraPosition;
	float4	RTSize;
	float	MipBias;
};

cbuffer perModelInstanceCB : register(b1)
{
	float4x4	World;
    float4		DiffuseColor;
};

Texture2D<float4> Textures[] : register(t0);
SamplerState SamplerLinear : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput result;

	result.normal = mul((float3x3)World, input.normal.xyz);
	result.tangent = mul((float3x3)World, input.tangent.xyz);

	result.position = mul(World, float4(input.position.xyz, 1));

	result.worldPos = result.position.xyz;

	result.position = mul(ViewProjection, result.position);

	result.uv = input.uv;

    return result;
}

struct PSOutput
{
	float4 colour : SV_Target0;
	float4 normal : SV_Target1;
};

PSOutput PSMain(PSInput input)
{
	PSOutput output = (PSOutput)0;

    float4 finalColor = DiffuseColor;
	
    if (DiffuseColor.w > 0.0) // just some basic art (checkerboard pattern for reflective surfaces - floor)
    {
        float checkerboard = floor(input.worldPos.x) + floor(input.worldPos.z);
        checkerboard = frac(checkerboard * 0.5) * 2;
        finalColor = lerp(float4(0, 0, 0, DiffuseColor.w), float4(0.2, 0.2, 0.2, DiffuseColor.w), checkerboard);
    }

    output.colour = finalColor;
    output.normal = float4(input.normal, 1);

    return output;
}
