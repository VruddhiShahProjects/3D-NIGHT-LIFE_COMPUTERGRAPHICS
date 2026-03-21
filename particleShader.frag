#version 330 core

// =============================================================================
// 3D Night Life - Particle Fragment Shader (rain streaks)
// =============================================================================

in vec3  fragColor;
in float fragAlpha;

out vec4 fragOutput;

void main(){
    fragOutput = vec4(fragColor, fragAlpha);
}
