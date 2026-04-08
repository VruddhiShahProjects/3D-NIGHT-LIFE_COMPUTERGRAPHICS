#version 330 core

// =============================================================================
// 3D Night Life - Shadow Map Depth-Pass Fragment Shader
// No colour output needed — the depth buffer IS the shadow map
// =============================================================================

void main() {
    // gl_FragDepth is written automatically from gl_Position.z / gl_Position.w
}
