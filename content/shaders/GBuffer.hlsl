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

	float3 n = 0;

	//if (AlbedoID >= 0)
	//{
	//	albedo = Textures[AlbedoID].SampleBias(SamplerLinear, finalTexOffset, MipBias).rgba;
	//}
	//clip(albedo.a - 0.5f);

	//if (NormalID >= 0)
	//{
	//	n = Textures[NormalID].SampleBias(SamplerLinear, input.uv, MipBias).rgb * 2 - 1;
	//}


    output.colour = DiffuseColor;
    output.normal = float4(input.normal, 1);
    //normalize(input.normal.xyz + float3(n.xy, 0));

    return output;
}
