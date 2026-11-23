//what the vertex shader receives (one vertex, read using your input layout)
struct VSInput
{
    float3 position : POSITION;
    float4 colour : COLOUR;
};

//what the vertex shader hands to the rasterizer
struct PSInput
{
    float4 position : SV_POSITION;
    float4 colour : COLOUR;
};

//runs once per vertex (3 times)
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.colour = input.colour;
    return output;
}

//runs once per pixel the triangle covers
float4 PSMain(PSInput input) : SV_TARGET
{
    return input.colour;
}