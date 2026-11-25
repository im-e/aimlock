//the lecture's "spinning lights" pixel shader experiment

cbuffer PerObject : register(b0) //root index 0, vertex shader
{
    row_major float4x4 WVP;
};

cbuffer PerFrame : register(b1) //root index 1, pixel shader
{
    float time;
    float2 lights[4]; //screen space positions in pixels - every array element takes a full 16 bytes
};

struct VSInput
{
    float3 position : POSITION;
    float4 colour : COLOUR;
};

struct PSInput
{
    float4 position : SV_POSITION; //in the pixel shader this is the pixel position on screen
    float4 colour : COLOUR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(WVP, float4(input.position, 1.0f));
    output.colour = input.colour;
    return output;
}

//distance to a light, scaled by a radius that grows and shrinks with time
float dist(float2 screenspacePos, float2 light)
{
    return length(screenspacePos - light) / (50.0 * abs(cos(time)));
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 accumulated = float3(0, 0, 0);
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        accumulated += 1.0 / dist(input.position.xy, lights[i]);
    }
    accumulated *= input.colour.rgb;
    return float4(accumulated, 1.0);
}
