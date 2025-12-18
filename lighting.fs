#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoord;
  
// Main light (ceiling light)
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

// Lamp light (desk lamp)
uniform bool lampOn;
uniform vec3 lampLightPos;
uniform vec3 lampLightColor;
uniform float lampLightIntensity;
uniform float lampLightRadius;

// Spirit orb light
uniform bool orbEmitting;
uniform vec3 orbLightPos;
uniform vec3 orbLightColor;
uniform float orbLightIntensity;
uniform float orbLightRadius;

// Texture
uniform sampler2D texture1;
uniform bool hasTexture;
uniform float alpha;

// Calculate lamp light attenuation
float calcLampAttenuation(float distance) {
    float constant = 1.0;
    float linear = 2.0;
    float quadratic = 3.0;
    return 1.0 / (constant + linear * distance + quadratic * distance * distance);
}

// Calculate orb light attenuation (softer falloff)
float calcOrbAttenuation(float distance) {
    float constant = 1.0;
    float linear = 1.5;
    float quadratic = 2.0;
    return 1.0 / (constant + linear * distance + quadratic * distance * distance);
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // =========================================
    // Main Light (ceiling light)
    // =========================================
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
  	
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    float specularStrength = 0.5;
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 mainLightResult = ambient + diffuse + specular;
    
    // =========================================
    // Lamp Light (desk lamp - only if on)
    // =========================================
    vec3 lampResult = vec3(0.0);
    
    if (lampOn) {
        // Calculate lamp contribution
        vec3 lampDir = normalize(lampLightPos - FragPos);
        float lampDistance = length(lampLightPos - FragPos);
        float lampAttenuation = calcLampAttenuation(lampDistance / lampLightRadius);
        
        // Lamp ambient (warm glow)
        vec3 lampAmbient = 0.1 * lampLightColor * lampLightIntensity * lampAttenuation;
        
        // Lamp diffuse
        float lampDiff = max(dot(norm, lampDir), 0.0);
        vec3 lampDiffuse = lampDiff * lampLightColor * lampLightIntensity * lampAttenuation;
        
        // Lamp specular (softer)
        vec3 lampReflectDir = reflect(-lampDir, norm);
        float lampSpec = pow(max(dot(viewDir, lampReflectDir), 0.0), 16);
        vec3 lampSpecular = 0.3 * lampSpec * lampLightColor * lampLightIntensity * lampAttenuation;
        
        lampResult = lampAmbient + lampDiffuse + lampSpecular;
    }
    
    // =========================================
    // Spirit Orb Light (mystical glow - only if emitting)
    // =========================================
    vec3 orbResult = vec3(0.0);
    
    if (orbEmitting) {
        // Calculate orb contribution
        vec3 orbDir = normalize(orbLightPos - FragPos);
        float orbDistance = length(orbLightPos - FragPos);
        float orbAttenuation = calcOrbAttenuation(orbDistance / orbLightRadius);
        
        // Mystical ambient glow (stronger than lamp)
        vec3 orbAmbient = 0.15 * orbLightColor * orbLightIntensity * orbAttenuation;
        
        // Orb diffuse (soft, ethereal)
        float orbDiff = max(dot(norm, orbDir), 0.0);
        vec3 orbDiffuse = orbDiff * orbLightColor * orbLightIntensity * orbAttenuation;
        
        // Orb specular (shimmer effect)
        vec3 orbReflectDir = reflect(-orbDir, norm);
        float orbSpec = pow(max(dot(viewDir, orbReflectDir), 0.0), 8);
        vec3 orbSpecular = 0.5 * orbSpec * orbLightColor * orbLightIntensity * orbAttenuation;
        
        orbResult = orbAmbient + orbDiffuse + orbSpecular;
    }
    
    // =========================================
    // Combine lighting
    // =========================================
    vec3 totalLight = mainLightResult + lampResult + orbResult;
    
    vec4 textureColor = vec4(1.0);
    if (hasTexture) {
        textureColor = texture(texture1, TexCoord);
    }
    
    vec3 result = totalLight * objectColor * textureColor.rgb;
    FragColor = vec4(result, textureColor.a * alpha);
} 
