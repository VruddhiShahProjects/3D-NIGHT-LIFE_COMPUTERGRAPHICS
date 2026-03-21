#version 330 core

// =============================================================================
// 3D Night Life - Particle Vertex Shader (rain streaks)
// =============================================================================

in vec3 vertex;
in vec3 color;

out vec3 fragColor;
out float fragAlpha;

uniform mat4 projection_mat;
uniform mat4 view_mat;
uniform mat4 world_mat;
uniform float time;
uniform float fogDensity;

void main(){
    vec4 viewPos = view_mat * world_mat * vec4(vertex, 1.0);
    fragColor    = color;

    // Fade rain with depth and fog
    float depth  = -viewPos.z;
    float fogAtt = exp(-fogDensity * depth * 0.0003);
    fragAlpha    = clamp(fogAtt * 0.6, 0.0, 0.6);

    gl_Position  = projection_mat * viewPos;
    gl_PointSize = 1.5;
}
