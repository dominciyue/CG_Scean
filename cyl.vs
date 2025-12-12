#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Cylindrical projection parameters for lamp shade
uniform float cylYMin;
uniform float cylYMax;
uniform bool useCylindricalMapping;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Check if we have valid texture coordinates from OBJ
    // If aTexCoords is (0,0) and useCylindricalMapping is true, use cylindrical projection
    if (useCylindricalMapping && aTexCoords.x == 0.0 && aTexCoords.y == 0.0) {
        // Cylindrical projection based on local position
        float u = atan(aPos.z, aPos.x) / (2.0 * 3.14159265);
        if (u < 0.0) u += 1.0;
        
        // V coordinate based on height
        float v = (aPos.y - cylYMin) / (cylYMax - cylYMin);
        v = clamp(v, 0.0, 1.0);
        
        TexCoords = vec2(u, v);
    } else {
        // Use texture coordinates from OBJ file
        TexCoords = aTexCoords;
    }
    
    gl_Position = projection * view * worldPos;
}
