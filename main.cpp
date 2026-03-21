/*
=============================================================================
COSC 3307 – Final Project: 3D Night Life
Team Members:
1.Vruddhi Shah (0685315): City geometry modeling (buildings, roads, lamp posts), shader pipeline (vert/frag), 
  neon flicker system, CMake build setup, and co-authored the project proposal.
2.Devanshi Raulji (0680356): Quaternion camera navigation, Phong multi-light shading, additive light-shaft cones, 
headlight spotlight system (debugging), and keyboard runtime controls.
3.Dhara Rokad (0677661): Exponential fog shader, traffic light state machine, 
lane-based vehicle animation (orientation/wrap-around in progress), rain particle system (planned), UV texture prep,
and integration testing lead.
a
=============================================================================

CONTROLS
─────────────────────────────────────────────────────
Camera navigation
  Arrow UP / DOWN        – Pitch  ±2°
  Arrow LEFT / RIGHT     – Yaw    ±2°
  A / D                  – Roll   ±2°
  W / S                  – Move forward / backward (5 units)
  Q / E                  – Move up / down (5 units)
  R                      – Reset camera to default viewpoint

Fog & atmosphere
  F / f                  – Fog density  +0.1 / -0.1
  G / g                  – Fog colour cycle  (grey / blue / green)
  V                      – Toggle viewpoint preset  (1→2→3)

Lighting controls
  1                      – Toggle street lights ON/OFF
  2                      – Toggle neon signs ON/OFF
  3                      – Toggle traffic light colours cycle
  4                      – Toggle vehicle headlights ON/OFF
  L / l                  – Global light intensity  +0.2 / -0.2

Animation
  P                      – Pause / resume vehicle animation
  + / =                  – Vehicle speed  ×1.5
  - / _                  – Vehicle speed  ÷1.5

Effects
  T                      – Toggle rain particles ON/OFF
  N                      – Cycle neon sign flicker (slow/fast/off)
  H / h                  – Headlight cone angle  +5° / -5°
  X / x                  – Light shaft alpha  +0.05 / -0.05

  ESC / Z                – Quit
=============================================================================
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <GL/glut.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <camera.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Window
// ─────────────────────────────────────────────────────────────────────────────
static const int   WIN_W    = 1024;
static const int   WIN_H    = 768;
static const char* WIN_TITLE = "COSC 3307 – 3D Night Life  |  W/S/A/D/Arrows – camera  |  F/f fog  |  P pause  |  T rain  |  ESC quit";

// ─────────────────────────────────────────────────────────────────────────────
//  Vertex layout (pos + color + normal + texCoord)
// ─────────────────────────────────────────────────────────────────────────────
struct Vertex {
    glm::vec3 pos;       // offset  0
    glm::vec3 color;     // offset 12
    glm::vec3 normal;    // offset 24
    glm::vec2 texCoord;  // offset 36
};                       // stride  44

struct Mesh {
    GLuint vao, vbo, ibo;
    GLuint indexCount;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Light struct
// ─────────────────────────────────────────────────────────────────────────────
struct Light {
    glm::vec3 pos;
    glm::vec3 color;
    float     intensity;
    float     radius;    // attenuation radius
    bool      enabled;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Vehicle struct
// ─────────────────────────────────────────────────────────────────────────────
struct Vehicle {
    glm::vec3 pos;
    glm::vec3 dir;          // normalized travel direction
    float     speed;
    float     t;            // 0..1 along route
    glm::vec3 bodyColor;
    float     routeLen;     // total route length
    // headlight data
    glm::vec3 hlOffset;     // headlight pos relative to vehicle front
    glm::vec3 hlColor;
    float     hlCone;       // degrees
};

// ─────────────────────────────────────────────────────────────────────────────
//  Globals
// ─────────────────────────────────────────────────────────────────────────────
static Camera* g_cam = nullptr;

// Shaders
static GLuint g_shCity      = 0;    // main city geometry (buildings, roads)
static GLuint g_shShaft     = 0;    // light shafts
static GLuint g_shParticle  = 0;    // rain particles

// Meshes
static Mesh* g_road      = nullptr;
static Mesh* g_sidewalk  = nullptr;
static std::vector<Mesh*> g_buildings;
static Mesh* g_streetLampPost = nullptr;
static Mesh* g_lampHead       = nullptr;
static Mesh* g_lightShaft     = nullptr;
static Mesh* g_trafficLight   = nullptr;
static Mesh* g_neonSign       = nullptr;

// Particle buffer (rain)
static GLuint g_rainVAO = 0, g_rainVBO = 0;
static const int RAIN_COUNT = 3000;
struct RainDrop { glm::vec3 pos; glm::vec3 color; float speed; };
static std::vector<RainDrop> g_rainDrops;

// Lights
static std::vector<Light> g_streetLights;
static std::vector<Light> g_neonLights;
static std::vector<Light> g_trafficLights;

// Scene vehicles
static std::vector<Vehicle> g_vehicles;

// --- Fog ---
static float     g_fogDensity  = 0.6f;
static glm::vec3 g_fogColor    = glm::vec3(0.03f, 0.04f, 0.08f);
static int       g_fogColorIdx = 0;

// --- Global toggles ---
static bool  g_streetLightsOn  = true;
static bool  g_neonOn          = true;
static bool  g_headlightsOn    = true;
static bool  g_rainOn          = true;
static bool  g_paused          = false;
static float g_globalLightInt  = 1.0f;
static float g_vehicleSpeed    = 1.0f;
static float g_headlightCone   = 18.0f;
static float g_shaftAlpha      = 0.18f;
static int   g_neonFlicker     = 1;    // 0=off, 1=slow, 2=fast
static int   g_trafficState    = 0;    // 0=red, 1=amber, 2=green
static float g_trafficTimer    = 0.0f;
static int   g_viewPreset      = 0;

static float g_time = 0.0f;

// Temp mesh-build buffers
static std::vector<Vertex> g_vtmp;
static std::vector<unsigned int> g_itmp;

// ─────────────────────────────────────────────────────────────────────────────
//  Shader utilities (same pattern as Assignment 2)
// ─────────────────────────────────────────────────────────────────────────────
static std::string ReadFile(const std::string& p){
    std::ifstream f(p.c_str());
    if(!f.is_open()) throw std::runtime_error("Cannot open: " + p);
    std::string s, l;
    while(std::getline(f,l)) s += l + "\n";
    return s;
}
static GLuint CompileShader(GLenum type, const std::string& src, const std::string& label){
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s,1,&c,NULL);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){ char b[4096]; glGetShaderInfoLog(s,4096,NULL,b);
             throw std::runtime_error("Compile ["+label+"]: "+b); }
    return s;
}
static GLuint LoadShaders(const std::string& base){
    GLuint vs = CompileShader(GL_VERTEX_SHADER,   ReadFile(base+".vert"), base+".vert");
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, ReadFile(base+".frag"), base+".frag");
    GLuint p  = glCreateProgram();
    glAttachShader(p,vs); glAttachShader(p,fs);
    glLinkProgram(p);
    GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ char b[4096]; glGetProgramInfoLog(p,4096,NULL,b);
             throw std::runtime_error("Link ["+base+"]: "+b); }
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}
// Uniform helpers
static void U1f(GLuint p, const char* n, float v)        { GLint l=glGetUniformLocation(p,n); if(l>=0) glUniform1f(l,v); }
static void U1i(GLuint p, const char* n, int v)          { GLint l=glGetUniformLocation(p,n); if(l>=0) glUniform1i(l,v); }
static void U3f(GLuint p, const char* n, glm::vec3 v)    { GLint l=glGetUniformLocation(p,n); if(l>=0) glUniform3fv(l,1,glm::value_ptr(v)); }
static void UM4(GLuint p, const char* n, glm::mat4 m)    { GLint l=glGetUniformLocation(p,n); if(l>=0) glUniformMatrix4fv(l,1,GL_FALSE,glm::value_ptr(m)); }
static void U3fv(GLuint p,const char* n, int cnt, const float* v){ GLint l=glGetUniformLocation(p,n); if(l>=0) glUniform3fv(l,cnt,v); }
static void U1fv(GLuint p,const char* n, int cnt, const float* v){ GLint l=glGetUniformLocation(p,n); if(l>=0) glUniform1fv(l,cnt,v); }
static void U1iv(GLuint p,const char* n, int cnt, const int*   v){ GLint l=glGetUniformLocation(p,n); if(l>=0) glUniform1iv(l,cnt,v); }

// ─────────────────────────────────────────────────────────────────────────────
//  Mesh builder helpers
// ─────────────────────────────────────────────────────────────────────────────
static Mesh* UploadMesh(){
    Mesh* m = new Mesh();
    m->indexCount = (GLuint)g_itmp.size();
    glGenVertexArrays(1,&m->vao); glBindVertexArray(m->vao);
    glGenBuffers(1,&m->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*g_vtmp.size(), g_vtmp.data(), GL_DYNAMIC_DRAW);
    glGenBuffers(1,&m->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*g_itmp.size(), g_itmp.data(), GL_STATIC_DRAW);
    g_vtmp.clear(); g_itmp.clear();
    return m;
}

static void BindCityAttribs(Mesh* m, GLuint prog){
    glBindVertexArray(m->vao);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
    GLint va = glGetAttribLocation(prog,"vertex");
    if(va>=0){ glVertexAttribPointer(va,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);  glEnableVertexAttribArray(va); }
    GLint ca = glGetAttribLocation(prog,"color");
    if(ca>=0){ glVertexAttribPointer(ca,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)12); glEnableVertexAttribArray(ca); }
    GLint na = glGetAttribLocation(prog,"normal");
    if(na>=0){ glVertexAttribPointer(na,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)24); glEnableVertexAttribArray(na); }
    GLint ta = glGetAttribLocation(prog,"texCoord");
    if(ta>=0){ glVertexAttribPointer(ta,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)36); glEnableVertexAttribArray(ta); }
}

// Add a quad (two triangles) from 4 vertices
static void AddQuad(unsigned int a, unsigned int b, unsigned int c, unsigned int d){
    g_itmp.push_back(a); g_itmp.push_back(b); g_itmp.push_back(c);
    g_itmp.push_back(a); g_itmp.push_back(c); g_itmp.push_back(d);
}

// Push a box (axis-aligned) with per-face normals and checkerboard UVs
static Mesh* MakeBox(float w, float h, float d, glm::vec3 col){
    float hw=w*0.5f, hd=d*0.5f;
    // 8 corners, but we duplicate for normals
    // +Y (top)
    unsigned int base = (unsigned int)g_vtmp.size();
    glm::vec3 top(0,1,0), bot(0,-1,0), front(0,0,1), back(0,0,-1), right(1,0,0), left(-1,0,0);

    auto PV=[&](float x, float y, float z, glm::vec3 n, float u, float v){
        Vertex vt; vt.pos=glm::vec3(x,y,z); vt.color=col; vt.normal=n; vt.texCoord=glm::vec2(u,v);
        g_vtmp.push_back(vt);
    };
    // Top
    PV(-hw,h,-hd,top,0,0); PV(hw,h,-hd,top,1,0); PV(hw,h,hd,top,1,1); PV(-hw,h,hd,top,0,1);
    AddQuad(base,base+1,base+2,base+3); base+=4;
    // Bottom
    PV(-hw,0,hd,bot,0,0); PV(hw,0,hd,bot,1,0); PV(hw,0,-hd,bot,1,1); PV(-hw,0,-hd,bot,0,1);
    AddQuad(base,base+1,base+2,base+3); base+=4;
    // Front (+Z)
    PV(-hw,0,hd,front,0,0); PV(hw,0,hd,front,1,0); PV(hw,h,hd,front,1,1); PV(-hw,h,hd,front,0,1);
    AddQuad(base,base+1,base+2,base+3); base+=4;
    // Back (-Z)
    PV(hw,0,-hd,back,0,0); PV(-hw,0,-hd,back,1,0); PV(-hw,h,-hd,back,1,1); PV(hw,h,-hd,back,0,1);
    AddQuad(base,base+1,base+2,base+3); base+=4;
    // Right (+X)
    PV(hw,0,hd,right,0,0); PV(hw,0,-hd,right,1,0); PV(hw,h,-hd,right,1,1); PV(hw,h,hd,right,0,1);
    AddQuad(base,base+1,base+2,base+3); base+=4;
    // Left (-X)
    PV(-hw,0,-hd,left,0,0); PV(-hw,0,hd,left,1,0); PV(-hw,h,hd,left,1,1); PV(-hw,h,-hd,left,0,1);
    AddQuad(base,base+1,base+2,base+3);
    return UploadMesh();
}

// Flat quad on the XZ plane
static Mesh* MakeFlat(float w, float d, glm::vec3 col, float tileU=1.f, float tileV=1.f){
    float hw=w*0.5f, hd=d*0.5f;
    glm::vec3 n(0,1,0);
    g_vtmp.push_back({glm::vec3(-hw,0,-hd),col,n,glm::vec2(0,0)});
    g_vtmp.push_back({glm::vec3( hw,0,-hd),col,n,glm::vec2(tileU,0)});
    g_vtmp.push_back({glm::vec3( hw,0, hd),col,n,glm::vec2(tileU,tileV)});
    g_vtmp.push_back({glm::vec3(-hw,0, hd),col,n,glm::vec2(0,tileV)});
    AddQuad(0,1,2,3);
    return UploadMesh();
}

// Cylinder (for lamp posts)
static Mesh* MakeCylinder(float r, float h, int segs, glm::vec3 col){
    unsigned int base = 0;
    for(int i=0;i<segs;i++){
        float a0 = 2.f*glm::pi<float>()*i/segs;
        float a1 = 2.f*glm::pi<float>()*(i+1)/segs;
        glm::vec3 n0(cosf(a0),0,sinf(a0)), n1(cosf(a1),0,sinf(a1));
        unsigned int vi = (unsigned int)g_vtmp.size();
        g_vtmp.push_back({glm::vec3(r*cosf(a0),0,r*sinf(a0)),  col,n0,glm::vec2(0,0)});
        g_vtmp.push_back({glm::vec3(r*cosf(a1),0,r*sinf(a1)),  col,n1,glm::vec2(1,0)});
        g_vtmp.push_back({glm::vec3(r*cosf(a1),h,r*sinf(a1)),  col,n1,glm::vec2(1,1)});
        g_vtmp.push_back({glm::vec3(r*cosf(a0),h,r*sinf(a0)),  col,n0,glm::vec2(0,1)});
        AddQuad(vi,vi+1,vi+2,vi+3);
    }
    return UploadMesh();
}

// Cone mesh for light shafts
static Mesh* MakeCone(float r, float h, int segs){
    glm::vec3 col(1,1,0.8f);
    unsigned int apex = (unsigned int)g_vtmp.size();
    g_vtmp.push_back({glm::vec3(0,0,0), col, glm::vec3(0,1,0), glm::vec2(0.5f,0)});
    for(int i=0;i<segs;i++){
        float a0 = 2.f*glm::pi<float>()*i/segs;
        float a1 = 2.f*glm::pi<float>()*(i+1)/segs;
        unsigned int vi = (unsigned int)g_vtmp.size();
        g_vtmp.push_back({glm::vec3(r*cosf(a0),-h,r*sinf(a0)), col, glm::vec3(0,-1,0), glm::vec2(0,1)});
        g_vtmp.push_back({glm::vec3(r*cosf(a1),-h,r*sinf(a1)), col, glm::vec3(0,-1,0), glm::vec2(1,1)});
        g_itmp.push_back(apex); g_itmp.push_back(vi); g_itmp.push_back(vi+1);
    }
    return UploadMesh();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Set standard city uniforms on city shader
// ─────────────────────────────────────────────────────────────────────────────
static void SetCityUniforms(GLuint prog, glm::mat4 world,
                            glm::vec3 matAmb, glm::vec3 matDif, glm::vec3 matSpec,
                            float shin, int useVC, int emissive,
                            glm::vec3 emCol, float emStr, int hasTexture)
{
    glUseProgram(prog);
    glm::mat4 view = g_cam->GetViewMatrix(NULL);
    glm::mat4 proj = g_cam->GetProjectionMatrix(NULL);
    UM4(prog,"world_mat",      world);
    UM4(prog,"view_mat",       view);
    UM4(prog,"projection_mat", proj);
    U3f(prog,"mat_ambient",  matAmb);
    U3f(prog,"mat_diffuse",  matDif);
    U3f(prog,"mat_specular", matSpec);
    U1f(prog,"mat_shininess",shin);
    U1i(prog,"useVertexColor", useVC);
    U1i(prog,"isEmissive",     emissive);
    U3f(prog,"emissiveColor",  emCol);
    U1f(prog,"emissiveStrength", emStr);
    U1i(prog,"hasTexture",     hasTexture);
    U3f(prog,"fogColor",       g_fogColor);
    U1f(prog,"fogDensity",     g_fogDensity);
    U1f(prog,"fogStart",       5.0f);
    U1f(prog,"fogEnd",         800.0f);
    U1f(prog,"time",           g_time);
    U1i(prog,"rainEnabled",    g_rainOn ? 1 : 0);

    // Pack all active lights
    static glm::vec3 posArr[16];
    static glm::vec3 colArr[16];
    static float     intArr[16];
    static float     radArr[16];
    int cnt = 0;
    // Street lights
    if(g_streetLightsOn){
        for(auto& lp : g_streetLights){
            if(!lp.enabled || cnt>=16) continue;
            posArr[cnt] = lp.pos;
            colArr[cnt] = lp.color * g_globalLightInt;
            intArr[cnt] = lp.intensity;
            radArr[cnt] = lp.radius;
            cnt++;
        }
    }
    // Neon lights
    if(g_neonOn){
        for(auto& lp : g_neonLights){
            if(!lp.enabled || cnt>=16) continue;
            float flicker = 1.0f;
            if(g_neonFlicker==1) flicker = 0.85f + 0.15f*sinf(g_time*3.0f + lp.pos.x*0.1f);
            if(g_neonFlicker==2) flicker = (sinf(g_time*15.f + lp.pos.z*0.2f) > 0.3f) ? 1.f : 0.4f;
            posArr[cnt] = lp.pos;
            colArr[cnt] = lp.color * g_globalLightInt * flicker;
            intArr[cnt] = lp.intensity;
            radArr[cnt] = lp.radius;
            cnt++;
        }
    }
    // Traffic light
    {
        glm::vec3 tc = (g_trafficState==0) ? glm::vec3(1,0,0) :
                       (g_trafficState==1) ? glm::vec3(1,0.6f,0) :
                                             glm::vec3(0,1,0);
        for(auto& lp : g_trafficLights){
            if(!lp.enabled || cnt>=16) continue;
            posArr[cnt] = lp.pos;
            colArr[cnt] = tc * g_globalLightInt;
            intArr[cnt] = lp.intensity;
            radArr[cnt] = lp.radius;
            cnt++;
        }
    }

    U1i(prog,"numLights", cnt);
    if(cnt>0){
        U3fv(prog,"lightPos",   cnt, (float*)posArr);
        U3fv(prog,"lightColor", cnt, (float*)colArr);
        U1fv(prog,"lightIntensity", cnt, intArr);
        U1fv(prog,"lightRadius",    cnt, radArr);
    }
    UM4(prog,"view_mat", view);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Draw a mesh
// ─────────────────────────────────────────────────────────────────────────────
static void DrawMesh(Mesh* m, GLuint prog){
    BindCityAttribs(m, prog);
    glDrawElements(GL_TRIANGLES, m->indexCount, GL_UNSIGNED_INT, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Draw a light shaft cone (additive blending)
// ─────────────────────────────────────────────────────────────────────────────
static void DrawShaft(Mesh* m, glm::mat4 world, glm::vec3 col, float alpha){
    glUseProgram(g_shShaft);
    glm::mat4 view = g_cam->GetViewMatrix(NULL);
    glm::mat4 proj = g_cam->GetProjectionMatrix(NULL);
    UM4(g_shShaft,"world_mat",      world);
    UM4(g_shShaft,"view_mat",       view);
    UM4(g_shShaft,"projection_mat", proj);
    U3f(g_shShaft,"shaftColor",  col);
    U1f(g_shShaft,"shaftAlpha",  alpha);
    U1f(g_shShaft,"fogDensity",  g_fogDensity);
    BindCityAttribs(m, g_shShaft);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDrawElements(GL_TRIANGLES, m->indexCount, GL_UNSIGNED_INT, 0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Building factory – creates varied height/width buildings
// ─────────────────────────────────────────────────────────────────────────────
struct BuildingDef { float x, z, w, h, d; glm::vec3 col; };

// Grid-layout city block
static void CreateCityGeometry(){
    // Road: 600×600 dark asphalt
    g_road = MakeFlat(600, 600, glm::vec3(0.08f,0.08f,0.1f), 20.f, 20.f);

    // Sidewalks (4 strips)
    g_sidewalk = MakeFlat(600, 20, glm::vec3(0.25f,0.25f,0.28f), 10.f, 1.f);

    // --- Buildings ---
    // Left block (negative X)
    std::vector<BuildingDef> defs = {
        // x    z      w     h     d     color
        {-160.f, -200.f, 45.f, 120.f, 45.f, glm::vec3(0.20f,0.20f,0.30f)},
        {-210.f, -200.f, 40.f,  80.f, 40.f, glm::vec3(0.15f,0.18f,0.22f)},
        {-160.f, -140.f, 45.f, 160.f, 40.f, glm::vec3(0.18f,0.15f,0.25f)},
        {-210.f, -140.f, 35.f,  90.f, 35.f, glm::vec3(0.22f,0.20f,0.18f)},
        {-160.f,  -70.f, 50.f, 100.f, 50.f, glm::vec3(0.12f,0.15f,0.20f)},
        {-215.f,  -70.f, 35.f,  70.f, 35.f, glm::vec3(0.18f,0.22f,0.28f)},
        {-160.f,   10.f, 45.f, 200.f, 45.f, glm::vec3(0.25f,0.22f,0.30f)},
        {-210.f,   10.f, 35.f, 130.f, 35.f, glm::vec3(0.20f,0.18f,0.25f)},
        {-160.f,  100.f, 40.f,  75.f, 40.f, glm::vec3(0.15f,0.20f,0.22f)},
        {-215.f,  100.f, 30.f,  60.f, 30.f, glm::vec3(0.22f,0.18f,0.18f)},
        {-160.f,  160.f, 50.f, 140.f, 50.f, glm::vec3(0.18f,0.22f,0.28f)},
        {-215.f,  160.f, 35.f,  95.f, 35.f, glm::vec3(0.28f,0.25f,0.30f)},

        // Right block (+X)
        { 160.f, -200.f, 45.f, 110.f, 45.f, glm::vec3(0.20f,0.18f,0.28f)},
        { 210.f, -200.f, 40.f,  70.f, 40.f, glm::vec3(0.15f,0.20f,0.22f)},
        { 160.f, -140.f, 50.f, 180.f, 50.f, glm::vec3(0.18f,0.15f,0.22f)},
        { 210.f, -140.f, 35.f, 100.f, 35.f, glm::vec3(0.25f,0.22f,0.28f)},
        { 160.f,  -70.f, 45.f,  90.f, 45.f, glm::vec3(0.20f,0.25f,0.30f)},
        { 215.f,  -70.f, 30.f,  60.f, 30.f, glm::vec3(0.15f,0.18f,0.20f)},
        { 160.f,   10.f, 45.f, 220.f, 45.f, glm::vec3(0.22f,0.20f,0.30f)},
        { 215.f,   10.f, 30.f, 140.f, 30.f, glm::vec3(0.18f,0.22f,0.25f)},
        { 160.f,  100.f, 50.f,  85.f, 50.f, glm::vec3(0.25f,0.20f,0.22f)},
        { 215.f,  100.f, 30.f,  55.f, 30.f, glm::vec3(0.18f,0.25f,0.28f)},
        { 160.f,  160.f, 45.f, 150.f, 45.f, glm::vec3(0.20f,0.18f,0.25f)},
        { 215.f,  160.f, 35.f,  80.f, 35.f, glm::vec3(0.22f,0.20f,0.18f)},

        // Far back buildings
        {  -80.f, -260.f, 55.f, 130.f, 50.f, glm::vec3(0.15f,0.18f,0.25f)},
        {    0.f, -260.f, 50.f, 100.f, 45.f, glm::vec3(0.20f,0.22f,0.28f)},
        {   80.f, -260.f, 55.f, 170.f, 50.f, glm::vec3(0.18f,0.15f,0.22f)},
        {  -80.f,  240.f, 50.f, 110.f, 45.f, glm::vec3(0.22f,0.20f,0.30f)},
        {    0.f,  240.f, 55.f,  90.f, 50.f, glm::vec3(0.25f,0.22f,0.28f)},
        {   80.f,  240.f, 50.f, 140.f, 45.f, glm::vec3(0.18f,0.25f,0.22f)},
    };

    for(auto& d : defs){
        Mesh* b = MakeBox(d.w, d.h, d.d, d.col);
        g_buildings.push_back(b);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Streetlight setup
// ─────────────────────────────────────────────────────────────────────────────
static void CreateStreetLights(){
    // Positions along both sides of the main road (X=±70)
    float lampZ[] = {-200,-140,-80,-20,40,100,160,200};
    float sides[] = {-75.f, 75.f};
    for(float sx : sides){
        for(float sz : lampZ){
            Light l;
            l.pos       = glm::vec3(sx, 22.0f, sz);
            l.color     = glm::vec3(1.0f, 0.90f, 0.65f);
            l.intensity = 1.4f;
            l.radius    = 80.f;
            l.enabled   = true;
            g_streetLights.push_back(l);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Neon lights (on building facades)
// ─────────────────────────────────────────────────────────────────────────────
static void CreateNeonLights(){
    struct NL { glm::vec3 pos; glm::vec3 col; };
    std::vector<NL> nl = {
        {{-137.f,25.f,-185.f}, {1,0.1f,0.8f}},  // pink
        {{-137.f,40.f,-130.f}, {0,0.8f,1.f}},    // cyan
        {{-137.f,18.f,-55.f},  {1,0.6f,0.f}},    // orange
        {{-137.f,35.f, 30.f},  {0.4f,1.f,0.4f}}, // green
        {{-137.f,22.f,110.f},  {1,1.f,0.2f}},    // yellow
        {{-137.f,50.f,175.f},  {0.6f,0.2f,1.f}}, // purple
        {{ 137.f,28.f,-190.f}, {0,0.8f,1.f}},    // cyan
        {{ 137.f,45.f,-130.f}, {1,0.1f,0.8f}},   // pink
        {{ 137.f,20.f, -60.f}, {0.4f,1.f,0.4f}}, // green
        {{ 137.f,38.f,  25.f}, {1,0.6f,0.f}},    // orange
        {{ 137.f,15.f, 115.f}, {1,1.f,0.2f}},    // yellow
        {{ 137.f,55.f, 170.f}, {0.8f,0.2f,1.f}}, // purple
    };
    for(auto& n : nl){
        Light l;
        l.pos       = n.pos;
        l.color     = n.col;
        l.intensity = 1.8f;
        l.radius    = 60.f;
        l.enabled   = true;
        g_neonLights.push_back(l);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Traffic lights
// ─────────────────────────────────────────────────────────────────────────────
static void CreateTrafficLights(){
    glm::vec3 pos[] = {
        {-78,8,-5}, {78,8,-5}, {-78,8,5}, {78,8,5}
    };
    for(auto& p : pos){
        Light l;
        l.pos       = p;
        l.color     = glm::vec3(1,0,0);
        l.intensity = 2.0f;
        l.radius    = 50.f;
        l.enabled   = true;
        g_trafficLights.push_back(l);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Vehicles – simple box cars on two routes
// ─────────────────────────────────────────────────────────────────────────────
static void CreateVehicles(){
    struct VDef { glm::vec3 pos; glm::vec3 dir; glm::vec3 col; float speed; float t; };
    std::vector<VDef> vd = {
        {{-30.f,0.f,-250.f}, {0.f,0.f, 1.f}, {0.8f,0.2f,0.2f}, 90.f, 0.00f},
        {{-30.f,0.f, -90.f}, {0.f,0.f, 1.f}, {0.2f,0.4f,0.8f}, 70.f, 0.30f},
        {{-30.f,0.f,  80.f}, {0.f,0.f, 1.f}, {0.8f,0.7f,0.2f}, 80.f, 0.60f},
        {{ 30.f,0.f, 250.f}, {0.f,0.f,-1.f}, {0.3f,0.7f,0.3f}, 75.f, 0.00f},
        {{ 30.f,0.f,  90.f}, {0.f,0.f,-1.f}, {0.8f,0.4f,0.1f}, 85.f, 0.40f},
        {{ 30.f,0.f, -80.f}, {0.f,0.f,-1.f}, {0.5f,0.2f,0.7f}, 65.f, 0.70f},
    };
    for(auto& v : vd){
        Vehicle vh;
        vh.pos       = v.pos;
        vh.dir       = v.dir;
        vh.speed     = v.speed;
        vh.t         = v.t;
        vh.bodyColor = v.col;
        vh.routeLen  = 500.f;
        vh.hlColor   = glm::vec3(1,0.95f,0.85f);
        vh.hlCone    = g_headlightCone;
        g_vehicles.push_back(vh);
    }
}

// We'll use a single shared car body mesh and tint via vertex color on the fly
static Mesh* g_carBody    = nullptr;
static Mesh* g_carTop     = nullptr;
static Mesh* g_carWheel   = nullptr;

static void CreateCarMeshes(){
    g_carBody  = MakeBox(8.f, 2.5f, 16.f, glm::vec3(1,1,1)); // white – colored per instance via uniform
    g_carTop   = MakeBox(6.f, 2.0f, 10.f, glm::vec3(0.7f,0.7f,0.7f));
    g_carWheel = MakeCylinder(1.5f, 1.f, 12, glm::vec3(0.15f,0.15f,0.15f));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Rain particles
// ─────────────────────────────────────────────────────────────────────────────
static void InitRain(){
    g_rainDrops.resize(RAIN_COUNT);
    for(auto& d : g_rainDrops){
        d.pos   = glm::vec3((rand()%600)-300.f, (rand()%80)+5.f, (rand()%600)-300.f);
        d.color = glm::vec3(0.6f,0.7f,0.9f);
        d.speed = 50.f + (rand()%60);
    }
    glGenVertexArrays(1,&g_rainVAO); glBindVertexArray(g_rainVAO);
    glGenBuffers(1,&g_rainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, g_rainVBO);
    glBufferData(GL_ARRAY_BUFFER, RAIN_COUNT*sizeof(RainDrop), g_rainDrops.data(), GL_DYNAMIC_DRAW);
}

static void UpdateRain(float dt){
    if(!g_rainOn) return;
    for(auto& d : g_rainDrops){
        d.pos.y -= d.speed * dt;
        if(d.pos.y < 0.f){
            d.pos.y = 80.f + (rand()%30);
            d.pos.x = (rand()%600)-300.f;
            d.pos.z = (rand()%600)-300.f;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, g_rainVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, RAIN_COUNT*sizeof(RainDrop), g_rainDrops.data());
}

static void DrawRain(){
    if(!g_rainOn) return;
    glUseProgram(g_shParticle);
    glm::mat4 view = g_cam->GetViewMatrix(NULL);
    glm::mat4 proj = g_cam->GetProjectionMatrix(NULL);
    glm::mat4 ident(1.f);
    UM4(g_shParticle,"world_mat",      ident);
    UM4(g_shParticle,"view_mat",       view);
    UM4(g_shParticle,"projection_mat", proj);
    U1f(g_shParticle,"time",       g_time);
    U1f(g_shParticle,"fogDensity", g_fogDensity);

    glBindVertexArray(g_rainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_rainVBO);

    GLint va = glGetAttribLocation(g_shParticle,"vertex");
    if(va>=0){ glVertexAttribPointer(va,3,GL_FLOAT,GL_FALSE,sizeof(RainDrop),(void*)0); glEnableVertexAttribArray(va); }
    GLint ca = glGetAttribLocation(g_shParticle,"color");
    if(ca>=0){ glVertexAttribPointer(ca,3,GL_FLOAT,GL_FALSE,sizeof(RainDrop),(void*)12); glEnableVertexAttribArray(ca); }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, RAIN_COUNT);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scene initialization
// ─────────────────────────────────────────────────────────────────────────────
static void InitScene(){
    CreateCityGeometry();
    CreateStreetLights();
    CreateNeonLights();
    CreateTrafficLights();
    CreateCarMeshes();
    InitRain();

    // Lamp post mesh (shared)
    g_streetLampPost = MakeCylinder(0.4f, 20.f, 8, glm::vec3(0.4f,0.4f,0.45f));
    g_lampHead       = MakeBox(2.5f, 1.0f, 2.5f, glm::vec3(0.9f,0.85f,0.7f));
    g_lightShaft     = MakeCone(5.f, 18.f, 16);
    g_trafficLight   = MakeBox(2.f, 6.f, 2.f, glm::vec3(0.15f,0.15f,0.15f));
    g_neonSign       = MakeBox(12.f, 3.f, 0.3f, glm::vec3(1,0.1f,0.8f)); // placeholder pink

    CreateVehicles();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Update (animation)
// ─────────────────────────────────────────────────────────────────────────────
static void Update(float dt){
    if(g_paused) return;
    g_time += dt;

    // Vehicles
    for(auto& v : g_vehicles){
        float spd = v.speed * g_vehicleSpeed;
        v.t += (spd * dt) / v.routeLen;
        if(v.t > 1.f) v.t -= 1.f;
        v.pos += v.dir * spd * dt;
        // Wrap around
        if(v.pos.z >  260.f) v.pos.z = -260.f;
        if(v.pos.z < -260.f) v.pos.z =  260.f;
    }

    // Traffic light cycle
    g_trafficTimer += dt;
    if(g_trafficTimer > 4.f){ g_trafficTimer = 0.f; g_trafficState = (g_trafficState+1)%3; }

    UpdateRain(dt);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Draw scene
// ─────────────────────────────────────────────────────────────────────────────
static void DrawScene(){
    glm::mat4 I(1.f);

    // --- Road ---
    SetCityUniforms(g_shCity, I,
        glm::vec3(0.02f),
        glm::vec3(0.08f,0.08f,0.10f),
        glm::vec3(0.15f),
        12.f, 0, 0, glm::vec3(0), 0.f, 0);
    DrawMesh(g_road, g_shCity);

    // --- Sidewalks (4 sides) ---
    float swZ[] = {-290,-270};
    for(int i=0;i<2;i++){
        glm::mat4 w = glm::translate(I, glm::vec3(0,0.01f,swZ[i]));
        SetCityUniforms(g_shCity, w,
            glm::vec3(0.03f), glm::vec3(0.22f,0.22f,0.24f), glm::vec3(0.1f),
            4.f, 0, 0, glm::vec3(0), 0.f, 0);
        DrawMesh(g_sidewalk, g_shCity);
    }

    // --- Buildings ---
    std::vector<BuildingDef> defs = {
        {-160.f,-200.f,45.f,120.f,45.f,glm::vec3(0.20f,0.20f,0.30f)},
        {-210.f,-200.f,40.f, 80.f,40.f,glm::vec3(0.15f,0.18f,0.22f)},
        {-160.f,-140.f,45.f,160.f,40.f,glm::vec3(0.18f,0.15f,0.25f)},
        {-210.f,-140.f,35.f, 90.f,35.f,glm::vec3(0.22f,0.20f,0.18f)},
        {-160.f, -70.f,50.f,100.f,50.f,glm::vec3(0.12f,0.15f,0.20f)},
        {-215.f, -70.f,35.f, 70.f,35.f,glm::vec3(0.18f,0.22f,0.28f)},
        {-160.f,  10.f,45.f,200.f,45.f,glm::vec3(0.25f,0.22f,0.30f)},
        {-210.f,  10.f,35.f,130.f,35.f,glm::vec3(0.20f,0.18f,0.25f)},
        {-160.f, 100.f,40.f, 75.f,40.f,glm::vec3(0.15f,0.20f,0.22f)},
        {-215.f, 100.f,30.f, 60.f,30.f,glm::vec3(0.22f,0.18f,0.18f)},
        {-160.f, 160.f,50.f,140.f,50.f,glm::vec3(0.18f,0.22f,0.28f)},
        {-215.f, 160.f,35.f, 95.f,35.f,glm::vec3(0.28f,0.25f,0.30f)},
        { 160.f,-200.f,45.f,110.f,45.f,glm::vec3(0.20f,0.18f,0.28f)},
        { 210.f,-200.f,40.f, 70.f,40.f,glm::vec3(0.15f,0.20f,0.22f)},
        { 160.f,-140.f,50.f,180.f,50.f,glm::vec3(0.18f,0.15f,0.22f)},
        { 210.f,-140.f,35.f,100.f,35.f,glm::vec3(0.25f,0.22f,0.28f)},
        { 160.f, -70.f,45.f, 90.f,45.f,glm::vec3(0.20f,0.25f,0.30f)},
        { 215.f, -70.f,30.f, 60.f,30.f,glm::vec3(0.15f,0.18f,0.20f)},
        { 160.f,  10.f,45.f,220.f,45.f,glm::vec3(0.22f,0.20f,0.30f)},
        { 215.f,  10.f,30.f,140.f,30.f,glm::vec3(0.18f,0.22f,0.25f)},
        { 160.f, 100.f,50.f, 85.f,50.f,glm::vec3(0.25f,0.20f,0.22f)},
        { 215.f, 100.f,30.f, 55.f,30.f,glm::vec3(0.18f,0.25f,0.28f)},
        { 160.f, 160.f,45.f,150.f,45.f,glm::vec3(0.20f,0.18f,0.25f)},
        { 215.f, 160.f,35.f, 80.f,35.f,glm::vec3(0.22f,0.20f,0.18f)},
        {  -80.f,-260.f,55.f,130.f,50.f,glm::vec3(0.15f,0.18f,0.25f)},
        {    0.f,-260.f,50.f,100.f,45.f,glm::vec3(0.20f,0.22f,0.28f)},
        {   80.f,-260.f,55.f,170.f,50.f,glm::vec3(0.18f,0.15f,0.22f)},
        {  -80.f, 240.f,50.f,110.f,45.f,glm::vec3(0.22f,0.20f,0.30f)},
        {    0.f, 240.f,55.f, 90.f,50.f,glm::vec3(0.25f,0.22f,0.28f)},
        {   80.f, 240.f,50.f,140.f,45.f,glm::vec3(0.18f,0.25f,0.22f)},
    };
    for(int i=0;i<(int)g_buildings.size() && i<(int)defs.size();i++){
        auto& d = defs[i];
        glm::mat4 w = glm::translate(I, glm::vec3(d.x, 0.f, d.z));
        SetCityUniforms(g_shCity, w,
            glm::vec3(0.03f), d.col, glm::vec3(0.3f,0.3f,0.4f),
            20.f, 0, 0, glm::vec3(0), 0.f, 0);
        DrawMesh(g_buildings[i], g_shCity);
    }

    // --- Lamp posts ---
    {
        float lampZ[] = {-200,-140,-80,-20,40,100,160,200};
        float sides[] = {-75.f, 75.f};
        for(float sx : sides){
            for(float sz : lampZ){
                glm::mat4 wp = glm::translate(I, glm::vec3(sx,0.f,sz));
                SetCityUniforms(g_shCity, wp,
                    glm::vec3(0.05f), glm::vec3(0.4f,0.4f,0.45f), glm::vec3(0.2f),
                    8.f, 0, 0, glm::vec3(0), 0.f, 0);
                DrawMesh(g_streetLampPost, g_shCity);

                // Lamp head (emissive if street lights on)
                glm::mat4 wh = glm::translate(I, glm::vec3(sx,20.5f,sz));
                glm::vec3 emCol = glm::vec3(1.0f,0.9f,0.65f);
                float emStr = g_streetLightsOn ? 2.5f : 0.0f;
                SetCityUniforms(g_shCity, wh,
                    glm::vec3(0.5f), glm::vec3(1.f,0.9f,0.7f), glm::vec3(0.5f),
                    30.f, 0, 1, emCol, emStr, 0);
                DrawMesh(g_lampHead, g_shCity);

                // Light shaft
                if(g_streetLightsOn){
                    glm::mat4 ws = glm::translate(I, glm::vec3(sx,20.f,sz));
                    DrawShaft(g_lightShaft, ws, glm::vec3(1.f,0.85f,0.5f), g_shaftAlpha);
                }
            }
        }
    }

    // --- Traffic lights ---
    {
        glm::vec3 tc = (g_trafficState==0) ? glm::vec3(1,0,0) :
                       (g_trafficState==1) ? glm::vec3(1,0.6f,0) :
                                             glm::vec3(0,1,0);
        float px[] = {-78, 78, -78, 78};
        float pz[] = { -5, -5,   5,  5};
        for(int i=0;i<4;i++){
            glm::mat4 wt = glm::translate(I, glm::vec3(px[i],0.f,pz[i]));
            SetCityUniforms(g_shCity, wt,
                glm::vec3(0.02f), glm::vec3(0.15f,0.15f,0.15f), glm::vec3(0.1f),
                4.f, 0, 0, glm::vec3(0), 0.f, 0);
            DrawMesh(g_trafficLight, g_shCity);

            // Colored light glow (emissive box on top)
            glm::mat4 wg = glm::translate(I, glm::vec3(px[i],6.5f,pz[i]));
            float fl = 0.8f + 0.2f*sinf(g_time*3.f);
            SetCityUniforms(g_shCity, wg,
                glm::vec3(0.1f), tc*0.5f, glm::vec3(0.2f),
                4.f, 0, 1, tc, fl*2.5f, 0);
            DrawMesh(g_lampHead, g_shCity);
        }
    }

    // --- Neon signs (emissive panels on building facades) ---
    if(g_neonOn){
        int idx=0;
        for(auto& nl : g_neonLights){
            float flicker = 1.0f;
            if(g_neonFlicker==1) flicker = 0.85f + 0.15f*sinf(g_time*3.0f + nl.pos.x*0.1f);
            if(g_neonFlicker==2) flicker = (sinf(g_time*15.f + nl.pos.z*0.2f) > 0.3f) ? 1.f : 0.4f;

            glm::mat4 wn = glm::translate(I, nl.pos);
            // Face the road
            if(nl.pos.x < 0)
                wn = glm::rotate(wn, glm::radians(90.f), glm::vec3(0,1,0));
            else
                wn = glm::rotate(wn, glm::radians(-90.f), glm::vec3(0,1,0));
            SetCityUniforms(g_shCity, wn,
                glm::vec3(0.1f), nl.color, glm::vec3(0.3f),
                8.f, 0, 1, nl.color, flicker*3.f, 0);
            DrawMesh(g_neonSign, g_shCity);
            idx++;
        }
    }

    // --- Vehicles ---
    for(auto& v : g_vehicles){
        // Build orientation from direction
        float angle = atan2f(v.dir.x, v.dir.z);
        glm::mat4 wv = glm::translate(I, v.pos + glm::vec3(0,1.25f,0));
        wv = glm::rotate(wv, angle, glm::vec3(0,1,0));

        // Body
        glm::vec3 headlightPosWorld = v.pos + v.dir * 8.f + glm::vec3(0, 2.f, 0);
        int hlOn = (g_headlightsOn && g_streetLightsOn) ? 1 : (g_headlightsOn ? 1 : 0);

        SetCityUniforms(g_shCity, wv,
            glm::vec3(0.05f), v.bodyColor, glm::vec3(0.5f,0.5f,0.6f),
            30.f, 0, 0, glm::vec3(0), 0.f, 0);
        // Pass headlight as spotlight
        if(g_headlightsOn){
            U1i(g_shCity,"hasHeadlight", 1);
            U3f(g_shCity,"headlightPos", headlightPosWorld);
            U3f(g_shCity,"headlightDir", v.dir);
            U1f(g_shCity,"headlightCone", g_headlightCone);
            U3f(g_shCity,"headlightColor", v.hlColor);
            U1f(g_shCity,"headlightIntensity", 2.5f * g_globalLightInt);
        } else {
            U1i(g_shCity,"hasHeadlight", 0);
        }
        DrawMesh(g_carBody, g_shCity);

        // Car roof
        glm::mat4 wr = glm::translate(wv, glm::vec3(0,2.5f,0.5f));
        SetCityUniforms(g_shCity, wr,
            glm::vec3(0.05f), v.bodyColor * 0.7f, glm::vec3(0.3f),
            15.f, 0, 0, glm::vec3(0), 0.f, 0);
        U1i(g_shCity,"hasHeadlight",0);
        DrawMesh(g_carTop, g_shCity);

        // Headlights (emissive box at front)
        if(g_headlightsOn){
            glm::mat4 whl = glm::translate(wv, glm::vec3(3.f,0.5f,-7.5f));
            SetCityUniforms(g_shCity, whl,
                glm::vec3(0.5f), glm::vec3(1.f,0.95f,0.85f), glm::vec3(0.5f),
                10.f, 0, 1, glm::vec3(1.f,0.95f,0.85f), 3.f, 0);
            DrawMesh(g_lampHead, g_shCity);
            glm::mat4 whr = glm::translate(wv, glm::vec3(-3.f,0.5f,-7.5f));
            SetCityUniforms(g_shCity, whr,
                glm::vec3(0.5f), glm::vec3(1.f,0.95f,0.85f), glm::vec3(0.5f),
                10.f, 0, 1, glm::vec3(1.f,0.95f,0.85f), 3.f, 0);
            DrawMesh(g_lampHead, g_shCity);

            // Vehicle light shafts (headlight cones)
            glm::mat4 wsh = glm::translate(I, headlightPosWorld);
            // Rotate shaft to face vehicle direction
            float ha = atan2f(v.dir.x, v.dir.z);
            wsh = glm::rotate(wsh, ha + glm::pi<float>(), glm::vec3(0,1,0));
            wsh = glm::rotate(wsh, glm::radians(-80.f), glm::vec3(1,0,0));
            DrawShaft(g_lightShaft, wsh, glm::vec3(1.0f,0.9f,0.7f), g_shaftAlpha*0.8f);
        }
    }

    // --- Rain ---
    DrawRain();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Camera presets
// ─────────────────────────────────────────────────────────────────────────────
static void ApplyViewPreset(int idx){
    switch(idx % 3){
    case 0: // Overview from above
        g_cam->SetCamera(glm::vec3(0,300,200), glm::vec3(0,0,0), glm::vec3(0,1,0));
        break;
    case 1: // Street level, walking the main road
        g_cam->SetCamera(glm::vec3(0,8,-150), glm::vec3(0,8,0), glm::vec3(0,1,0));
        break;
    case 2: // Close-up on neon district
        g_cam->SetCamera(glm::vec3(-60,25,-80), glm::vec3(-130,20,-80), glm::vec3(0,1,0));
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  GLUT callbacks
// ─────────────────────────────────────────────────────────────────────────────
static int   g_lastTime = 0;

void Display(){
    glClearColor(g_fogColor.r, g_fogColor.g, g_fogColor.b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    DrawScene();

    glutSwapBuffers();
}

void Reshape(int w, int h){
    glViewport(0,0,w,h);
    if(g_cam) g_cam->SetPerspectiveView(60.f,(float)w/h,0.5f,2000.f);
}

void Idle(){
    int t = glutGet(GLUT_ELAPSED_TIME);
    float dt = (t - g_lastTime) * 0.001f;
    g_lastTime = t;
    dt = std::min(dt, 0.05f); // clamp huge gaps
    Update(dt);
    glutPostRedisplay();
}

static glm::vec3 g_fogColors[] = {
    glm::vec3(0.03f,0.04f,0.08f),  // dark blue (default night)
    glm::vec3(0.06f,0.07f,0.10f),  // grey-blue
    glm::vec3(0.02f,0.06f,0.04f),  // greenish
};

void Keyboard(unsigned char key, int, int){
    switch(key){
    // Quit
    case 'z': case 'Z': case 27: exit(0); break;

    // Camera movement
    case 'w': g_cam->MoveForward (5.f); break;
    case 's': g_cam->MoveBackward(5.f); break;
    case 'a': g_cam->Roll( 2.f); break;
    case 'd': g_cam->Roll(-2.f); break;
    case 'q': g_cam->MoveUp  (5.f); break;
    case 'e': g_cam->MoveDown(5.f); break;

    // Reset camera
    case 'r': ApplyViewPreset(0); g_viewPreset=0; break;

    // Fog
    case 'F': g_fogDensity = std::min(3.0f, g_fogDensity+0.1f);
              std::cout<<"[Fog density: "<<g_fogDensity<<"]\n"; break;
    case 'f': g_fogDensity = std::max(0.0f, g_fogDensity-0.1f);
              std::cout<<"[Fog density: "<<g_fogDensity<<"]\n"; break;
    case 'G': case 'g':
              g_fogColorIdx=(g_fogColorIdx+1)%3;
              g_fogColor=g_fogColors[g_fogColorIdx];
              std::cout<<"[Fog color cycle: "<<g_fogColorIdx<<"]\n"; break;

    // Viewpoint preset
    case 'V': case 'v':
              g_viewPreset=(g_viewPreset+1)%3;
              ApplyViewPreset(g_viewPreset);
              std::cout<<"[Viewpoint preset "<<g_viewPreset<<"]\n"; break;

    // Light toggles
    case '1': g_streetLightsOn = !g_streetLightsOn;
              std::cout<<"[Street lights "<<(g_streetLightsOn?"ON":"OFF")<<"]\n"; break;
    case '2': g_neonOn = !g_neonOn;
              std::cout<<"[Neon signs "<<(g_neonOn?"ON":"OFF")<<"]\n"; break;
    case '3': g_trafficState=(g_trafficState+1)%3;
              std::cout<<"[Traffic light: "<<g_trafficState<<"]\n"; break;
    case '4': g_headlightsOn = !g_headlightsOn;
              std::cout<<"[Headlights "<<(g_headlightsOn?"ON":"OFF")<<"]\n"; break;

    // Global light intensity
    case 'L': g_globalLightInt = std::min(3.0f, g_globalLightInt+0.2f);
              std::cout<<"[Light intensity: "<<g_globalLightInt<<"]\n"; break;
    case 'l': g_globalLightInt = std::max(0.0f, g_globalLightInt-0.2f);
              std::cout<<"[Light intensity: "<<g_globalLightInt<<"]\n"; break;

    // Pause
    case 'P': case 'p':
              g_paused = !g_paused;
              std::cout<<"[Animation "<<(g_paused?"PAUSED":"RESUMED")<<"]\n"; break;

    // Vehicle speed
    case '+': case '=': g_vehicleSpeed = std::min(5.f, g_vehicleSpeed*1.5f);
              std::cout<<"[Vehicle speed: "<<g_vehicleSpeed<<"]\n"; break;
    case '-': case '_': g_vehicleSpeed = std::max(0.1f, g_vehicleSpeed/1.5f);
              std::cout<<"[Vehicle speed: "<<g_vehicleSpeed<<"]\n"; break;

    // Rain
    case 'T': case 't':
              g_rainOn = !g_rainOn;
              std::cout<<"[Rain "<<(g_rainOn?"ON":"OFF")<<"]\n"; break;

    // Neon flicker cycle
    case 'N': case 'n':
              g_neonFlicker=(g_neonFlicker+1)%3;
              std::cout<<"[Neon flicker mode: "<<g_neonFlicker<<"]\n"; break;

    // Headlight cone angle
    case 'H': g_headlightCone = std::min(60.f, g_headlightCone+5.f);
              std::cout<<"[Headlight cone: "<<g_headlightCone<<"]\n"; break;
    case 'h': g_headlightCone = std::max(5.f, g_headlightCone-5.f);
              std::cout<<"[Headlight cone: "<<g_headlightCone<<"]\n"; break;

    // Shaft alpha
    case 'X': g_shaftAlpha = std::min(0.6f, g_shaftAlpha+0.05f);
              std::cout<<"[Shaft alpha: "<<g_shaftAlpha<<"]\n"; break;
    case 'x': g_shaftAlpha = std::max(0.0f, g_shaftAlpha-0.05f);
              std::cout<<"[Shaft alpha: "<<g_shaftAlpha<<"]\n"; break;
    }
    glutPostRedisplay();
}

void SpecialKey(int key, int, int){
    switch(key){
    case GLUT_KEY_UP:    g_cam->Pitch( 2.f); break;
    case GLUT_KEY_DOWN:  g_cam->Pitch(-2.f); break;
    case GLUT_KEY_LEFT:  g_cam->Yaw(  -2.f); break;
    case GLUT_KEY_RIGHT: g_cam->Yaw(   2.f); break;
    }
    glutPostRedisplay();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char** argv){
    try{
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
        glutInitWindowSize(WIN_W, WIN_H);
        glutInitWindowPosition(80, 80);
        glutCreateWindow(WIN_TITLE);

        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if(err != GLEW_OK)
            throw std::runtime_error(std::string("GLEW: ")+(char*)glewGetErrorString(err));

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);

        std::cout << "Renderer : " << glGetString(GL_RENDERER) << "\n"
                  << "OpenGL   : " << glGetString(GL_VERSION)  << "\n"
                  << "GLSL     : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n\n";

        // Camera – default overview
        g_cam = new Camera();
        g_cam->SetCamera(glm::vec3(0,300,200), glm::vec3(0,0,0), glm::vec3(0,1,0));
        g_cam->SetPerspectiveView(60.f,(float)WIN_W/WIN_H,0.5f,2000.f);

        // Load shaders
        g_shCity     = LoadShaders("shaders/cityShader");
        g_shShaft    = LoadShaders("shaders/shaftShader");
        g_shParticle = LoadShaders("shaders/particleShader");

        // Build scene
        InitScene();

        g_lastTime = glutGet(GLUT_ELAPSED_TIME);

        std::cout
            << "╔══════════════════════════════════════════════╗\n"
            << "║   COSC 3307 – 3D Night Life                  ║\n"
            << "║   Devanshi Raulji | Vruddhi Shah | Dhara Rokad║\n"
            << "╠══════════════════════════════════════════════╣\n"
            << "║  Arrow UP/DOWN/LEFT/RIGHT  Pitch / Yaw       ║\n"
            << "║  W / S                     Forward / Back    ║\n"
            << "║  A / D                     Roll              ║\n"
            << "║  Q / E                     Up / Down         ║\n"
            << "║  R                         Reset camera      ║\n"
            << "║  V                         Cycle viewpoints  ║\n"
            << "╠══════════════════════════════════════════════╣\n"
            << "║  F / f    Fog density  +/-                   ║\n"
            << "║  G        Fog colour cycle                   ║\n"
            << "║  1        Toggle street lights               ║\n"
            << "║  2        Toggle neon signs                  ║\n"
            << "║  3        Cycle traffic light colour         ║\n"
            << "║  4        Toggle headlights                  ║\n"
            << "║  L / l    Global light intensity +/-         ║\n"
            << "╠══════════════════════════════════════════════╣\n"
            << "║  P        Pause / resume vehicles            ║\n"
            << "║  + / -    Vehicle speed x1.5 / ÷1.5         ║\n"
            << "║  T        Toggle rain                        ║\n"
            << "║  N        Cycle neon flicker mode            ║\n"
            << "║  H / h    Headlight cone +5° / -5°          ║\n"
            << "║  X / x    Light shaft alpha +/-              ║\n"
            << "║  ESC / Z  Quit                               ║\n"
            << "╚══════════════════════════════════════════════╝\n\n";

        glutDisplayFunc(Display);
        glutReshapeFunc(Reshape);
        glutKeyboardFunc(Keyboard);
        glutSpecialFunc(SpecialKey);
        glutIdleFunc(Idle);
        glutMainLoop();

    } catch(std::exception& e){
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
