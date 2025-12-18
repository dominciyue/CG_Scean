#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 particlePos;
uniform float particleSize;

void main()
{
    // Billboard effect: always face camera
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);
    
    vec3 vertexPosition = particlePos 
        + cameraRight * aPos.x * particleSize 
        + cameraUp * aPos.y * particleSize;
    
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(vertexPosition, 1.0);
}






