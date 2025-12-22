#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uDisplayMode;  // 0=final, 1=albedo, 2=normal, 3=position, 4=rsm_flux, 5=rsm_normal, 6=rsm_position

void main() {
    vec3 color = texture(uTexture, vTexCoord).rgb;
    
    if (uDisplayMode == 0) {
        // Final result with gamma correction
        color = pow(color, vec3(1.0 / 2.2));
    } else if (uDisplayMode == 2 || uDisplayMode == 5) {
        // Normal visualization: remap from [-1,1] to [0,1]
        color = color * 0.5 + 0.5;
    } else if (uDisplayMode == 3 || uDisplayMode == 6) {
        // Position visualization: normalize for display
        color = abs(color) * 0.1;
    }
    // Albedo and flux are displayed as-is
    
    FragColor = vec4(color, 1.0);
}
