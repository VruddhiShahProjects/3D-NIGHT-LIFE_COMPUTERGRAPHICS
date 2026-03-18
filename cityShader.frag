#version 330 core

// =============================================================================
// 3D Night Life - City Geometry Fragment Shader
// Features:
//   - Phong shading with multiple colored point lights
//   - Depth-based volumetric fog with exponential density
//   - Neon sign flickering effect
//   - Ambient night sky
// =============================================================================

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragColor;
in vec2 fragTexCoord;
in float fragDepth;

// ---- Material ----
uniform vec3  mat_ambient;
uniform vec3  mat_diffuse;
uniform vec3  mat_specular;
uniform float mat_shininess;
uniform int   useVertexColor;   // 1=use fragColor, 0=use mat_diffuse only

// ---- Up to 16 point lights ----
#define MAX_LIGHTS 16
uniform int   numLights;
uniform vec3  lightPos[MAX_LIGHTS];       // world space
uniform vec3  lightColor[MAX_LIGHTS];
uniform float lightIntensity[MAX_LIGHTS];
uniform float lightRadius[MAX_LIGHTS];    // attenuation falloff radius
uniform mat4  view_mat;

// ---- Fog ----
uniform float fogDensity;    // 0.0 = no fog, 1.0 = thick fog
uniform vec3  fogColor;
uniform float fogStart;
uniform float fogEnd;

// ---- Headlight (spotlight on vehicle) ----
uniform int   hasHeadlight;
uniform vec3  headlightPos;     // world space
uniform vec3  headlightDir;     // world space direction
uniform float headlightCone;    // half angle degrees
uniform vec3  headlightColor;
uniform float headlightIntensity;

// ---- Neon / emission ----
uniform int   isEmissive;     // 1 if this surface emits light (neon sign)
uniform vec3  emissiveColor;
uniform float emissiveStrength;

// ---- Rain toggle ----
uniform int   rainEnabled;
uniform float time;

// ---- Texture ----
uniform int   hasTexture;
uniform sampler2D tex;

out vec4 fragOutput;

// -------------------------------------------------------------------
// Compute fog factor  (exponential squared)
// -------------------------------------------------------------------
float fogFactor(float depth){
    float d = max(0.0, depth - fogStart);
    float f = exp(-fogDensity * fogDensity * d * d * 0.0001);
    return clamp(f, 0.0, 1.0);
}

// -------------------------------------------------------------------
// Point light Phong contribution with quadratic attenuation
// -------------------------------------------------------------------
vec3 pointLightContrib(int i, vec3 N, vec3 V, vec3 baseColor){
    vec3  Lp     = vec3(view_mat * vec4(lightPos[i], 1.0));
    vec3  L      = Lp - fragPos;
    float dist   = length(L);
    L = normalize(L);

    // Attenuation: 1/(1 + (dist/radius)^2)
    float r      = lightRadius[i];
    float atten  = 1.0 / (1.0 + (dist/r)*(dist/r));
    atten       *= lightIntensity[i];

    float NdL    = max(dot(N, L), 0.0);
    vec3  diff   = lightColor[i] * mat_diffuse * baseColor * NdL * atten;

    vec3  R      = reflect(-L, N);
    float spec   = (NdL > 0.0) ? pow(max(dot(R, V), 0.0), mat_shininess) : 0.0;
    vec3  specC  = lightColor[i] * mat_specular * spec * atten;

    return diff + specC;
}

// -------------------------------------------------------------------
// Headlight / spotlight contribution
// -------------------------------------------------------------------
vec3 headlightContrib(vec3 N, vec3 V, vec3 baseColor){
    if(hasHeadlight == 0) return vec3(0.0);

    vec3 hlPosV  = vec3(view_mat * vec4(headlightPos, 1.0));
    vec3 hlDirV  = normalize(mat3(view_mat) * headlightDir);

    vec3  L_hl   = normalize(hlPosV - fragPos);
    vec3  Vobj   = -L_hl;

    float cosTheta  = cos(radians(headlightCone));
    float angFactor = pow(max(dot(hlDirV, Vobj) - cosTheta, 0.0), 4.0);

    float dist   = length(hlPosV - fragPos);
    float atten  = headlightIntensity / (1.0 + 0.001*dist*dist);

    float NdL    = max(dot(N, L_hl), 0.0);
    vec3  diff   = headlightColor * mat_diffuse * baseColor * NdL * angFactor * atten;

    vec3  R      = reflect(-L_hl, N);
    float spec   = (NdL > 0.0) ? pow(max(dot(R, V), 0.0), mat_shininess) : 0.0;
    vec3  specC  = headlightColor * mat_specular * spec * angFactor * atten;

    return diff + specC;
}

// -------------------------------------------------------------------
// Rain shimmer on wet surfaces (subtle normal-based sparkle)
// -------------------------------------------------------------------
float rainShimmer(){
    if(rainEnabled == 0) return 0.0;
    // Simple hash shimmer based on position and time
    float t = time * 0.5;
    float h = fract(sin(dot(fragTexCoord, vec2(127.1, 311.7)) + t) * 43758.5453);
    return h * 0.15;
}

void main(){
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(-fragPos);

    // Base color: texture or vertex color or material
    vec3 baseColor;
    if(hasTexture == 1){
        baseColor = texture(tex, fragTexCoord).rgb;
    } else if(useVertexColor == 1){
        baseColor = fragColor;
    } else {
        baseColor = mat_diffuse;
    }

    // Emissive surfaces (neon signs, window glow) just output their color
    if(isEmissive == 1){
        vec3 emColor = emissiveColor * emissiveStrength;
        // Apply fog to emissive too
        float ff = fogFactor(fragDepth);
        vec3 finalColor = mix(fogColor, emColor, ff);
        fragOutput = vec4(finalColor, 1.0);
        return;
    }

    // Night ambient (dim bluish sky)
    vec3 ambient = mat_ambient * vec3(0.02, 0.02, 0.05);

    // Accumulate point lights
    vec3 lighting = vec3(0.0);
    for(int i = 0; i < numLights && i < MAX_LIGHTS; i++){
        lighting += pointLightContrib(i, N, V, baseColor);
    }

    // Headlight
    lighting += headlightContrib(N, V, baseColor);

    // Rain shimmer
    lighting += baseColor * rainShimmer();

    vec3 litColor = ambient * baseColor + lighting;

    // Fog
    float ff = fogFactor(fragDepth);
    vec3 finalColor = mix(fogColor, litColor, ff);

    fragOutput = vec4(finalColor, 1.0);
}
