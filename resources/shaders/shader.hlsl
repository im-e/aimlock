//declares a block of values that C++ sends to the shader
cbuffer PerObject : register(b0) //register b0 - b0 is the slot it arrives in defined by the shader register
{
    row_major float4x4 WVP; //matrix called WVP
};

//what the vertex shader receives
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
    output.position = mul(WVP, float4(input.position, 1.0f));
    output.colour = input.colour;
    return output;
}

//runs once per pixel the triangle covers
float4 PSMain(PSInput input) : SV_TARGET
{
    return input.colour;
}