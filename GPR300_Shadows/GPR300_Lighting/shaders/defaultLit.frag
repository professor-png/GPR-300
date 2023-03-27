//#version 450                          
//out vec4 FragColor;
//
//uniform float _AmbientK;
//uniform float _DiffuseK;
//uniform float _SpecularK;
//uniform float _Shininess;
//
////From application...
//uniform vec3 _CameraPos;
//uniform vec3 _Color; //material color
//uniform float _NormalIntensity;
//
//in struct Vertex
//{
//    vec3 Normal;
//    vec3 WorldNormal; // fragment normal in world space
//    vec3 WorldPosition; // fragment position in world space
//    vec2 UV;
//    mat3 TBN;
//}vs_out;
//
//struct DirectionalLight
//{
//    vec3 direction;
//    vec3 color;
//    float intensity;
//};
//
//uniform DirectionalLight _Light;
//
//vec3 CalculateAmbient(float lightIntensity, vec3 lightColor)
//{
//    return (_AmbientK * lightIntensity) * lightColor;
//}
//
//vec3 CalculateDiffuse(float lightIntensity, vec3 lightColor, vec3 lightDir, vec3 worldNormal)
//{
//    float diffuseDot = max(dot(lightDir, worldNormal), 0);
//
//    return (_DiffuseK * diffuseDot * lightIntensity) * lightColor;
//}
//
//vec3 CalculateSpecular(float lightIntensity, vec3 lightColor, vec3 lightDir, vec3 worldNormal)
//{
//    vec3 halfway = normalize(normalize(_CameraPos - vs_out.WorldPosition) + normalize(lightDir));
//    float specularDot = max(dot(worldNormal, halfway), 0);
//
//    return (_SpecularK * pow(specularDot, _Shininess) * lightIntensity) * lightColor;
//}
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
//uniform sampler2D _Texture;
//uniform sampler2D _NormalMap;
//uniform float _Time;
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
#version 450
out vec4 FragColor;

in struct Vertex
{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} vs_out;
  
in vec2 TexCoords;

uniform sampler2D _DiffuseTexture;
uniform sampler2D _ShadowMap;

uniform vec3 _LightPos;
uniform vec3 _ViewPos;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 pos = vs_out.FragPosLightSpace.xyz * 0.5 + 0.5;
    float depth = texture(_ShadowMap, pos.xy).r;

    return depth < pos.z ? 0.0 : 1.0;
}

void main()
{             
    vec3 color = texture(_DiffuseTexture, vs_out.TexCoords).rgb;
    vec3 normal = normalize(vs_out.Normal);
    vec3 lightColor = vec3(0.3);
    // ambient
    vec3 ambient = lightColor;
    // diffuse
    vec3 lightDir = normalize(_LightPos - vs_out.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(_ViewPos - vs_out.FragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    
    // calculate shadow
    float shadow = ShadowCalculation(vs_out.FragPosLightSpace);       
    vec3 lighting = (shadow * (diffuse + specular) + ambient) * color;
    
    FragColor = vec4(lighting, 1.0);
}  