#include "Common.hlsl"

[shader("closesthit")]
void ClosestHitObject(inout HitInfo payload, Attributes attrib) 
{
    payload.colorAndDistance = float4(float3(0.2, 0.2, 0.2), RayTCurrent());
}

[shader("closesthit")]
void ClosestHitPlane(inout HitInfo payload, Attributes attrib)
{
    payload.colorAndDistance = float4(float3(0.1, 0.4, 0.6), RayTCurrent());
}