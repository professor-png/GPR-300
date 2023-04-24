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

uniform sampler2D _Hatch1;
uniform sampler2D _Hatch2;
uniform sampler2D _Hatch3;
uniform sampler2D _Hatch4;

uniform DirectionalLight _DirLight;
in vec2 UV;


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

//here's Nate's fancy stuff for hatch lerping.
//this is kind of gross. 
struct GradientSegment
{
    float percent;
    vec3 color;
};

vec3 GetHatchGradient(float ratio, vec2 screenUV) 
{    
    if (ratio > 1) 
    {
        ratio = 1;
    }
    
    GradientSegment colors[4];
    colors[0].color = texture(_Hatch1, screenUV).xyz;
    colors[0].percent = 0.3;

    colors[1].color = texture(_Hatch1, screenUV).xyz;
    colors[1].percent = 0.55;

    colors[2].color = texture(_Hatch2, screenUV).xyz;
    colors[2].percent = 0.7;

    colors[3].color = texture(_Hatch4, screenUV).xyz;
    colors[3].percent = 1;
    
    vec3 finalColor = vec3(0, 0, 0);
    //lerp.

    for (int i = 0; i < 3; i++)
    {
        if (colors[i].percent <= ratio && colors[i + 1].percent >= ratio)
        {
            float lerpTime = (ratio - colors[i].percent) / (colors[i + 1].percent - colors[i].percent);
            finalColor.r = colors[i].color.x + lerpTime * (colors[i + 1].color.x - colors[i].color.x);
            finalColor.g = colors[i].color.y + lerpTime * (colors[i + 1].color.y - colors[i].color.y);
            finalColor.b = colors[i].color.z + lerpTime * (colors[i + 1].color.z - colors[i].color.z);
            return finalColor;
        }
    }
    return finalColor;
}

void main()
{
    vec3 normal = normalize(vs_out.WorldNormal);
    vec2 screenUV = gl_FragCoord.xy;
    vec2 tiledScreenUV = screenUV / (2 * -1.0f);
    vec3 lightColor = CalculateDirectionalLight(_DirLight, normal);
    float hatchPow = length(lightColor);
    vec3 hatchColor = GetHatchGradient(hatchPow, tiledScreenUV);

    FragColor = vec4(hatchColor, 1.0f);
}