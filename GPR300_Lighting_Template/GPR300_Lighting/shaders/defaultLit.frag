#version 450                          
out vec4 FragColor;

in vec3 Normal;
in vec3 WorldNormal;
in vec3 WorldPos;

uniform vec3 _LightDir;

uniform float _LightIntensity;
uniform float _AmbientK;
uniform float _DiffuseK;
uniform float _SpecularK;
uniform float _Shininess;

//From application...
uniform vec3 _CameraPos;
uniform vec3 _Color; //material color
uniform vec3 _LightColor;

void main(){
    vec3 ambient = (_AmbientK * _LightIntensity) * _LightColor;
    float diffuseDot = max(dot(_LightDir, Normal), 0);
    vec3 diffuse = (_DiffuseK * diffuseDot * _LightIntensity) * _LightColor;

    vec3 halfway = normalize(normalize(_CameraPos - WorldPos) + normalize(_LightDir));
    float specularDot = max(dot(Normal, halfway), 0);
    vec3 specular = (_SpecularK * pow(specularDot, _Shininess) * _LightIntensity) * _LightColor;

    vec3 lightColor = ambient + diffuse + specular;

    FragColor = vec4(_Color * lightColor,1.0f);
}

//(_SpecularK * pow(dot(reflect(normalize(vPos - _LightPos), vNormal), _CameraPos), _Shininess) * _LightIntensity) * _LightColor; //point light