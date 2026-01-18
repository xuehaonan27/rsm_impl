#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

// G-Buffer textures (camera view space)
uniform sampler2D uAlbedoTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uPositionTexture;

// RSM textures (camera view space)
uniform sampler2D uRSMFluxTexture;
uniform sampler2D uRSMNormalTexture;
uniform sampler2D uRSMPositionTexture;

// Transform matrices and lighting
uniform mat4 uLightVPMulInvCameraView;
uniform vec3 uLightDirViewSpace;
uniform vec3 uLightColor;

// RSM sampling parameters
uniform int uRSMResolution;
uniform int uVPLNum;
uniform float uMaxSampleRadius;
uniform float uIndirectStrength;
uniform bool uEnableRSM;  // RSM 开关

uniform mat4 uInvView;
uniform vec3 uLightDirWorld;

// Shadow map for traditional shadow mapping
uniform sampler2D uShadowMap;

// VPL sample positions (passed via uniform array)
#define MAX_VPL_NUM 128
uniform vec4 uVPLSamples[MAX_VPL_NUM];

// Calculate indirect illumination from RSM
vec3 calculateIndirectIllumination(vec3 fragPos, vec3 fragNormal) {
    // Transform fragment position to light clip space
    vec4 fragPosLightClip = uLightVPMulInvCameraView * vec4(fragPos, 1.0);
    vec3 fragPosLightNDC = fragPosLightClip.xyz / fragPosLightClip.w;
    vec2 fragPosRSM = fragPosLightNDC.xy * 0.5 + 0.5; // [-1,1] -> [0,1]
    
    vec3 indirectIllum = vec3(0.0);
    float rsmTexelSize = 1.0 / float(uRSMResolution);
    
    for (int i = 0; i < uVPLNum; ++i) {
        // Get sample offset and weight
        vec2 sampleOffset = uVPLSamples[i].xy * uMaxSampleRadius * rsmTexelSize;
        float sampleWeight = uVPLSamples[i].z;
        
        // Sample RSM textures
        vec2 sampleCoord = fragPosRSM + sampleOffset;
        
        // Boundary check
        if (sampleCoord.x < 0.0 || sampleCoord.x > 1.0 || 
            sampleCoord.y < 0.0 || sampleCoord.y > 1.0) {
            continue;
        }
        
        vec3 vplFlux = texture(uRSMFluxTexture, sampleCoord).rgb;
        vec3 vplNormal = texture(uRSMNormalTexture, sampleCoord).rgb;
        vec3 vplPos = texture(uRSMPositionTexture, sampleCoord).rgb;
        
        // Skip invalid VPLs
        if (length(vplFlux) < 0.001 || length(vplNormal) < 0.001) {
            continue;
        }
        
        // Calculate irradiance according to RSM paper equation
        vec3 r = fragPos - vplPos;
        float dist2 = dot(r, r);
        
        if (dist2 < 0.0001) continue; // Avoid singularity
        
        vec3 rNorm = normalize(r);
        
        // Form factors
        float cosTheta_vpl = max(0.0, dot(vplNormal, rNorm));     // VPL emission angle
        float cosTheta_frag = max(0.0, dot(fragNormal, -rNorm));  // Fragment receiving angle
        
        // Irradiance: E = Phi * cos(theta_vpl) * cos(theta_frag) / (dist^4)
        // The dist^4 comes from 1/r^2 for solid angle and 1/r^2 for inverse square law
        vec3 irradiance = vplFlux * cosTheta_vpl * cosTheta_frag / (dist2 * dist2 + 1.0);
        
        // Weight by sampling pattern
        indirectIllum += irradiance * sampleWeight;
    }
    
    // Normalize by number of samples and apply strength
    return indirectIllum * uIndirectStrength / float(uVPLNum);
}

// Calculate shadow factor using RSM position (simplified shadow mapping)
float calculateShadow(vec3 fragPos) {
    vec4 fragPosLightClip = uLightVPMulInvCameraView * vec4(fragPos, 1.0);
    
    // Check if in front of light
    if (fragPosLightClip.w <= 0.0) return 1.0;
    
    vec3 ndc = fragPosLightClip.xyz / fragPosLightClip.w;
    
    // Check if in light frustum
    if (ndc.x < -1.0 || ndc.x > 1.0 ||
        ndc.y < -1.0 || ndc.y > 1.0 ||
        ndc.z < 0.0 || ndc.z > 1.0) {
        return 1.0;
    }
    
    vec2 uv = ndc.xy * 0.5 + 0.5;
    float fragDepth = ndc.z; // OpenGL NDC z is [0,1] for ortho projection!
    
    float shadowMapDepth = texture(uShadowMap, uv).r;
    
    float bias = 0.005;
    float distanceBias = fragDepth * 0.0005;
    bias = bias + distanceBias;
    return (fragDepth - bias) > shadowMapDepth ? 0.0 : 1.0;
}

void main() {
    // Sample G-Buffer
    vec3 albedo = texture(uAlbedoTexture, vTexCoord).rgb;
    vec3 normal = texture(uNormalTexture, vTexCoord).rgb;
    vec3 position = texture(uPositionTexture, vTexCoord).rgb;
    
    // Check if this is a valid pixel (has geometry)
    if (length(normal) < 0.5) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    normal = normalize(normal);
    
    // Direct lighting (Lambertian)
    float NdotL = max(dot(normal, -uLightDirViewSpace), 0.0);
    vec3 directLight = albedo * uLightColor * NdotL;
    
    // Ambient term
    vec3 ambient = albedo * 0.1;

    
    vec3 finalColor;
    
    if (uEnableRSM) {
        // RSM 模式：直接光 + 间接光 + 环境光
        vec3 indirectLight = calculateIndirectIllumination(position, normal) * albedo;
        indirectLight*=100.0;
        finalColor = directLight + indirectLight + ambient;
    } else {
        // 普通 Shadow Mapping 模式：直接光 * 阴影因子 + 环境光
        float shadow = calculateShadow(position);
        finalColor = directLight * shadow + ambient;
    }
    
    FragColor = vec4(finalColor, 1.0);
}
