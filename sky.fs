#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPos;

uniform float time;

// =====================================================================
// 噪声函数 - 用于生成程序化云层
// =====================================================================

// 伪随机哈希函数
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// 2D值噪声
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    // 四个角的随机值
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    // 平滑插值
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// 分形布朗运动 (FBM) - 多层噪声叠加产生自然云层效果
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    // 6层噪声叠加
    for (int i = 0; i < 6; i++) {
        value += amplitude * noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value;
}

// =====================================================================
// 主着色器
// =====================================================================

void main()
{
    // 纹理坐标（0-1范围）
    vec2 uv = TexCoord;
    
    // =========================================
    // 天空渐变背景
    // =========================================
    // 从下到上：浅蓝 -> 深蓝的渐变
    vec3 skyColorBottom = vec3(0.6, 0.8, 1.0);   // 地平线附近 - 浅蓝色
    vec3 skyColorTop = vec3(0.3, 0.5, 0.9);      // 天顶 - 深蓝色
    
    // 使用y坐标的平方使渐变更自然
    float gradientT = pow(uv.y, 0.8);
    vec3 skyColor = mix(skyColorBottom, skyColorTop, gradientT);
    
    // =========================================
    // 流动云层
    // =========================================
    // 云的移动速度（随时间向右飘动）
    float cloudSpeedX = time * 0.03;  // 水平移动
    float cloudSpeedY = time * 0.01;  // 轻微垂直移动
    
    // 云层UV（添加时间偏移产生流动效果）
    vec2 cloudUV = uv * 3.0;  // 缩放UV以控制云的大小
    cloudUV.x += cloudSpeedX;
    cloudUV.y += cloudSpeedY * 0.3;
    
    // 主云层 - 大块的云
    float clouds1 = fbm(cloudUV * 1.0);
    
    // 第二层云 - 更细节的小云，移动速度稍快
    vec2 cloudUV2 = uv * 5.0;
    cloudUV2.x += cloudSpeedX * 1.3;
    cloudUV2.y += cloudSpeedY * 0.5;
    float clouds2 = fbm(cloudUV2 * 1.2);
    
    // 第三层 - 薄云/卷云效果
    vec2 cloudUV3 = uv * 8.0;
    cloudUV3.x += cloudSpeedX * 0.8;
    float clouds3 = fbm(cloudUV3 * 0.8);
    
    // 合并云层
    float cloudDensity = clouds1 * 0.6 + clouds2 * 0.3 + clouds3 * 0.1;
    
    // 云的阈值和边缘柔化
    // 使用 smoothstep 产生柔和的云边缘
    float cloudThreshold = 0.45;
    float cloudSoftness = 0.25;
    float cloudMask = smoothstep(cloudThreshold - cloudSoftness, cloudThreshold + cloudSoftness, cloudDensity);
    
    // 云层颜色（略带蓝色调的白云）
    vec3 cloudColor = vec3(1.0, 1.0, 1.0);
    // 云的阴影部分略暗
    vec3 cloudShadow = vec3(0.85, 0.88, 0.95);
    
    // 根据云层厚度混合亮部和暗部
    float shadowFactor = smoothstep(0.3, 0.7, cloudDensity);
    vec3 finalCloudColor = mix(cloudShadow, cloudColor, shadowFactor);
    
    // =========================================
    // 合成最终颜色
    // =========================================
    // 将云层叠加到天空上
    vec3 finalColor = mix(skyColor, finalCloudColor, cloudMask * 0.9);
    
    // 添加一点大气散射效果（底部略带白色）
    float atmosphericHaze = pow(1.0 - uv.y, 2.0) * 0.15;
    finalColor = mix(finalColor, vec3(1.0), atmosphericHaze);
    
    // 输出最终颜色（完全不透明）
    FragColor = vec4(finalColor, 1.0);
}

