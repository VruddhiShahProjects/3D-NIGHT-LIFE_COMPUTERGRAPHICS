/*
=============================================================================
COSC 3307 - 3D Night Life
Team Members:
  1. Devanshi Raulji  - 3D city geometry, buildings, streets, texture mapping, vehicles
  2. Vruddhi Shah     - Volumetric fog, depth-based attenuation, light shafts, particles
  3. Dhara Rokad      - Dynamic lighting, vehicle animation, camera controls, UI controls

Build:  Visual C++ / CMake / Windows
Libs:   OpenGL, GLEW, freeGLUT, GLM
=============================================================================
*/

#include <iostream>
#include <stdexcept>
#include <algorithm>

#include <globals.h>
#include <renderer.h>
#include <scene.h>
#include <camera.h>

// ---------------------------------------------------------------------------
//  Camera preset helper
// ---------------------------------------------------------------------------
static void ApplyViewPreset(int idx) {
    switch (idx % 3) {
    case 0: // Overview from above
        g_cam->SetCamera(glm::vec3(0,300,200), glm::vec3(0,0,0), glm::vec3(0,1,0));
        break;
    case 1: // Street level
        g_cam->SetCamera(glm::vec3(0,8,-150), glm::vec3(0,8,0), glm::vec3(0,1,0));
        break;
    case 2: // Close-up neon district
        g_cam->SetCamera(glm::vec3(-60,25,-80), glm::vec3(-130,20,-80), glm::vec3(0,1,0));
        break;
    }
}

// ---------------------------------------------------------------------------
//  Cinematic Camera Path
//  Catmull-Rom spline through a set of handcrafted keyframes that showcase
//  the whole city: overview swoop → street-level → neon district → rooftop.
//  Toggle with  C  key.  Automatically loops.
// ---------------------------------------------------------------------------
struct CamKeyframe {
    glm::vec3 pos;
    glm::vec3 target;
};

static const CamKeyframe g_cinematicKeys[] = {
    // 0 — high overview, city centre
    { glm::vec3(  0.f, 320.f,  250.f), glm::vec3(  0.f,   0.f,   0.f) },
    // 1 — sweep down toward street
    { glm::vec3( 80.f,  80.f,  180.f), glm::vec3(  0.f,  10.f,   0.f) },
    // 2 — street level, looking down the road
    { glm::vec3(  0.f,   8.f, -140.f), glm::vec3(  0.f,   8.f,  60.f) },
    // 3 — neon district close-up
    { glm::vec3(-60.f,  25.f,  -80.f), glm::vec3(-130.f, 20.f, -80.f) },
    // 4 — low pass beside buildings
    { glm::vec3(-120.f, 15.f,   20.f), glm::vec3(-120.f, 15.f, 120.f) },
    // 5 — rooftop corner looking across
    { glm::vec3( 110.f, 90.f,  110.f), glm::vec3( -50.f, 20.f,  -50.f) },
    // 6 — back to overview (loop seam)
    { glm::vec3(  0.f, 320.f,  250.f), glm::vec3(  0.f,   0.f,   0.f) },
};
static const int  g_numCinKeys    = 7;
static const float g_segDuration  = 6.0f;   // seconds per segment
static bool  g_cinematicOn   = false;
static float g_cinematicTime = 0.f;

// Catmull-Rom interpolation for a single vec3
static glm::vec3 CatmullRom(glm::vec3 p0, glm::vec3 p1,
                             glm::vec3 p2, glm::vec3 p3, float t) {
    float t2 = t * t, t3 = t2 * t;
    return 0.5f * ((2.f * p1)
        + (-p0 + p2) * t
        + (2.f*p0 - 5.f*p1 + 4.f*p2 - p3) * t2
        + (-p0 + 3.f*p1 - 3.f*p2 + p3) * t3);
}

