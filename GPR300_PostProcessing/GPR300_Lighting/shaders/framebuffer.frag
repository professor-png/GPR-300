#version 450

out vec4 FragColor;
in vec2 texCoords;

uniform int _ApplyEffect;
uniform int _CurrentEffect;
uniform float _ScreenWidth;
uniform float _ScreenHeight;
uniform sampler2D _ScreenTexture;

float xOffset = 1f / _ScreenWidth;
float yOffset = 1f / _ScreenHeight;

vec2 neighborPixels[9] = vec2[] (vec2(-xOffset, yOffset), vec2(0, yOffset), vec2(xOffset, yOffset),
                                 vec2(-xOffset, 0), vec2(0, 0), vec2(xOffset, 0),
                                 vec2(-xOffset, -yOffset), vec2(0, -yOffset), vec2(xOffset, -yOffset));

float edgeDetection[9] = float[] (1, 1, 1,
                             1, -9, 1,
                             1, 1, 1);

float random[9] = float[] (1, 2, 1,
                             2, -10, 2,
                             1, 2, 1);

void main()
{
    if (_ApplyEffect == 0)
        FragColor = texture(_ScreenTexture, texCoords);
    else
    {
        vec4 color = texture(_ScreenTexture, texCoords);

        //greyscale
        if (_CurrentEffect == 0)
        {
            float average = (color.x + color.y + color.z) / 3;
            color.x = average;
            color.y = average;
            color.z = average;

            FragColor = color;
        }

        //edge detection
        if (_CurrentEffect == 1)
        {
            vec3 newCol = vec3(0);
            for (int i = 0; i < 9; i++)
                newCol += vec3(texture(_ScreenTexture, texCoords.st + neighborPixels[i])) * edgeDetection[i];
            FragColor = vec4(newCol, 1);
        }

        //inverse
        if (_CurrentEffect == 2)
        {
            FragColor = vec4(1 - color.x, 1 - color.y, 1 - color.z, 1);
        }

        //vignette
        if (_CurrentEffect == 3)
        {
            vec3 newCol = vec3(0);
            for (int i = 0; i < 9; i++)
                newCol += vec3(texture(_ScreenTexture, texCoords.st + neighborPixels[i])) * random[i];
            FragColor = vec4(newCol, 1);
        }
    }
}