#version 450                          
out vec4 FragColor;

uniform float _AmbientK;
uniform float _DiffuseK;
uniform float _SpecularK;
uniform float _Shininess;

//From application...
uniform vec3 _CameraPos;
uniform vec3 _Color; //material color

in struct Vertex
{
    vec3 WorldNormal; // fragment normal in world space
    vec3 WorldPosition; // fragment position in world space
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
in vec2 UV;
in vec3 Normal;
uniform float _Time;

void main()
{
    vec3 normal = normalize(vs_out.WorldNormal);

    vec3 lightColor = CalculatePointLight(_PointLight, vs_out.WorldNormal);

    vec4 color = texture(_Texture, UV);

    FragColor = vec4(_Color * lightColor, 1.0f) * color;
}