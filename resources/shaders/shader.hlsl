//default lit shader: one directional light (the sun), a bit of ambient, and a blinn-phong highlight

cbuffer PerObject : register(b0) //root index 0, vertex shader
{
    row_major float4x4 WVP;   //model -> clip space
    row_major float4x4 World; //model -> world, also used to turn the normals with the object
    float4 objectColour;
};

cbuffer PerFrame : register(b1) //root index 1, pixel shader
{
    float3 lightDirection;  //world space direction the light travels in (away from the sun), normalised
    float ambient;          //0-1, how lit the side facing away from the light is
    float3 lightColour;
    float specularStrength; //0-1, how bright the highlight is
    float3 cameraPosition;  //world space
    float shininess;        //higher = smaller, tighter highlight
};

//has to match Mesh::inputLayout()
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

//interpolated across each triangle on the way to the pixel shader
struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL;
    float4 colour : COLOUR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(WVP, float4(input.position, 1.0f));
    output.worldPos = mul(World, float4(input.position, 1.0f)).xyz;

    //only the top left 3x3 of World, since a direction shouldn't be moved by the translation
    //this is only right for uniform scale, non-uniform scale needs the inverse transpose instead
    output.normal = mul((float3x3)World, input.normal);

    output.colour = objectColour;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    //interpolation shortens the normal, so it needs normalising again
    float3 N = normalize(input.normal);
    float3 L = -lightDirection;                            //surface -> light
    float3 V = normalize(cameraPosition - input.worldPos); //surface -> camera

    float diffuse = saturate(dot(N, L));

    //blinn-phong uses the halfway vector between the light and the eye instead of reflecting the light
    float3 H = normalize(L + V);
    float specular = pow(saturate(dot(N, H)), shininess);
    if (diffuse <= 0.0f)
        specular = 0.0f; //no highlight on the side facing away from the light

    float3 colour = input.colour.rgb * (ambient + diffuse * lightColour)
                  + specularStrength * specular * lightColour;
    return float4(colour, input.colour.a);
}
