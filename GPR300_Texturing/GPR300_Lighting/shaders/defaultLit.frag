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

struct DirectionalLight
{
    vec3 direction;
    vec3 color;
    float intensity;
};

uniform DirectionalLight _DirLight;

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

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 worldNormal)
{
    vec3 ambient = CalculateAmbient(light.intensity, light.color);
    vec3 diffuse = CalculateDiffuse(light.intensity, light.color, light.direction, worldNormal);

    vec3 specular = CalculateSpecular(light.intensity, light.color, light.direction, worldNormal);

    return ambient + diffuse + specular;
}

uniform sampler2D _GrassSide;
uniform sampler2D _GrassTop;
in vec2 UV;
in vec3 Normal;
uniform float _Time;

void main()
{
    vec3 normal = normalize(vs_out.WorldNormal);

    vec3 lightColor = CalculateDirectionalLight(_DirLight, normal);

    vec4 color;
    vec2 newUV = vec2(UV.x * abs(cos(_Time)), UV.y * abs(sin(_Time)));

    if (Normal.y > 0)
    {
        color = texture(_GrassTop, newUV);
    }
    else
    {
        color = texture(_GrassSide, newUV);
    }

    FragColor = vec4(_Color * lightColor, 1.0f) * color;
}