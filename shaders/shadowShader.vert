#version 330 core

// =============================================================================
// 3D Night Life - Shadow Map Depth-Pass Vertex Shader
// Renders scene from the light's point of view to build the shadow map
// =============================================================================

in vec3 vertex;

uniform mat4 lightSpaceMatrix;
uniform mat4 world_mat;

void main() {
    gl_Position = lightSpaceMatrix * world_mat * vec4(vertex, 1.0);
}
