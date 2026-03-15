#pragma pack_matrix(column_major)

[[vk::binding(0, 0)]]
SamplerState u_Sampler;

[[vk::binding(0, 1)]]
Texture2D<float4> u_Input;

[[vk::binding(1, 1)]]
RWTexture2D<float4> u_Output;

[[vk::push_constant]]
cbuffer Constants
{
    float2 InputSize;
    float2 OutputSize;
    float BloomStrength;
};

[numthreads(16, 16, 1)]
void CShader(uint3 DTid : SV_DispatchThreadID)
{
    uint2 coord = DTid.xy;
    if (coord.x >= OutputSize.x || coord.y >= OutputSize.y)
        return;

    float2 inputTexelSize = 1.0 / InputSize;
    float2 uv = (coord + 0.5) * inputTexelSize * 2.0;

    float3 color = float3(0.0, 0.0, 0.0);

    color += u_Input.SampleLevel(u_Sampler, uv + inputTexelSize * float2(-1, -1), 0).rgb;
    color += u_Input.SampleLevel(u_Sampler, uv + inputTexelSize * float2( 1, -1), 0).rgb;
    color += u_Input.SampleLevel(u_Sampler, uv + inputTexelSize * float2(-1,  1), 0).rgb;
    color += u_Input.SampleLevel(u_Sampler, uv + inputTexelSize * float2( 1,  1), 0).rgb;

    color *= 0.25;
    float3 base = u_Output[coord].rgb;

    u_Output[coord] = float4(base + color * BloomStrength, 1.0);
}
