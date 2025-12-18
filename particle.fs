#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 particleColor;
uniform float particleAlpha;

void main()
{
    // Soft circular particle
    vec2 center = TexCoords - vec2(0.5);
    float dist = length(center) * 2.0;
    
    // Soft falloff
    float alpha = 1.0 - smoothstep(0.0, 1.0, dist);
    alpha = pow(alpha, 1.5);  // Make edges softer
    
    // Add glow
    float glow = exp(-dist * 3.0);
    
    vec3 color = particleColor * (1.0 + glow * 0.5);
    
    FragColor = vec4(color, alpha * particleAlpha);
}






