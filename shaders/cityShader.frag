#version 330 core

// =============================================================================
// 3D Night Life - City Geometry Fragment Shader
// T7/T8 compliant: toggle uniforms, professor naming, Blinn-Phong, spotlight fix
// =============================================================================

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragColor;
in vec2 fragTexCoord;
in float fragDepth;

// ---- Material (professor convention) ----
uniform vec3  ambient_color;
uniform vec3  diffuse_color;
uniform vec3  specular_color;
uniform float shine;

// ---- Separate coefficient uniforms ----
uniform float coefA;
uniform float coefD;
uniform float coefS;

// ---- T7 toggle uniforms ----
uniform int ambientOn;
uniform int diffuseOn;
uniform int specularOn;
uniform int useHalfVector;

uniform int   useVertexColor;

// ---- Up to 16 point lights ----
#define MAX_LIGHTS 16
uniform int   numLights;
uniform vec3  lightPos[MAX_LIGHTS];
uniform vec3  lightColor[MAX_LIGHTS];
uniform float lightIntensity[MAX_LIGHTS];
uniform float lightRadius[MAX_LIGHTS];
uniform mat4  view_mat;

// ---- Fog ----
uniform float fogDensity;
uniform vec3  fogColor;
uniform float fogStart;
uniform float fogEnd;

// ---- Headlight / spotlight ----
uniform int   hasHeadlight;
uniform int   useSpotlight;
uniform vec3  headlightPos;
uniform vec3  headlightDir;
uniform float headlightCone;
uniform vec3  headlightColor;
uniform float headlightIntensity;
uniform float spotlightAtten;

// ---- Neon / emission ----
uniform int   isEmissive;
uniform vec3  emissiveColor;
uniform float emissiveStrength;

// ---- Rain ----
uniform int   rainEnabled;
uniform float time;

// ---- Texture ----
uniform int   hasTexture;
uniform sampler2D tex;

out vec4 fragOutput;

float fogFactor(float depth){
    float d = max(0.0, depth - fogStart);
    float f = exp(-fogDensity * fogDensity * d * d * 0.0001);
    return clamp(f, 0.0, 1.0);
}

vec3 pointLightContrib(int i, vec3 N, vec3 V, vec3 baseColor){
    vec3  Lp   = vec3(view_mat * vec4(lightPos[i], 1.0));
    vec3  L    = Lp - fragPos;
    float dist = length(L);
    L = normalize(L);

    float r     = lightRadius[i];
    float atten = 1.0 / (1.0 + (dist/r)*(dist/r));
    atten      *= lightIntensity[i];

    vec3 diff = vec3(0.0);
    if(diffuseOn != 0){
        diff = coefD * diffuse_color * baseColor * max(dot(L, N), 0.0) * lightColor[i] * atten;
    }

    vec3 specC = vec3(0.0);
    if(specularOn != 0){
        float NdL = max(dot(N, L), 0.0);
        float spec = 0.0;
        if(useHalfVector != 0){
            vec3 H = normalize(L + V);
            spec = pow(max(dot(H, N), 0.0), shine);
        } else {
            vec3 R = reflect(-L, N);
            spec = (NdL > 0.0) ? pow(max(dot(R, V), 0.0), shine) : 0.0;
        }
        specC = coefS * specular_color * spec * lightColor[i] * atten;
    }

    return diff + specC;
}

vec3 headlightContrib(vec3 N, vec3 V, vec3 baseColor){
    if(hasHeadlight == 0) return vec3(0.0);

    vec3 hlPosV = vec3(view_mat * vec4(headlightPos, 1.0));
    vec3 hlDirV = normalize(mat3(view_mat) * headlightDir);
    vec3 L_hl   = normalize(hlPosV - fragPos);
    float NdL   = max(dot(N, L_hl), 0.0);

    float dist = length(hlPosV - fragPos);
    float radAtten = 1.0 / (4.0 * dist * dist + 1.0);
    radAtten *= headlightIntensity;

    float angFactor = 1.0;
    if(useSpotlight != 0){
        vec3  Vobj     = -L_hl;
        float cosTheta = cos(radians(headlightCone));
        angFactor = pow(max(dot(hlDirV, Vobj) - cosTheta, 0.0), spotlightAtten);
    }

    vec3 diff = vec3(0.0);
    if(diffuseOn != 0)
        diff = coefD * diffuse_color * baseColor * NdL * angFactor * radAtten * headlightColor;

    vec3 specC = vec3(0.0);
    if(specularOn != 0){
        float spec = 0.0;
        if(useHalfVector != 0){
            vec3 H = normalize(L_hl + V);
            spec = pow(max(dot(H, N), 0.0), shine);
        } else {
            vec3 R = reflect(-L_hl, N);
            spec = (NdL > 0.0) ? pow(max(dot(R, V), 0.0), shine) : 0.0;
        }
        specC = coefS * specular_color * spec * angFactor * radAtten * headlightColor;
    }

    return diff + specC;
}

float rainShimmer(){
    if(rainEnabled == 0) return 0.0;
    float t = time * 0.5;
    float h = fract(sin(dot(fragTexCoord, vec2(127.1, 311.7)) + t) * 43758.5453);
    return h * 0.15;
}

void main(){
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(-fragPos);

    vec3 baseColor;
    if(hasTexture == 1){
        baseColor = texture(tex, fragTexCoord).rgb;
    } else if(useVertexColor == 1){
        baseColor = fragColor;
    } else {
        baseColor = diffuse_color;
    }

    if(isEmissive == 1){
        vec3 emColor = emissiveColor * emissiveStrength;
        float ff = fogFactor(fragDepth);
        fragOutput = vec4(mix(fogColor, emColor, ff), 1.0);
        return;
    }

    vec3 ambient = vec3(0.0);
    if(ambientOn != 0){
        ambient = coefA * ambient_color * vec3(0.02, 0.02, 0.05);
    }

    vec3 lighting = vec3(0.0);
    for(int i = 0; i < numLights && i < MAX_LIGHTS; i++){
        lighting += pointLightContrib(i, N, V, baseColor);
    }

    lighting += headlightContrib(N, V, baseColor);
    lighting += baseColor * rainShimmer();

    vec3 litColor   = ambient * baseColor + lighting;
    float ff        = fogFactor(fragDepth);
    fragOutput      = vec4(mix(fogColor, litColor, ff), 1.0);
}
