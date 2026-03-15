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
    float Threshold;
};

[numthreads(16, 16, 1)]
void CShader(uint3 DTid : SV_DispatchThreadID)
{
    uint2 coord = DTid.xy;
    if (coord.x >= OutputSize.x || coord.y >= OutputSize.y)
        return;

    if (InputSize.x == OutputSize.x && InputSize.y == OutputSize.y)
    {
        float2 uv = float2(coord) / InputSize;

        float3 color = u_Input.Sample(u_Sampler, uv);
        float luminence = dot(color, float3(0.2126, 0.7152, 0.0722));

        if (luminence < Threshold)
            return;

        u_Output[coord] = float4(color, 1.0);
        return;
    }

    int2 srcCoord = int2(coord) * 2;

    float2 uv = (srcCoord + 1.5) / InputSize;
    float2 texelSize = 1.0 / InputSize;
    
    // . . . . . . .
    // . A . B . C .
    // . . D . E . .
    // . F . G . H .
    // . . I . J . .
    // . K . L . M .
    // . . . . . . .
    float4 A = u_Input.Sample(u_Sampler, uv + texelSize * float2(-1.0f, -1.0f));
    float4 B = u_Input.Sample(u_Sampler, uv + texelSize * float2( 0.0f, -1.0f));
    float4 C = u_Input.Sample(u_Sampler, uv + texelSize * float2( 1.0f, -1.0f));
    float4 D = u_Input.Sample(u_Sampler, uv + texelSize * float2(-0.5f, -0.5f));
    float4 E = u_Input.Sample(u_Sampler, uv + texelSize * float2( 0.5f, -0.5f));
    float4 F = u_Input.Sample(u_Sampler, uv + texelSize * float2(-1.0f,  0.0f));
    float4 G = u_Input.Sample(u_Sampler, uv + texelSize * float2( 0.0f,  0.0f));
    float4 H = u_Input.Sample(u_Sampler, uv + texelSize * float2( 1.0f,  0.0f));
    float4 I = u_Input.Sample(u_Sampler, uv + texelSize * float2(-0.5f,  0.5f));
    float4 J = u_Input.Sample(u_Sampler, uv + texelSize * float2( 0.5f,  0.5f));
    float4 K = u_Input.Sample(u_Sampler, uv + texelSize * float2(-1.0f,  1.0f));
    float4 L = u_Input.Sample(u_Sampler, uv + texelSize * float2( 0.0f,  1.0f));
    float4 M = u_Input.Sample(u_Sampler, uv + texelSize * float2( 1.0f,  1.0f));

    float2 div = 0.25 * float2(0.5, 0.125);

    float4 color = (D + E + I + J) * div.x;
    color += (A + B + G + F) * div.y;
    color += (B + C + H + G) * div.y;
    color += (F + G + L + K) * div.y;
    color += (G + H + M + L) * div.y;

    u_Output[coord] = color;
}
