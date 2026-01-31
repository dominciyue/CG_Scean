#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoord;
in float Height;

// Main light (ceiling)
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform float alpha;

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

// Calculate lamp light attenuation
float calcLampAttenuation(float distance) {
    float constant = 1.0;
    float linear = 2.0;
    float quadratic = 3.0;
    return 1.0 / (constant + linear * distance + quadratic * distance * distance);
}

// Calculate orb light attenuation
float calcOrbAttenuation(float distance) {
    float constant = 1.0;
    float linear = 1.5;
    float quadratic = 2.0;
    return 1.0 / (constant + linear * distance + quadratic * distance * distance);
}

void main()
{
    // Height-based color gradient
    vec3 lowColor = vec3(0.25, 0.55, 0.25);
    vec3 midColor = vec3(0.45, 0.60, 0.30);
    vec3 highColor = vec3(0.65, 0.65, 0.60);
    
    float heightFactor = clamp(Height / 0.05, 0.0, 1.0);
    vec3 baseColor = mix(lowColor, midColor, smoothstep(0.0, 0.5, heightFactor));
    baseColor = mix(baseColor, highColor, smoothstep(0.5, 1.0, heightFactor));
    
    // Slope-based rock color
    vec3 norm = normalize(Normal);
    float slope = 1.0 - abs(dot(norm, vec3(0.0, 1.0, 0.0)));
    vec3 rockColor = vec3(0.50, 0.48, 0.45);
    baseColor = mix(baseColor, rockColor, smoothstep(0.4, 0.8, slope) * 0.3);
    
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // =========================================
    // Main Light (ceiling)
    // =========================================
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    float specularStrength = 0.2;
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 mainLightResult = ambient + diffuse + specular;
    
    // =========================================
    // Lamp Light (desk lamp)
    // =========================================
    vec3 lampResult = vec3(0.0);
    
    if (lampOn) {
        // Calculate lamp contribution
        vec3 lampDir = normalize(lampLightPos - FragPos);
        float lampDistance = length(lampLightPos - FragPos);
        float lampAttenuation = calcLampAttenuation(lampDistance / lampLightRadius);
        
        // Lamp ambient (warm glow)
        vec3 lampAmbient = 0.15 * lampLightColor * lampLightIntensity * lampAttenuation;
        
        // Lamp diffuse (soft, warm)
        float lampDiff = max(dot(norm, lampDir), 0.0);
        vec3 lampDiffuse = lampDiff * lampLightColor * lampLightIntensity * lampAttenuation;
        
        // Lamp specular (softer)
        vec3 lampReflectDir = reflect(-lampDir, norm);
        float lampSpec = pow(max(dot(viewDir, lampReflectDir), 0.0), 16);
        vec3 lampSpecular = 0.2 * lampSpec * lampLightColor * lampLightIntensity * lampAttenuation;
        
        lampResult = lampAmbient + lampDiffuse + lampSpecular;
    }
    
    // =========================================
    // Spirit Orb Light
    // =========================================
    vec3 orbResult = vec3(0.0);
    
    if (orbEmitting) {
        vec3 orbDir = normalize(orbLightPos - FragPos);
        float orbDistance = length(orbLightPos - FragPos);
        float orbAttenuation = calcOrbAttenuation(orbDistance / orbLightRadius);
        
        vec3 orbAmbient = 0.15 * orbLightColor * orbLightIntensity * orbAttenuation;
        
        float orbDiff = max(dot(norm, orbDir), 0.0);
        vec3 orbDiffuse = orbDiff * orbLightColor * orbLightIntensity * orbAttenuation;
        
        vec3 orbReflectDir = reflect(-orbDir, norm);
        float orbSpec = pow(max(dot(viewDir, orbReflectDir), 0.0), 8);
        vec3 orbSpecular = 0.3 * orbSpec * orbLightColor * orbLightIntensity * orbAttenuation;
        
        orbResult = orbAmbient + orbDiffuse + orbSpecular;
    }
    
    // =========================================
    // Combine
    // =========================================
    float ao = 0.85 + 0.15 * heightFactor;
    vec3 totalLight = mainLightResult + lampResult + orbResult;
    vec3 result = totalLight * baseColor * ao;
    
    FragColor = vec4(result, alpha);
}
