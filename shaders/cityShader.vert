#version 330 core

// =============================================================================
// 3D Night Life - City Geometry Vertex Shader
// Handles buildings, roads, sidewalks with Phong lighting
// =============================================================================

in vec3 vertex;
in vec3 color;
in vec3 normal;
in vec2 texCoord;

out vec3 fragPos;
out vec3 fragNormal;
out vec3 fragColor;
out vec2 fragTexCoord;
out float fragDepth;

uniform mat4 projection_mat;
uniform mat4 view_mat;
uniform mat4 world_mat;

void main(){
    vec4 worldPos  = world_mat * vec4(vertex, 1.0);
    vec4 viewPos   = view_mat * worldPos;

    fragPos      = vec3(viewPos);
    fragDepth    = -viewPos.z;

    mat3 NM      = mat3(transpose(inverse(view_mat * world_mat)));
    fragNormal   = normalize(NM * normal);
    fragColor    = color;
    fragTexCoord = texCoord;

    gl_Position  = projection_mat * viewPos;
}
