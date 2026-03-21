#version 330 core

// =============================================================================
// 3D Night Life - Light Shaft Fragment Shader
// Additive blended translucent cone to simulate volumetric light
// =============================================================================

in vec3 fragPos;
in float fragHeight;

uniform vec3  shaftColor;
uniform float shaftAlpha;
uniform float fogDensity;

out vec4 fragOutput;

void main(){
    // Alpha tapers from 0 at apex to shaftAlpha at base, then fades with fog
    float alpha  = fragHeight * shaftAlpha;

    // Attenuate deeper cones with fog density
    float depth  = length(fragPos);
    float fogAtt = exp(-fogDensity * depth * 0.002);
    alpha       *= (0.3 + 0.7 * fogAtt);
    alpha        = clamp(alpha, 0.0, 0.6);

    fragOutput   = vec4(shaftColor, alpha);
}
