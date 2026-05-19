#include "core.h"

[SpecializationConstant]
const bool swizzle_rrrr;

[vk::binding(0, 0)]
StructuredBuffer<Textured_Quad_f32> quads : register(t0);

[vk::binding(1, 0)]
Texture2D<float4> source_tex : register(t1);

[vk::binding(2, 0)]
SamplerState source_sampler: register(s2);

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 tint;
};

[shader("vertex")]
VSOutput vs_main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
	Textured_Quad_f32 quad = quads[instanceID];
    VSOutput o;
    o.position = quad.position[vertexID];
    o.uv = quad.uv[vertexID];
    o.tint = quad.tint[vertexID];
    return o;
}

[shader("fragment")]
float4 fs_main(VSOutput input) : SV_Target0
{
	float4 source_value = source_tex.Sample(source_sampler, input.uv);
	if (swizzle_rrrr) {
		source_value = source_value.rrrr;
	}
    return source_value * input.tint;
}
