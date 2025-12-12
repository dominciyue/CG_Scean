#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse;
uniform bool hasTexture;
uniform vec3 objectColor;

// Lamp state
uniform bool lampOn;
uniform vec3 lampEmissionColor;
uniform float lampEmissionIntensity;

// Main light for lamp body
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main()
{
    vec4 baseColor;
    if (hasTexture) {
        baseColor = texture(texture_diffuse, TexCoords);
    } else {
        baseColor = vec4(objectColor, 1.0);
    }
    
    // Basic lighting for lamp body
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Ambient
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = 0.3 * spec * lightColor;
    
    vec3 lighting = ambient + diffuse + specular;
    vec3 result = lighting * baseColor.rgb;
    
    // Add emission when lamp is ON (lamp shade glows)
    if (lampOn) {
        // Check if this is the lamp shade (upper part of lamp)
        // Emission is stronger for the shade area
        float emissionFactor = lampEmissionIntensity;
        result += lampEmissionColor * emissionFactor * 0.5;
        
        // Make it look like it's glowing
        result = mix(result, lampEmissionColor, 0.3);
    }
    
    FragColor = vec4(result, baseColor.a);
}