static void UpdateCinematicCamera(float dt) {
    if (!g_cinematicOn) return;

    g_cinematicTime += dt;
    float totalDur = g_segDuration * (g_numCinKeys - 1);
    // Loop
    while (g_cinematicTime >= totalDur) g_cinematicTime -= totalDur;

    int   seg = (int)(g_cinematicTime / g_segDuration);
    float t   = (g_cinematicTime - seg * g_segDuration) / g_segDuration;

    // Clamp indices with wrap for Catmull-Rom ghost points
    int i0 = std::max(0,              seg - 1);
    int i1 = seg;
    int i2 = std::min(g_numCinKeys-1, seg + 1);
    int i3 = std::min(g_numCinKeys-1, seg + 2);

    glm::vec3 pos = CatmullRom(g_cinematicKeys[i0].pos,
                                g_cinematicKeys[i1].pos,
                                g_cinematicKeys[i2].pos,
                                g_cinematicKeys[i3].pos, t);

    glm::vec3 tgt = CatmullRom(g_cinematicKeys[i0].target,
                                g_cinematicKeys[i1].target,
                                g_cinematicKeys[i2].target,
                                g_cinematicKeys[i3].target, t);

    g_cam->SetCamera(pos, tgt, glm::vec3(0.f, 1.f, 0.f));
}

// ---------------------------------------------------------------------------
//  GLUT callbacks
// ---------------------------------------------------------------------------
static int g_lastTime = 0;
static int g_winW = WIN_W;
static int g_winH = WIN_H;

