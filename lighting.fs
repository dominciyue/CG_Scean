#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoord;
  
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform sampler2D texture1;
uniform bool hasTexture;
uniform float alpha;  // 透明度控制

void main()
{
    // ������
    float ambientStrength = 0.8;  // 增加环境光强度
    vec3 ambient = ambientStrength * lightColor;
  	
    // ������ 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // ���淴��
    float specularStrength = 0.9;  // 增加镜面反射强度
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec4 textureColor = vec4(1.0);  // 默认白色，完全不透明
    if (hasTexture) {
        textureColor = texture(texture1, TexCoord);
    }
    vec3 result = (ambient + diffuse + specular) * objectColor * textureColor.rgb;
    FragColor = vec4(result, textureColor.a * alpha);  // 使用纹理的alpha通道和uniform alpha
} 