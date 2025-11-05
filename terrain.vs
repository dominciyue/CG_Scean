#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out float Height;  // 传递高度信息到片段着色器

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    TexCoord = aTexCoord;
    Height = aPos.y;  // 记录顶点的原始高度（模型空间）
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}

