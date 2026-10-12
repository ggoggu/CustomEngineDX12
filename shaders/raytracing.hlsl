RaytracingAccelerationStructure Scene : register(t0);
StructuredBuffer<uint> Indices : register(t2);

RWTexture2D<float4> RenderTarget : register(u0);
RWTexture2D<float4> AccumulationTarget : register(u1);

cbuffer CameraCB : register(b0)
{
    float4x4 viewInverse;
    float4x4 projInverse;
    uint frameCount;
};

struct Vertex {
    float3 position;
    float4 color; // From our C++ Vertex struct
};

StructuredBuffer<Vertex> VertexBuffer : register(t1);

struct RayPayload
{
    float3 color;
    float hitT;
    float3 normal;
    float3 albedo;
};

// PCG Random Number Generator
uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float RandomFloat(inout uint seed)
{
    seed = pcg_hash(seed);
    return (float)seed / 4294967296.0;
}

float3 RandomCosineDirection(float3 n, inout uint seed)
{
    float u1 = RandomFloat(seed);
    float u2 = RandomFloat(seed);
    float r = sqrt(u1);
    float theta = 2.0 * 3.14159265 * u2;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0.0, 1.0 - u1));

    float3 up = abs(n.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, n));
    float3 bitangent = cross(n, tangent);

    return tangent * x + bitangent * y + n * z;
}

[shader("raygeneration")]
void RayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;

    uint seed = launchIndex.x + launchIndex.y * launchDim.x + frameCount * 1973;

    float2 jitter = float2(RandomFloat(seed), RandomFloat(seed));
    float2 crd = float2(launchIndex.x + jitter.x, launchIndex.y + jitter.y);
    float2 dims = float2(launchDim.x, launchDim.y);

    float2 d = ((crd / dims) * 2.0f - 1.0f);
    d.y = -d.y;

    float4 target = mul(float4(d.x, d.y, 1, 1), projInverse);
    float3 rayDir = normalize(mul(float4(normalize(target.xyz / target.w), 0), viewInverse).xyz);
    float3 origin = mul(float4(0, 0, 0, 1), viewInverse).xyz;

    float3 throughput = float3(1, 1, 1);
    float3 radiance = float3(0, 0, 0);

    for (int bounce = 0; bounce < 4; bounce++) {
        RayDesc ray;
        ray.Origin = origin;
        ray.Direction = rayDir;
        ray.TMin = 0.001f;
        ray.TMax = 10000.0f;

        RayPayload payload;
        payload.hitT = -1.0f;
        
        TraceRay(Scene, RAY_FLAG_NONE, ~0, 0, 1, 0, ray, payload);
        
        if (payload.hitT < 0.0f) {
            radiance += throughput * payload.color;
            break;
        }

        float3 hitPos = origin + rayDir * payload.hitT;
        float3 normal = payload.normal;
        if (dot(normal, rayDir) > 0.0f) {
            normal = -normal;
        }
        
        float3 nextDir = RandomCosineDirection(normal, seed);
        
        origin = hitPos + normal * 0.001f;
        rayDir = nextDir;
        throughput *= payload.albedo;
    }

    float3 currentRadiance = radiance;
    
    if (frameCount > 1) {
        float3 prevRadiance = AccumulationTarget[launchIndex].xyz;
        currentRadiance = lerp(prevRadiance, currentRadiance, 1.0f / float(frameCount));
    }
    
    AccumulationTarget[launchIndex] = float4(currentRadiance, 1.0f);
    
    // Simple Reinhard Tonemapping
    float3 mapped = currentRadiance / (currentRadiance + float3(1.0, 1.0, 1.0));
    // Gamma correction
    mapped = pow(mapped, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));
    
    RenderTarget[launchIndex] = float4(mapped, 1.0f);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    float t = 0.5f * (WorldRayDirection().y + 1.0f);
    payload.color = lerp(float3(1.0f, 1.0f, 1.0f), float3(0.5f, 0.7f, 1.0f), t) * 1.5f; // Sky intensity
    payload.hitT = -1.0f;
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.hitT = RayTCurrent();
    
    uint primitiveIndex = PrimitiveIndex();
    uint i0 = Indices[primitiveIndex * 3 + 0];
    uint i1 = Indices[primitiveIndex * 3 + 1];
    uint i2 = Indices[primitiveIndex * 3 + 2];
    
    float3 v0 = VertexBuffer[i0].position;
    float3 v1 = VertexBuffer[i1].position;
    float3 v2 = VertexBuffer[i2].position;
    
    payload.normal = normalize(cross(v1 - v0, v2 - v0));
    
    // Instead of taking the vertex color, let's use a nice diffuse color
    payload.albedo = float3(0.8f, 0.3f, 0.3f); // Red
}
