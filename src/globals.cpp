/*
=============================================================================
COSC 3307 - 3D Night Life
Global variable definitions
=============================================================================
*/
#include <globals.h>

Camera* g_cam = nullptr;

GLuint g_shCity     = 0;
GLuint g_shShaft    = 0;
GLuint g_shParticle = 0;
GLuint g_shShadow   = 0;

// Shadow mapping
GLuint     g_shadowFBO        = 0;
GLuint     g_shadowMap        = 0;
glm::mat4  g_lightSpaceMatrix = glm::mat4(1.f);
bool       g_shadowOn         = true;

GLuint g_roadTexture = 0;  // T8 road texture

Mesh* g_road           = nullptr;
Mesh* g_sidewalk       = nullptr;
std::vector<Mesh*> g_buildings;
Mesh* g_streetLampPost = nullptr;
Mesh* g_lampHead       = nullptr;
Mesh* g_lightShaft     = nullptr;
Mesh* g_trafficLight   = nullptr;
Mesh* g_neonSign       = nullptr;
Mesh* g_carBody        = nullptr;
Mesh* g_carTop         = nullptr;
Mesh* g_carWheel       = nullptr;

GLuint g_rainVAO = 0;
GLuint g_rainVBO = 0;
std::vector<RainDrop> g_rainDrops;

std::vector<Light>   g_streetLights;
std::vector<Light>   g_neonLights;
std::vector<Light>   g_trafficLights;
std::vector<Vehicle> g_vehicles;

float     g_fogDensity  = 0.6f;
glm::vec3 g_fogColor    = glm::vec3(0.03f, 0.04f, 0.08f);
int       g_fogColorIdx = 0;
glm::vec3 g_fogColors[3] = {
    glm::vec3(0.03f, 0.04f, 0.08f),  // dark blue (default night)
    glm::vec3(0.06f, 0.07f, 0.10f),  // grey-blue
    glm::vec3(0.02f, 0.06f, 0.04f),  // greenish
};

bool  g_streetLightsOn = true;
bool  g_neonOn         = true;
bool  g_headlightsOn   = true;
bool  g_rainOn         = true;
bool  g_paused         = false;
float g_globalLightInt = 1.0f;
float g_vehicleSpeed   = 1.0f;
float g_headlightCone  = 18.0f;
float g_shaftAlpha     = 0.18f;
int   g_neonFlicker    = 1;
int   g_trafficState   = 0;
float g_trafficTimer   = 0.0f;
int   g_viewPreset     = 0;
float g_time           = 0.0f;

std::vector<Vertex>       g_vtmp;
std::vector<unsigned int> g_itmp;
