#version 400 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main()
{
    FragPos = vec3(modelMat * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(modelMat))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projMat * viewMat * modelMat * vec4(aPos, 1.0);
    
}