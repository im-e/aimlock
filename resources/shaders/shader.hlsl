//basic lighting: one directional light (like the sun) plus a little ambient light

cbuffer PerObject : register(b0) //root index 0, vertex shader
{
    row_major float4x4 WVP;   //model -> screen
    row_major float4x4 World; //model -> world, used to turn the normals the same way as the object
    float4 objectColour;      //base colour of this object
};

cbuffer PerFrame : register(b1) //root index 1, pixel shader
{
    float3 lightDirection; //world space direction the light travels in (pointing away from the sun), normalised
    float ambient;         //0..1, how bright the side facing away from the light is
    float3 lightColour;
    float specularStrength; //0->1 how bright the highlight is
    float3 cameraPosition; //world space
    float shininess; //higher = smaller, tighter highlight
};

//what the vertex shader receives - names must match Mesh::inputLayout()
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

//what the vertex shader hands to the rasterizer, interpolated across each triangle
struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL;
    float4 colour : COLOUR;
};

//runs once per vertex
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(WVP, float4(input.position, 1.0f));
    output.worldPos = mul(World, float4(input.position, 1.0f)).xyz; //w = 1, so this one does move with the object
    
    //rotate the normal into world space with the top-left 3x3 of World (no translation for directions)
    //note: this is only correct for uniform scaling (same scale on x, y and z)
    //non-uniform scaling needs the inverse transpose of World instead
    output.normal = mul((float3x3)World, input.normal);

    output.colour = objectColour;
    return output;
}

//runs once per pixel the triangle covers
float4 PSMain(PSInput input) : SV_TARGET
{
    //phong shading diffuse + ambient + specular
    //interpolating between vertices shortens the normal, so make it length 1 again
    float3 N = normalize(input.normal);
    float3 L = -lightDirection; //surface -> light
    float3 V = normalize(cameraPosition - input.worldPos); //surface -> camera
    
    float diffuse = saturate(dot(N, L));

    //phong: bounce the light off the surface like a mirror, and see how close the bounce comes to your eye
    float3 R = reflect(lightDirection, N);
    float3 H = normalize(L + V); //blinn phong halfway vector between light and view
    float specular = pow(saturate(dot(N, H)), shininess); 
    
    //no highlight on surfaces facing away from the light
    if (diffuse <= 0.0f)
        specular = 0.0f;

    float3 colour = input.colour.rgb * (ambient + diffuse * lightColour) //surface colour, lit
                  + specularStrength * specular * lightColour; //highlight, added on top
    return float4(colour, input.colour.a);
}
