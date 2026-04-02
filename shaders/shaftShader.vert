#version 330 core

// =============================================================================
// 3D Night Life - Light Shaft Vertex Shader
// Renders translucent cones from streetlights / headlights through fog
// =============================================================================

in vec3 vertex;

out vec3 fragPos;
out float fragHeight;  // 0=apex, 1=base – for alpha gradient

uniform mat4 projection_mat;
uniform mat4 view_mat;
uniform mat4 world_mat;

void main(){
    vec4 viewPos = view_mat * world_mat * vec4(vertex, 1.0);
    fragPos      = vec3(viewPos);
    fragHeight   = clamp(vertex.y / -10.0, 0.0, 1.0); // cone opens downward
    gl_Position  = projection_mat * viewPos;
}
