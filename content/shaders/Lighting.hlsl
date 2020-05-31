#define PI 3.14159265359f

struct DirectionalLightData
{
    float4 Direction;
    float4 Colour;
    float Intensity;
    float3 DirectionalLightData_pad;
};


cbuffer LightingConstantBuffer : register(b0)
{
	float4x4	InvViewProjection;
	float4		CameraPos;
	float4		RTSize;
};

cbuffer LightsConstantBuffer : register(b1)
{
	DirectionalLightData DirectionalLight;
};

struct VSInput
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float2 uv : TEXCOORD;
	float4 position : SV_POSITION;
};

struct PSOutput
{
	float4 diffuse : SV_Target0;
	float4 specular : SV_Target1;
};

PSInput VSMain(VSInput input)
{
	PSInput result;

	result.position = float4(input.position.xyz, 1);
	result.uv = input.uv;

	return result;
}

Texture2D<float4> albedoBuffer : register(t0);
Texture2D<float4> normalBuffer : register(t1);
Texture2D<float> depthBuffer : register(t2);

float3 Fresnel(float3 F0, float3 L, float3 H)
{
    float dotLH = saturate(dot(L, H));
    float dotLH5 = pow(1.0f - dotLH, 5);
    return F0 + (1.0 - F0) * (dotLH5);
}

float G1V(float dotNV, float k)
{
    return 1.0f / (dotNV * (1.0f - k) + k);
}

float3 SpecularBRDF(float3 N, float3 V, float3 L, float roughness, float3 F0)
{
    float alpha = roughness * roughness;

    float3 H = normalize(V + L);

    float dotNL = saturate(dot(N, L));
    float dotNV = saturate(dot(N, V));
    float dotNH = saturate(dot(N, H));
    float dotLH = saturate(dot(L, H));

	// D, based on GGX
    float alphaSqr = alpha * alpha;
    float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0f;
    float D = alphaSqr / (PI * denom * denom);

	// F
    float3 F = Fresnel(F0, L, H);

	// V
    float k = alpha / 2.0f;
    float Vis = G1V(dotNL, k) * G1V(dotNV, k);

    float3 specular = (D * Vis) * F;

    return specular;
}

PSOutput PSMain(PSInput input)
{
	PSOutput output = (PSOutput)0;

	float depth = depthBuffer[input.position.xy].x;

	if (depth >= 1.0f)
        output.diffuse.rgb = float3(1, 0, 1);
	
	float4 normal = normalBuffer[input.position.xy];
	float4 albedo = albedoBuffer[input.position.xy];

	float roughness = 1.0f;
    float metalness = 0.5f;
	float3 specularColour = lerp(0.04, albedo.rgb, metalness);

	float2 uv = input.position.xy * RTSize.zw;
	float4 clipPos = float4(2 * uv - 1, depth, 1);
	clipPos.y = -clipPos.y;

	float4 worldPos = mul(InvViewProjection, clipPos);
	worldPos.xyz /= worldPos.w;

	float3 viewDir = normalize(CameraPos.xyz - worldPos.xyz);

	float3 lightDir = DirectionalLight.Direction.xyz;
	float3 lightColour = DirectionalLight.Colour.xyz;
	float lightIntensity = DirectionalLight.Intensity.x;
	float NdotL = saturate(dot(normal.xyz, lightDir));

    output.diffuse.rgb = (lightIntensity * (1.0 / PI) * NdotL) * lightColour * albedo.rgb;
	output.specular.rgb = (lightIntensity * NdotL) *  SpecularBRDF(normal.xyz, viewDir, lightDir, roughness, specularColour) * lightColour;

    return output;
}
