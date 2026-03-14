#pragma pack_matrix(column_major)

#define FLT_MAX 3.40282347e+38
#define UINT_MAX 0xFFFFFFFF

struct Ray
{
    float3 Origin;
    float3 Direction;
};

struct HitPayload
{
    float HitDistance;
    float3 WorldPosition;
    float3 WorldNormal;

    int ObjectIndex;
};

struct Material
{
    float4 Albedo;
    float Roughness;
    float Metallic;
    float2 Padding;
    float3 EmissionColor;
    float EmissionPower;
    float4 Padding;
};

struct Sphere
{
    float4 Position;
    float Radius;
    int MaterialIndex;
    float2 Padding;
};

[[vk::binding(0, 0)]]
RWTexture2D<float4> u_AccumulatedImage;
[[vk::binding(1, 0)]]
RWTexture2D<float4> u_FinalImage;

[[vk::binding(2, 0)]]
cbuffer Camera
{
    struct
    {
        float4x4 InverseProjection;
        float4x4 InverseView;
        float4 Position;
        float4 ViewportSize;
    } u_Camera;
};

[[vk::binding(3, 0)]]
cbuffer Scene
{
    struct
    {
        int SphereCount;
        int MaterialCount;
        float2 Padding;
        Sphere Spheres[10];
        Material Materials[10];
    } u_Scene;
}

[[vk::push_constant]]
cbuffer PushConstants
{
    uint u_FrameIndex;
    bool u_Accumulate;
};

float4 perPixel(uint x, uint y);
HitPayload traceRay(Ray ray);
HitPayload closestHit(Ray ray, float hitDistance, int objectIndex);
HitPayload miss(Ray ray);
float3 calculateRayDirection(uint x, uint y);
float3 inUnitSphere(inout uint seed);
float3 getEmission(int materialIndex);

[[numthreads(16, 16, 1)]]
void CShader(uint3 DTid : SV_DispatchThreadID)
{
    uint2 coord = DTid.xy;

    if (coord.x > 600 || coord.y > 600)
        return;

    float4 color = perPixel(coord.x, coord.y);
    if (u_Accumulate)
    {
        u_AccumulatedImage[coord] += color;
        
        float4 accumulatedColor = u_AccumulatedImage[coord];
        accumulatedColor /= (float)u_FrameIndex;

        u_FinalImage[coord] = accumulatedColor;
    }
    else
    {
        u_FinalImage[coord] = color;
    }
}

float4 perPixel(uint x, uint y)
{
    Ray ray;
    ray.Origin = u_Camera.Position;
    ray.Direction = calculateRayDirection(x, y);

    uint seed = x + y * (uint)u_Camera.ViewportSize.x;
    seed *= u_FrameIndex;

    float3 light = float3(0.0, 0.0, 0.0);
    float3 contribution = float3(1.0, 1.0, 1.0);

    const uint bounces = 5;
    for (uint i = 0; i < bounces; i++)
    {
        seed += i;

        HitPayload payload = traceRay(ray);
        if (payload.HitDistance < 0.0)
        {
            float3 skyColor = float3(0.6, 0.7, 0.9);
            //light += skyColor * contribution;
            break;
        }

        float3 albedo = u_Scene.Materials[u_Scene.Spheres[payload.ObjectIndex].MaterialIndex].Albedo.xyz;

        contribution *= albedo;
        light += getEmission(u_Scene.Spheres[payload.ObjectIndex].MaterialIndex);

        ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001;
        ray.Direction = normalize(payload.WorldNormal + inUnitSphere(seed));
    }

    return float4(light, 1.0);
}

HitPayload traceRay(Ray ray)
{
    int closestSphere = -1;
    float hitDistance = FLT_MAX;

    for (int i = 0; i < u_Scene.SphereCount; i++)
    {
        float3 origin = ray.Origin - u_Scene.Spheres[i].Position;

        float a = dot(ray.Direction, ray.Direction);
        float b = 2.0 * dot(origin, ray.Direction);
        float c = dot(origin, origin) - (u_Scene.Spheres[i].Radius * u_Scene.Spheres[i].Radius);

        float discriminant = b * b - 4 * a * c;

        if (discriminant < 0.0)
            continue;

        float t0 = (-b + sqrt(discriminant)) / (2 * a);
        float closestT = (-b - sqrt(discriminant)) / (2 * a);

        if (closestT > 0.0 && closestT < hitDistance)
        {
            hitDistance = closestT;
            closestSphere = i;
        }
    }

    if (closestSphere < 0)
        return miss(ray);

    return closestHit(ray, hitDistance, closestSphere);
}

HitPayload closestHit(Ray ray, float hitDistance, int objectIndex)
{
    HitPayload payload;
    payload.HitDistance = hitDistance;
    payload.ObjectIndex = objectIndex;

    float3 origin = ray.Origin - u_Scene.Spheres[objectIndex].Position;

    payload.WorldPosition = origin + ray.Direction * hitDistance;
    payload.WorldNormal = normalize(payload.WorldPosition);
    payload.WorldPosition += u_Scene.Spheres[objectIndex].Position;

    return payload;
}

HitPayload miss(Ray ray)
{
    HitPayload payload;
    payload.HitDistance = -1.0;
    return payload;
}

float3 calculateRayDirection(uint x, uint y)
{
    float2 coord = float2(x, y) / u_Camera.ViewportSize;
    coord = coord * 2.0 - 1.0;

    float4 target = mul(u_Camera.InverseProjection, float4(coord, 1.0, 1.0));
    float3 rayDirection = normalize(target.xyz / target.w);

    rayDirection = mul((float3x3)u_Camera.InverseView, rayDirection);

    return normalize(rayDirection);
}

uint pcgHash(uint input)
{
    uint state = input * 747796405 + 2891336453;
    uint word = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
    return (word >> 22) ^ word;
}

float randomFloat(inout uint seed)
{
    seed = pcgHash(seed);
    return (float)seed / 4294967295.0;
}

float3 inUnitSphere(inout uint seed)
{
    return normalize(float3(randomFloat(seed) * 2.0 - 1.0, randomFloat(seed) * 2.0 - 1.0, randomFloat(seed) * 2.0 - 1.0));
}

float3 getEmission(int materialIndex)
{
    return u_Scene.Materials[materialIndex].EmissionColor * u_Scene.Materials[materialIndex].EmissionPower;
}
