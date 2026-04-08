#pragma once
/*
=============================================================================
COSC 3307 - 3D Night Life
Shared globals, structs, and extern declarations
=============================================================================
*/

#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <camera.h>

// ---------------------------------------------------------------------------
//  Vertex layout
// ---------------------------------------------------------------------------
struct Vertex {
    glm::vec3 pos;       // offset  0
    glm::vec3 color;     // offset 12
    glm::vec3 normal;    // offset 24
    glm::vec2 texCoord;  // offset 36
};                       // stride 44

struct Mesh {
    GLuint vao, vbo, ibo;
    GLuint indexCount;
};

// ---------------------------------------------------------------------------
//  Light
// ---------------------------------------------------------------------------
struct Light {
    glm::vec3 pos;
    glm::vec3 color;
    float     intensity;
    float     radius;
    bool      enabled;
};

// ---------------------------------------------------------------------------
//  Vehicle
// ---------------------------------------------------------------------------
struct Vehicle {
    glm::vec3 pos;
    glm::vec3 dir;
    float     speed;
    float     t;
    glm::vec3 bodyColor;
    float     routeLen;
    glm::vec3 hlOffset;
    glm::vec3 hlColor;
    float     hlCone;
};

// ---------------------------------------------------------------------------
//  Rain particle
// ---------------------------------------------------------------------------
struct RainDrop { glm::vec3 pos; glm::vec3 color; float speed; };

// ---------------------------------------------------------------------------
//  Extern globals (defined in globals.cpp)
// ---------------------------------------------------------------------------
extern Camera* g_cam;

// Shaders
extern GLuint g_shCity;
extern GLuint g_shShaft;
extern GLuint g_shParticle;
extern GLuint g_shShadow;       // depth-only shadow pass shader

// Shadow mapping
extern GLuint     g_shadowFBO;
extern GLuint     g_shadowMap;
extern glm::mat4  g_lightSpaceMatrix;
extern bool       g_shadowOn;
static const int  SHADOW_MAP_SIZE = 2048;

// Textures (T8)
extern GLuint g_roadTexture;

// Meshes
extern Mesh* g_road;
extern Mesh* g_sidewalk;
extern std::vector<Mesh*> g_buildings;
extern Mesh* g_streetLampPost;
extern Mesh* g_lampHead;
extern Mesh* g_lightShaft;
extern Mesh* g_trafficLight;
extern Mesh* g_neonSign;
extern Mesh* g_carBody;
extern Mesh* g_carTop;
extern Mesh* g_carWheel;

// Rain
extern GLuint g_rainVAO;
extern GLuint g_rainVBO;
extern std::vector<RainDrop> g_rainDrops;
static const int RAIN_COUNT = 3000;

// Lights
extern std::vector<Light> g_streetLights;
extern std::vector<Light> g_neonLights;
extern std::vector<Light> g_trafficLights;

// Vehicles
extern std::vector<Vehicle> g_vehicles;

// Fog
extern float     g_fogDensity;
extern glm::vec3 g_fogColor;
extern int       g_fogColorIdx;
extern glm::vec3 g_fogColors[3];

// Toggles / state
extern bool  g_streetLightsOn;
extern bool  g_neonOn;
extern bool  g_headlightsOn;
extern bool  g_rainOn;
extern bool  g_paused;
extern float g_globalLightInt;
extern float g_vehicleSpeed;
extern float g_headlightCone;
extern float g_shaftAlpha;
extern int   g_neonFlicker;
extern int   g_trafficState;
extern float g_trafficTimer;
extern int   g_viewPreset;
extern float g_time;

// Temp mesh-build buffers
extern std::vector<Vertex>       g_vtmp;
extern std::vector<unsigned int> g_itmp;

// Window
static const int   WIN_W     = 1024;
static const int   WIN_H     = 768;
static const char* WIN_TITLE = "COSC 3307 - 3D Night Life  |  W/S/A/D/Arrows-camera  |  F/f fog  |  P pause  |  T rain  |  C cinematic  |  ESC quit";
