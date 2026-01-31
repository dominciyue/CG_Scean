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
    // ========== 1. Fresnel效果（边缘发光）==========
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 normal = normalize(Normal);
    
    // Fresnel: 视线与法线夹角越大，fresnel值越高
    float fresnel = 1.0 - max(dot(viewDir, normal), 0.0);
    fresnel = pow(fresnel, 2.0);  // 二次方增强边缘效果
    
    // ========== 2. 内部发光图案 ==========
    // 基于局部坐标的动态波纹
    float innerPattern = sin(LocalPos.y * 20.0 + time * 3.0) * 0.5 + 0.5;
    innerPattern *= sin(LocalPos.x * 15.0 + time * 2.0) * 0.5 + 0.5;
    
    // ========== 3. 脉冲效果 ==========
    float pulse = sin(time * 4.0) * 0.15 + 0.85;  // 范围 0.7~1.0
    
    // ========== 4. 核心颜色 ==========
    vec3 core = coreColor * (0.8 + 0.2 * innerPattern);
    
    // ========== 5. 边缘光晕 ==========
    vec3 edgeGlow = glowColor * fresnel * 2.0;
    
    // ========== 6. 合并颜色 ==========
    vec3 finalColor = core + edgeGlow;
    finalColor *= glowIntensity * pulse;
    
    // ========== 7. 泛光增强（伪HDR）==========
    float luminance = dot(finalColor, vec3(0.299, 0.587, 0.114));  // 亮度
    finalColor += finalColor * luminance * 0.5;  // 亮的地方更亮
    
    // ========== 8. 透明度（边缘更透）==========
    float alpha = 0.9 + fresnel * 0.1;  // 范围 0.9~1.0
    
    FragColor = vec4(finalColor, alpha);
}






