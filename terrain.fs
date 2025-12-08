#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoord;
in float Height;  // 顶点阶段传过来的高度
  
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform float alpha;  // 供外部调节透明度

void main()
{
    // 随高度渐变的颜色：底部偏草绿，中段泛黄，上面带点岩色
    vec3 lowColor = vec3(0.25, 0.55, 0.25);
    vec3 midColor = vec3(0.45, 0.60, 0.30);
    vec3 highColor = vec3(0.65, 0.65, 0.60);
    
    // 使用smoothstep实现平滑过渡
    float heightFactor = clamp(Height / 0.05, 0.0, 1.0);  // 归一化高度到[0,1]
    vec3 baseColor = mix(lowColor, midColor, smoothstep(0.0, 0.5, heightFactor));
    baseColor = mix(baseColor, highColor, smoothstep(0.5, 1.0, heightFactor));
    
    // 坡度越陡越接近岩石色
    vec3 norm = normalize(Normal);
    float slope = 1.0 - abs(dot(norm, vec3(0.0, 1.0, 0.0)));  // 0=平坦, 1=垂直
    vec3 rockColor = vec3(0.50, 0.48, 0.45);  // 岩石灰
    baseColor = mix(baseColor, rockColor, smoothstep(0.4, 0.8, slope) * 0.3);  // 陡坡略带岩石色
    
    // 常规 Phong 光照：先算环境光
    float ambientStrength = 0.75;
    vec3 ambient = ambientStrength * lightColor;
  	
    // 然后是漫反射
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 镜面项弱一些
    float specularStrength = 0.2;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);  // 较低的光泽度
    vec3 specular = specularStrength * spec * lightColor;  
    
    // 简单做个 AO，低处稍暗
    float ao = 0.85 + 0.15 * heightFactor;
    
    // 最终颜色计算
    vec3 result = (ambient + diffuse + specular) * baseColor * ao;
    
    FragColor = vec4(result, alpha);
}