void Display() {
    // ------------------------------------------------------------------
    //  1. Shadow pass — render scene depth from the dominant light's POV
    // ------------------------------------------------------------------
    if (g_shadowOn) {
        // Use the first street-light (or a fixed overhead direction if none active)
        glm::vec3 lightPos = glm::vec3(0.f, 220.f, 0.f);
        if (!g_streetLights.empty() && g_streetLightsOn)
            lightPos = g_streetLights[0].pos + glm::vec3(0.f, 200.f, 0.f);

        // Orthographic projection covering the whole city block
        glm::mat4 lightProj = glm::ortho(-320.f, 320.f, -320.f, 320.f, 1.f, 600.f);
        glm::mat4 lightView = glm::lookAt(lightPos,
                                          glm::vec3(0.f, 0.f, 0.f),
                                          glm::vec3(0.f, 0.f, -1.f));
        g_lightSpaceMatrix = lightProj * lightView;

        glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, g_shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Cull front faces to reduce peter-panning artefacts
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        DrawSceneDepth(g_lightSpaceMatrix);

        glCullFace(GL_BACK);
        glDisable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // ------------------------------------------------------------------
    //  2. Normal colour pass
    // ------------------------------------------------------------------
    glViewport(0, 0, g_winW, g_winH);
    glClearColor(g_fogColor.r, g_fogColor.g, g_fogColor.b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    DrawScene();
    glutSwapBuffers();
}

void Reshape(int w, int h) {
    g_winW = w; g_winH = h;
    glViewport(0, 0, w, h);
    if (g_cam) g_cam->SetPerspectiveView(60.f, (float)w/h, 0.5f, 2000.f);
}

void Idle() {
    int t = glutGet(GLUT_ELAPSED_TIME);
    float dt = (t - g_lastTime) * 0.001f;
    g_lastTime = t;
    dt = std::min(dt, 0.05f);
    UpdateCinematicCamera(dt);
    Update(dt);
    glutPostRedisplay();
}

// ---------------------------------------------------------------------------
//  Keyboard  — every case has its own break; terminal feedback on every action
// ---------------------------------------------------------------------------
void Keyboard(unsigned char key, int, int) {
    switch (key) {

    // Quit
    case 'z': case 'Z': case 27:
        std::cout << "[Quit]\n";
        exit(0);
        break;

    // Camera movement  (no terminal spam — runs every frame while held)
    case 'w': g_cam->MoveForward (5.f); break;
    case 's': g_cam->MoveBackward(5.f); break;
    case 'a': g_cam->Roll( 2.f);        break;
    case 'd': g_cam->Roll(-2.f);        break;
    case 'q': g_cam->MoveUp  (5.f);     break;
    case 'e': g_cam->MoveDown(5.f);     break;

    // Reset camera
    case 'r': case 'R':
        ApplyViewPreset(0);
        g_viewPreset = 0;
        std::cout << "[Camera reset to overview]\n";
        break;

    // Fog density
    case 'F':
        g_fogDensity = std::min(3.0f, g_fogDensity + 0.1f);
        std::cout << "[Fog density: " << g_fogDensity << " (+)]\n";
        break;
    case 'f':
        g_fogDensity = std::max(0.0f, g_fogDensity - 0.1f);
        std::cout << "[Fog density: " << g_fogDensity << " (-)]\n";
        break;

    // Fog colour cycle
    case 'G': case 'g': {
        g_fogColorIdx = (g_fogColorIdx + 1) % 3;
        g_fogColor    = g_fogColors[g_fogColorIdx];
        const char* fogNames[] = { "Dark blue (night)", "Grey-blue", "Greenish" };
        std::cout << "[Fog colour: " << fogNames[g_fogColorIdx] << "]\n";
        break;
    }

    // Viewpoint preset
    case 'V': case 'v': {
        g_viewPreset = (g_viewPreset + 1) % 3;
        ApplyViewPreset(g_viewPreset);
        const char* presetNames[] = { "Overview (above)", "Street level", "Neon district close-up" };
        std::cout << "[Viewpoint: " << presetNames[g_viewPreset] << "]\n";
        break;
    }

    // Toggle street lights
    case '1':
        g_streetLightsOn = !g_streetLightsOn;
        std::cout << "[Street lights: " << (g_streetLightsOn ? "ON" : "OFF") << "]\n";
        break;

    // Toggle neon signs
    case '2':
        g_neonOn = !g_neonOn;
        std::cout << "[Neon signs: " << (g_neonOn ? "ON" : "OFF") << "]\n";
        break;

    // Cycle traffic light colour
    case '3': {
        g_trafficState = (g_trafficState + 1) % 3;
        const char* trafficNames[] = { "RED", "AMBER", "GREEN" };
        std::cout << "[Traffic light: " << trafficNames[g_trafficState] << "]\n";
        break;
    }

    // Toggle headlights
    case '4':
        g_headlightsOn = !g_headlightsOn;
        std::cout << "[Vehicle headlights: " << (g_headlightsOn ? "ON" : "OFF") << "]\n";
        break;

    // Global light intensity
    case 'L':
        g_globalLightInt = std::min(3.0f, g_globalLightInt + 0.2f);
        std::cout << "[Global light intensity: " << g_globalLightInt << " (+)]\n";
        break;
    case 'l':
        g_globalLightInt = std::max(0.0f, g_globalLightInt - 0.2f);
        std::cout << "[Global light intensity: " << g_globalLightInt << " (-)]\n";
        break;

    // Pause / resume vehicles
    case 'P': case 'p':
        g_paused = !g_paused;
        std::cout << "[Vehicles: " << (g_paused ? "PAUSED" : "RESUMED") << "]\n";
        break;

    // Vehicle speed
    case '+': case '=':
        g_vehicleSpeed = std::min(5.f, g_vehicleSpeed * 1.5f);
        std::cout << "[Vehicle speed: " << g_vehicleSpeed << " (faster)]\n";
        break;
    case '-': case '_':
        g_vehicleSpeed = std::max(0.1f, g_vehicleSpeed / 1.5f);
        std::cout << "[Vehicle speed: " << g_vehicleSpeed << " (slower)]\n";
        break;

    // Toggle rain
    case 'T': case 't':
        g_rainOn = !g_rainOn;
        std::cout << "[Rain: " << (g_rainOn ? "ON" : "OFF") << "]\n";
        break;

    // Cycle neon flicker mode  (0=steady, 1=slow flicker, 2=fast flicker)
    case 'N': case 'n': {
        g_neonFlicker = (g_neonFlicker + 1) % 3;
        const char* flickerNames[] = { "Steady (no flicker)", "Slow flicker", "Fast flicker" };
        std::cout << "[Neon flicker mode: " << flickerNames[g_neonFlicker] << "]\n";
        break;
    }

    // Headlight cone angle
    case 'H':
        g_headlightCone = std::min(60.f, g_headlightCone + 5.f);
        std::cout << "[Headlight cone angle: " << g_headlightCone << " deg (+)]\n";
        break;
    case 'h':
        g_headlightCone = std::max(5.f, g_headlightCone - 5.f);
        std::cout << "[Headlight cone angle: " << g_headlightCone << " deg (-)]\n";
        break;

    // Toggle shadow mapping
    case 'M': case 'm':
        g_shadowOn = !g_shadowOn;
        std::cout << "[Shadow mapping: " << (g_shadowOn ? "ON" : "OFF") << "]\n";
        break;

    //  Cinematic camera path
    case 'C': case 'c':
        g_cinematicOn = !g_cinematicOn;
        if (g_cinematicOn) {
            g_cinematicTime = 0.f;
            std::cout << "[Cinematic camera: ON  — press C again to resume manual control]\n";
        } else {
            std::cout << "[Cinematic camera: OFF]\n";
        }
        break;

    // Light shaft alpha
    case 'X':
        g_shaftAlpha = std::min(0.6f, g_shaftAlpha + 0.05f);
        std::cout << "[Light shaft alpha: " << g_shaftAlpha << " (+)]\n";
        break;
    case 'x':
        g_shaftAlpha = std::max(0.0f, g_shaftAlpha - 0.05f);
        std::cout << "[Light shaft alpha: " << g_shaftAlpha << " (-)]\n";
        break;
    }

    glutPostRedisplay();
}

void SpecialKey(int key, int, int) {
    switch (key) {
    case GLUT_KEY_UP:    g_cam->Pitch( 2.f); std::cout << "[Camera pitch up]\n";    break;
    case GLUT_KEY_DOWN:  g_cam->Pitch(-2.f); std::cout << "[Camera pitch down]\n";  break;
    case GLUT_KEY_LEFT:  g_cam->Yaw(  -2.f); std::cout << "[Camera yaw left]\n";    break;
    case GLUT_KEY_RIGHT: g_cam->Yaw(   2.f); std::cout << "[Camera yaw right]\n";   break;
    }
    glutPostRedisplay();
}

// ---------------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    try {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
        glutInitWindowSize(WIN_W, WIN_H);
        glutInitWindowPosition(80, 80);
        glutCreateWindow(WIN_TITLE);

        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK)
            throw std::runtime_error(std::string("GLEW: ") + (char*)glewGetErrorString(err));

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);

        // Camera - default overview
        g_cam = new Camera();
        g_cam->SetCamera(glm::vec3(0,300,200), glm::vec3(0,0,0), glm::vec3(0,1,0));
        g_cam->SetPerspectiveView(60.f, (float)WIN_W/WIN_H, 0.5f, 2000.f);

        // Load shaders
        g_shCity     = LoadShaders("shaders/cityShader");
        g_shShaft    = LoadShaders("shaders/shaftShader");
        g_shParticle = LoadShaders("shaders/particleShader");
        g_shShadow   = LoadShaders("shaders/shadowShader");

        // Build scene
        InitScene();
        InitShadowMap();

        g_lastTime = glutGet(GLUT_ELAPSED_TIME);

        // Print controls
        std::cout << "\nCOSC 3307 - 3D Night Life\n"
                  << "Devanshi Raulji | Vruddhi Shah | Dhara Rokad\n\n"
                  << "Controls:\n"
                  << "  Arrow UP/DOWN/LEFT/RIGHT  Pitch / Yaw\n"
                  << "  W / S                     Forward / Back\n"
                  << "  A / D                     Roll\n"
                  << "  Q / E                     Up / Down\n"
                  << "  R                         Reset camera\n"
                  << "  V                         Cycle viewpoints\n\n"
                  << "  F / f    Fog density +/-\n"
                  << "  G        Fog colour cycle\n"
                  << "  1        Toggle street lights\n"
                  << "  2        Toggle neon signs\n"
                  << "  3        Cycle traffic light colour\n"
                  << "  4        Toggle headlights\n"
                  << "  L / l    Global light intensity +/-\n\n"
                  << "  P        Pause / resume vehicles\n"
                  << "  + / -    Vehicle speed x1.5 / /1.5\n"
                  << "  T        Toggle rain\n"
                  << "  N        Cycle neon flicker mode\n"
                  << "  H / h    Headlight cone +5 / -5\n"
                  << "  X / x    Light shaft alpha +/-\n"
                  << "  M        Toggle shadow mapping\n"
                  << "  ESC / Z  Quit\n\n";

        glutDisplayFunc(Display);
        glutReshapeFunc(Reshape);
        glutKeyboardFunc(Keyboard);
        glutSpecialFunc(SpecialKey);
        glutIdleFunc(Idle);
        glutMainLoop();

    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
