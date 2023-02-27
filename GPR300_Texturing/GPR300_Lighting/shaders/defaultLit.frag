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

struct PointLight
{
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float radius;
    float innerAngle;
    float outerAngle;
};

uniform DirectionalLight _DirLight;
uniform SpotLight _SpotLight;
uniform PointLight _PointLights[3];
uniform int numPointLights;

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

vec3 CalculatePointLights(PointLight[3] lights, vec3 worldNormal)
{
    vec3 ambient = vec3(0);
    vec3 diffuse = vec3(0);
    vec3 specular = vec3(0);
    vec3 total = vec3(0);
    float attenuation;

    for (int i = 0; i < numPointLights; i++)
    {
        ambient += CalculateAmbient(lights[i].intensity, lights[i].color);
        diffuse += CalculateDiffuse(lights[i].intensity, lights[i].color, normalize(lights[i].position - vs_out.WorldPosition), worldNormal);
        specular += CalculateSpecular(lights[i].intensity, lights[i].color, normalize(lights[i].position - vs_out.WorldPosition), worldNormal);

        attenuation = clamp(1 - (pow(distance(vs_out.WorldPosition, lights[i].position) / lights[i].range, 4)), 0, 1);

        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;

        total += ambient + diffuse + specular;
    }

    return total;
}

vec3 CalculateSpotLight(SpotLight light, vec3 worldNormal)
{
    float theta = dot(normalize(vs_out.WorldPosition - light.position), normalize(-light.direction));

    if (theta < light.outerAngle)
        return vec3(0);

    float newIntensity = light.intensity * ((theta - light.outerAngle) / (light.innerAngle - light.outerAngle));

    vec3 ambient = CalculateAmbient(newIntensity, light.color);
    vec3 diffuse = CalculateDiffuse(newIntensity, light.color, normalize(light.direction), worldNormal);

    vec3 specular = CalculateSpecular(newIntensity, light.color, normalize(light.direction), worldNormal);

    return ambient + diffuse + specular;
}

void main()
{
    vec3 normal = normalize(vs_out.WorldNormal);

    vec3 lightColor = CalculateDirectionalLight(_DirLight, normal) + CalculatePointLights(_PointLights, normal) + CalculateSpotLight(_SpotLight, normal);

    FragColor = vec4(_Color * lightColor,1.0f);
}