#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 LocalPos;

uniform vec3 viewPos;
uniform vec3 coreColor;
uniform vec3 glowColor;
uniform float glowIntensity;
uniform float time;

void main()
{
    // Fresnel effect for edge glow
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 normal = normalize(Normal);
    float fresnel = 1.0 - max(dot(viewDir, normal), 0.0);
    fresnel = pow(fresnel, 2.0);
    
    // Inner glow pattern based on local position
    float innerPattern = sin(LocalPos.y * 20.0 + time * 3.0) * 0.5 + 0.5;
    innerPattern *= sin(LocalPos.x * 15.0 + time * 2.0) * 0.5 + 0.5;
    
    // Pulsing effect
    float pulse = sin(time * 4.0) * 0.15 + 0.85;
    
    // Core color with pattern
    vec3 core = coreColor * (0.8 + 0.2 * innerPattern);
    
    // Edge glow
    vec3 edgeGlow = glowColor * fresnel * 2.0;
    
    // Combine colors
    vec3 finalColor = core + edgeGlow;
    finalColor *= glowIntensity * pulse;
    
    // Add bloom-like effect by making bright areas brighter
    float luminance = dot(finalColor, vec3(0.299, 0.587, 0.114));
    finalColor += finalColor * luminance * 0.5;
    
    // Output with slight transparency for glow blending
    float alpha = 0.9 + fresnel * 0.1;
    
    FragColor = vec4(finalColor, alpha);
}






