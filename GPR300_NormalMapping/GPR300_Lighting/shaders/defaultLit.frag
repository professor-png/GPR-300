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

in struct Vertex
{
    vec3 Normal;
    vec3 WorldNormal; // fragment normal in world space
    vec3 WorldPosition; // fragment position in world space
    vec2 UV;
    mat3 TBN;
}vs_out;

struct PointLight
{
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

uniform PointLight _PointLight;

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

vec3 CalculatePointLight(PointLight light, vec3 worldNormal)
{
    vec3 ambient = vec3(0);
    vec3 diffuse = vec3(0);
    vec3 specular = vec3(0);
    vec3 total = vec3(0);
    float attenuation;

    ambient = CalculateAmbient(light.intensity, light.color);
    diffuse = CalculateDiffuse(light.intensity, light.color, normalize(light.position - vs_out.WorldPosition), worldNormal);
    specular = CalculateSpecular(light.intensity, light.color, normalize(light.position - vs_out.WorldPosition), worldNormal);

    attenuation = clamp(1 - (pow(distance(vs_out.WorldPosition, light.position) / light.range, 4)), 0, 1);

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    total += ambient + diffuse + specular;

    return total;
}

uniform sampler2D _Texture;
uniform sampler2D _NormalMap;
uniform float _Time;

void main()
{
    //convert to [-1,1] range
    vec3 normal = (texture(_NormalMap, vs_out.UV).rgb * 2) - 1;
    normal *= vec3(_NormalIntensity, _NormalIntensity, 1); // apply intensity while keeping z direction
    //normal = vs_out.TBN * normal;

    vec3 lightColor = CalculatePointLight(_PointLight, normal);

    vec4 color = texture(_Texture, vs_out.UV);

    FragColor = vec4(_Color * lightColor, 1.0f) * color;
}