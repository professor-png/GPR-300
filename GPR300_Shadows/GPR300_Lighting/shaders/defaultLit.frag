#version 450                          
out vec4 FragColor;

uniform float _AmbientK;
uniform float _DiffuseK;
uniform float _SpecularK;
uniform float _Shininess;

//From application...
uniform vec3 _CameraPos;
uniform vec3 _Color; //material color
uniform float _NormalIntensity;

uniform sampler2D _Texture;
uniform sampler2D _NormalMap;
uniform float _Time;

uniform sampler2D _DiffuseTexture;
uniform sampler2D _ShadowMap;

uniform vec3 _LightPos;
uniform vec3 _ViewPos;

in struct Vertex
{
    vec3 Normal;
    vec3 WorldNormal; // fragment normal in world space
    vec3 WorldPosition; // fragment position in world space
    vec2 UV;
    mat3 TBN;
    vec4 FragPosLightSpace;
}vs_out;

struct DirectionalLight
{
    vec3 direction;
    vec3 color;
    float intensity;
};

uniform DirectionalLight _Light;

float ShadowCalculation(float dotLightNorm)
{
    vec3 pos = vs_out.FragPosLightSpace.xyz * 0.5 + 0.5;

    if (pos.z > 1)
    {
        pos.z = 1;
    }

    float bias = max(0.05 * (1.0 - dotLightNorm), 0.005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(_ShadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float depth = texture(_ShadowMap, pos.xy + vec2(x, y) * texelSize).r;
            shadow += (depth + bias) < pos.z ? 0.0 : 1.0;
        }
    }

    return shadow / 9.0;
}

vec3 CalculateAmbient(float lightIntensity, vec3 lightColor)
{
    return (_AmbientK * lightIntensity) * lightColor;
}

vec3 CalculateDiffuse(float lightIntensity, vec3 lightColor, vec3 lightDir, vec3 worldNormal)
{
    float diffuseDot = max(dot(lightDir, worldNormal), 0);

    return (_DiffuseK * diffuseDot * lightIntensity) * lightColor;
}

vec3 CalculateSpecular(float lightIntensity, vec3 lightColor, vec3 lightDir, vec3 worldNormal)
{
    vec3 halfway = normalize(normalize(_CameraPos - vs_out.WorldPosition) + normalize(lightDir));
    float specularDot = max(dot(worldNormal, halfway), 0);

    return (_SpecularK * pow(specularDot, _Shininess) * lightIntensity) * lightColor;
}
//
//vec3 CalculateDirectionalLight(DirectionalLight light, vec3 worldNormal)
//{
//    vec3 ambient = CalculateAmbient(light.intensity, light.color);
//    vec3 diffuse = CalculateDiffuse(light.intensity, light.color, light.direction, worldNormal);
//
//    vec3 specular = CalculateSpecular(light.intensity, light.color, light.direction, worldNormal);
//
//    return ambient + diffuse + specular;
//}
//
//void main()
//{
//    //convert to [-1,1] range
//    vec3 normal = (texture(_NormalMap, vs_out.UV).rgb * 2) - 1;
//    normal *= vec3(_NormalIntensity, _NormalIntensity, 1); // apply intensity while keeping z direction
//    //normal = vs_out.TBN * normal;
//
//    vec3 lightColor = CalculateDirectionalLight(_Light, normal);
//
//    vec4 color = texture(_Texture, vs_out.UV);
//
//    FragColor = vec4(_Color * lightColor, 1.0f) * color;
//}

void main()
{             
    vec3 color = texture(_DiffuseTexture, vs_out.UV).rgb;
    vec3 normal = normalize(vs_out.Normal);

    // ambient
    vec3 ambient = CalculateAmbient(_Light.intensity, _Light.color);

    // diffuse
    vec3 lightDir = normalize(_LightPos - vs_out.WorldPosition);
    vec3 diffuse = CalculateDiffuse(_Light.intensity, _Light.color, lightDir, normal);

    // specular
    vec3 specular = CalculateSpecular(_Light.intensity, _Light.color, lightDir, normal);

    // calculate shadow
    float shadow = ShadowCalculation(dot(lightDir, normal));
    vec3 lighting = (shadow * (diffuse + specular) + ambient) * color;
    
    FragColor = vec4(lighting, 1.0);
}  