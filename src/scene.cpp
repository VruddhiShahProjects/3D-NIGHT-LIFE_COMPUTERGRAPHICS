/*
=============================================================================
COSC 3307 - 3D Night Life
Scene implementation
=============================================================================
*/
#include <scene.h>
#include <renderer.h>
#include <cstdlib>
#include <cmath>
#include <algorithm>

// T8 - single-header image loader
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

std::vector<BuildingDef> g_buildingDefs = {
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

// ---------------------------------------------------------------------------
//  Geometry creation
// ---------------------------------------------------------------------------
void CreateCityGeometry() {
    g_road     = MakeFlat(600, 600, glm::vec3(0.08f,0.08f,0.10f), 20.f, 20.f);
    g_sidewalk = MakeFlat(600,  20, glm::vec3(0.25f,0.25f,0.28f), 10.f, 1.f);
    for (auto& d : g_buildingDefs)
        g_buildings.push_back(MakeBox(d.w, d.h, d.d, d.col));
}

void CreateStreetLights() {
    float lampZ[] = {-200,-140,-80,-20,40,100,160,200};
    float sides[] = {-75.f, 75.f};
    for (float sx : sides) {
        for (float sz : lampZ) {
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

void CreateNeonLights() {
    struct NL { glm::vec3 pos; glm::vec3 col; };
    std::vector<NL> nl = {
        {{-137.f,25.f,-185.f}, {1,0.1f,0.8f}},
        {{-137.f,40.f,-130.f}, {0,0.8f,1.f}},
        {{-137.f,18.f, -55.f}, {1,0.6f,0.f}},
        {{-137.f,35.f,  30.f}, {0.4f,1.f,0.4f}},
        {{-137.f,22.f, 110.f}, {1,1.f,0.2f}},
        {{-137.f,50.f, 175.f}, {0.6f,0.2f,1.f}},
        {{ 137.f,28.f,-190.f}, {0,0.8f,1.f}},
        {{ 137.f,45.f,-130.f}, {1,0.1f,0.8f}},
        {{ 137.f,20.f, -60.f}, {0.4f,1.f,0.4f}},
        {{ 137.f,38.f,  25.f}, {1,0.6f,0.f}},
        {{ 137.f,15.f, 115.f}, {1,1.f,0.2f}},
        {{ 137.f,55.f, 170.f}, {0.8f,0.2f,1.f}},
    };
    for (auto& n : nl) {
        Light l;
        l.pos = n.pos; l.color = n.col; l.intensity = 1.8f; l.radius = 60.f; l.enabled = true;
        g_neonLights.push_back(l);
    }
}

void CreateTrafficLights() {
    glm::vec3 pos[] = { {-78,8,-5}, {78,8,-5}, {-78,8,5}, {78,8,5} };
    for (auto& p : pos) {
        Light l;
        l.pos = p; l.color = glm::vec3(1,0,0); l.intensity = 2.0f; l.radius = 50.f; l.enabled = true;
        g_trafficLights.push_back(l);
    }
}

void CreateVehicles() {
    struct VDef { glm::vec3 pos; glm::vec3 dir; glm::vec3 col; float speed; float t; };
    // Lane length ~520 (z:-260 to +260). 3 cars per lane spaced ~173 units apart
    // so no two cars start within merging distance of each other.
    std::vector<VDef> vd = {
        // Lane A: x=-30, travelling +Z
        {{-30.f,0.f,-240.f},{0.f,0.f, 1.f},{0.8f,0.2f,0.2f},90.f,0.00f},
        {{-30.f,0.f, -67.f},{0.f,0.f, 1.f},{0.2f,0.4f,0.8f},70.f,0.33f},
        {{-30.f,0.f, 107.f},{0.f,0.f, 1.f},{0.8f,0.7f,0.2f},80.f,0.67f},
        // Lane B: x=+30, travelling -Z
        {{ 30.f,0.f, 240.f},{0.f,0.f,-1.f},{0.3f,0.7f,0.3f},75.f,0.00f},
        {{ 30.f,0.f,  67.f},{0.f,0.f,-1.f},{0.8f,0.4f,0.1f},85.f,0.33f},
        {{ 30.f,0.f,-107.f},{0.f,0.f,-1.f},{0.5f,0.2f,0.7f},65.f,0.67f},
    };
    for (auto& v : vd) {
        Vehicle vh;
        vh.pos = v.pos; vh.dir = v.dir; vh.speed = v.speed; vh.t = v.t;
        vh.bodyColor = v.col; vh.routeLen = 500.f;
        vh.hlColor = glm::vec3(1,0.95f,0.85f);
        vh.hlCone  = g_headlightCone;
        g_vehicles.push_back(vh);
    }
}

void CreateCarMeshes() {
    g_carBody  = MakeBox(8.f, 2.5f, 16.f, glm::vec3(1,1,1));
    g_carTop   = MakeBox(6.f, 2.0f, 10.f, glm::vec3(0.7f,0.7f,0.7f));
    g_carWheel = MakeCylinder(1.5f, 1.f, 12, glm::vec3(0.15f,0.15f,0.15f));
}

// ---------------------------------------------------------------------------
//  Rain
// ---------------------------------------------------------------------------
void InitRain() {
    g_rainDrops.resize(RAIN_COUNT);
    for (auto& d : g_rainDrops) {
        d.pos   = glm::vec3((rand()%600)-300.f, (rand()%80)+5.f, (rand()%600)-300.f);
        d.color = glm::vec3(0.6f,0.7f,0.9f);
        d.speed = 50.f + (rand()%60);
    }
    glGenVertexArrays(1, &g_rainVAO); glBindVertexArray(g_rainVAO);
    glGenBuffers(1, &g_rainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, g_rainVBO);
    glBufferData(GL_ARRAY_BUFFER, RAIN_COUNT * sizeof(RainDrop), g_rainDrops.data(), GL_DYNAMIC_DRAW);
}

void UpdateRain(float dt) {
    if (!g_rainOn) return;
    for (auto& d : g_rainDrops) {
        d.pos.y -= d.speed * dt;
        if (d.pos.y < 0.f) {
            d.pos.y = 80.f + (rand()%30);
            d.pos.x = (rand()%600)-300.f;
            d.pos.z = (rand()%600)-300.f;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, g_rainVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, RAIN_COUNT * sizeof(RainDrop), g_rainDrops.data());
}

void DrawRain() {
    if (!g_rainOn) return;
    glUseProgram(g_shParticle);
    glm::mat4 view = g_cam->GetViewMatrix(NULL);
    glm::mat4 proj = g_cam->GetProjectionMatrix(NULL);
    glm::mat4 ident(1.f);
    UM4(g_shParticle, "world_mat",      ident);
    UM4(g_shParticle, "view_mat",       view);
    UM4(g_shParticle, "projection_mat", proj);
    U1f(g_shParticle, "time",       g_time);
    U1f(g_shParticle, "fogDensity", g_fogDensity);
    glBindVertexArray(g_rainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_rainVBO);
    GLint va = glGetAttribLocation(g_shParticle, "vertex");
    if (va >= 0) { glVertexAttribPointer(va,3,GL_FLOAT,GL_FALSE,sizeof(RainDrop),(void*)0);  glEnableVertexAttribArray(va); }
    GLint ca = glGetAttribLocation(g_shParticle, "color");
    if (ca >= 0) { glVertexAttribPointer(ca,3,GL_FLOAT,GL_FALSE,sizeof(RainDrop),(void*)12); glEnableVertexAttribArray(ca); }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, RAIN_COUNT);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

// ---------------------------------------------------------------------------
//  InitScene
// ---------------------------------------------------------------------------
void InitScene() {
    CreateCityGeometry();
    CreateStreetLights();
    CreateNeonLights();
    CreateTrafficLights();
    CreateCarMeshes();
    InitRain();
    g_streetLampPost = MakeCylinder(0.4f, 20.f, 8, glm::vec3(0.4f,0.4f,0.45f));
    g_lampHead       = MakeBox(2.5f, 1.0f, 2.5f, glm::vec3(0.9f,0.85f,0.7f));
    g_lightShaft     = MakeCone(5.f, 18.f, 16);
    g_trafficLight   = MakeBox(2.f, 6.f, 2.f, glm::vec3(0.15f,0.15f,0.15f));
    g_neonSign       = MakeBox(12.f, 3.f, 0.3f, glm::vec3(1,0.1f,0.8f));
    CreateVehicles();

    // T8 - Load road texture
    {
        stbi_set_flip_vertically_on_load(1);
        int w, h, ch;
        // Try to load a real PNG; fall back to procedural stub if missing
        unsigned char* data = stbi_load("textures/road.png", &w, &h, &ch, 3);
        if (!data) {
            // stb_image stub generates a 64x64 procedural road pattern
            data = stbi_load("road.png", &w, &h, &ch, 3);
        }
        if (data) {
            glGenTextures(1, &g_roadTexture);
            glBindTexture(GL_TEXTURE_2D, g_roadTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            stbi_image_free(data);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

// ---------------------------------------------------------------------------
//  Update
// ---------------------------------------------------------------------------
void Update(float dt) {
    if (g_paused) return;
    g_time += dt;
    // ---------------------------------------------------------------------------
    //  Vehicle collision avoidance — prevent cars on the same lane from merging.
    //  Each car checks every other car on the same lane (same X ± tolerance and
    //  same direction).  If a car is within MIN_GAP ahead, this car's effective
    //  speed is blended down so it trails at a safe distance.
    // ---------------------------------------------------------------------------
    const float LANE_TOL   = 4.f;    // X-distance to consider "same lane"
    const float CAR_LEN    = 18.f;   // car length + small bumper margin
    const float MIN_GAP    = CAR_LEN;
    const float BRAKE_ZONE = MIN_GAP * 3.f; // start easing at 3× gap

    for (auto& v : g_vehicles) {
        float baseSpd = v.speed * g_vehicleSpeed;
        float effSpd  = baseSpd;

        for (auto& other : g_vehicles) {
            if (&other == &v) continue;

            // Same lane: close X, same forward direction
            if (fabsf(other.pos.x - v.pos.x) > LANE_TOL) continue;
            if (glm::dot(v.dir, other.dir) < 0.5f)        continue;

            // "Ahead" = other is further along v.dir from v
            float ahead = glm::dot(other.pos - v.pos, v.dir);
            if (ahead <= 0.f) continue;

            // Ignore wrap-around ghosts (gap > half route length)
            if (ahead > v.routeLen * 0.5f) continue;

            if (ahead < BRAKE_ZONE) {
                // Blend speed: full at BRAKE_ZONE, match leader at MIN_GAP, stop if closer
                float t       = (ahead - MIN_GAP) / (BRAKE_ZONE - MIN_GAP);
                t             = glm::clamp(t, 0.f, 1.f);
                float cappedSpd = other.speed * g_vehicleSpeed * t;
                effSpd = glm::min(effSpd, glm::max(cappedSpd, 0.f));
            }
        }

        v.t += (effSpd * dt) / v.routeLen;
        if (v.t > 1.f) v.t -= 1.f;
        v.pos += v.dir * effSpd * dt;
        if (v.pos.z >  260.f) v.pos.z = -260.f;
        if (v.pos.z < -260.f) v.pos.z =  260.f;
    }
    g_trafficTimer += dt;
    if (g_trafficTimer > 4.f) { g_trafficTimer = 0.f; g_trafficState = (g_trafficState+1)%3; }
    UpdateRain(dt);
}

// ---------------------------------------------------------------------------
//  DrawScene
// ---------------------------------------------------------------------------
void DrawScene() {
    glm::mat4 I(1.f);

    // Road (T8 - textured)
    if (g_roadTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_roadTexture);
        U1i(g_shCity, "tex", 0);
    }
    SetCityUniforms(g_shCity, I, glm::vec3(0.02f), glm::vec3(0.08f,0.08f,0.10f), glm::vec3(0.15f),
                    12.f, 0, 0, glm::vec3(0), 0.f, g_roadTexture ? 1 : 0);
    DrawMesh(g_road, g_shCity);
    if (g_roadTexture) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Sidewalks
    float swZ[] = {-290,-270};
    for (int i = 0; i < 2; i++) {
        glm::mat4 w = glm::translate(I, glm::vec3(0,0.01f,swZ[i]));
        SetCityUniforms(g_shCity, w, glm::vec3(0.03f), glm::vec3(0.22f,0.22f,0.24f), glm::vec3(0.1f),
                        4.f, 0, 0, glm::vec3(0), 0.f, 0);
        DrawMesh(g_sidewalk, g_shCity);
    }

    // Buildings
    for (int i = 0; i < (int)g_buildings.size() && i < (int)g_buildingDefs.size(); i++) {
        auto& d = g_buildingDefs[i];
        glm::mat4 w = glm::translate(I, glm::vec3(d.x, 0.f, d.z));
        SetCityUniforms(g_shCity, w, glm::vec3(0.03f), d.col, glm::vec3(0.3f,0.3f,0.4f),
                        20.f, 0, 0, glm::vec3(0), 0.f, 0);
        DrawMesh(g_buildings[i], g_shCity);
    }

    // Lamp posts
    {
        float lampZ[] = {-200,-140,-80,-20,40,100,160,200};
        float sides[] = {-75.f, 75.f};
        for (float sx : sides) {
            for (float sz : lampZ) {
                glm::mat4 wp = glm::translate(I, glm::vec3(sx,0.f,sz));
                SetCityUniforms(g_shCity, wp, glm::vec3(0.05f), glm::vec3(0.4f,0.4f,0.45f), glm::vec3(0.2f),
                                8.f, 0, 0, glm::vec3(0), 0.f, 0);
                DrawMesh(g_streetLampPost, g_shCity);

                glm::mat4 wh = glm::translate(I, glm::vec3(sx,20.5f,sz));
                float emStr = g_streetLightsOn ? 2.5f : 0.0f;
                SetCityUniforms(g_shCity, wh, glm::vec3(0.5f), glm::vec3(1.f,0.9f,0.7f), glm::vec3(0.5f),
                                30.f, 0, 1, glm::vec3(1.0f,0.9f,0.65f), emStr, 0);
                DrawMesh(g_lampHead, g_shCity);

                if (g_streetLightsOn) {
                    glm::mat4 ws = glm::translate(I, glm::vec3(sx,20.f,sz));
                    DrawShaft(g_lightShaft, ws, glm::vec3(1.f,0.85f,0.5f), g_shaftAlpha);
                }
            }
        }
    }

    // Traffic lights
    {
        glm::vec3 tc = (g_trafficState == 0) ? glm::vec3(1,0,0) :
                       (g_trafficState == 1) ? glm::vec3(1,0.6f,0) :
                                               glm::vec3(0,1,0);
        float px[] = {-78, 78, -78, 78};
        float pz[] = { -5, -5,   5,  5};
        for (int i = 0; i < 4; i++) {
            glm::mat4 wt = glm::translate(I, glm::vec3(px[i],0.f,pz[i]));
            SetCityUniforms(g_shCity, wt, glm::vec3(0.02f), glm::vec3(0.15f,0.15f,0.15f), glm::vec3(0.1f),
                            4.f, 0, 0, glm::vec3(0), 0.f, 0);
            DrawMesh(g_trafficLight, g_shCity);

            glm::mat4 wg = glm::translate(I, glm::vec3(px[i],6.5f,pz[i]));
            float fl = 0.8f + 0.2f * sinf(g_time * 3.f);
            SetCityUniforms(g_shCity, wg, glm::vec3(0.1f), tc*0.5f, glm::vec3(0.2f),
                            4.f, 0, 1, tc, fl * 2.5f, 0);
            DrawMesh(g_lampHead, g_shCity);
        }
    }

    // Neon signs
    if (g_neonOn) {
        for (auto& nl : g_neonLights) {
            float flicker = 1.0f;
            if (g_neonFlicker == 1) flicker = 0.85f + 0.15f * sinf(g_time * 3.0f + nl.pos.x * 0.1f);
            if (g_neonFlicker == 2) flicker = (sinf(g_time * 15.f + nl.pos.z * 0.2f) > 0.3f) ? 1.f : 0.4f;
            glm::mat4 wn = glm::translate(I, nl.pos);
            if (nl.pos.x < 0) wn = glm::rotate(wn, glm::radians(90.f),  glm::vec3(0,1,0));
            else               wn = glm::rotate(wn, glm::radians(-90.f), glm::vec3(0,1,0));
            SetCityUniforms(g_shCity, wn, glm::vec3(0.1f), nl.color, glm::vec3(0.3f),
                            8.f, 0, 1, nl.color, flicker * 3.f, 0);
            DrawMesh(g_neonSign, g_shCity);
        }
    }

    // Vehicles
    for (auto& v : g_vehicles) {
        float angle = atan2f(v.dir.x, v.dir.z);
        glm::mat4 wv = glm::translate(I, v.pos + glm::vec3(0,1.25f,0));
        wv = glm::rotate(wv, angle, glm::vec3(0,1,0));

        glm::vec3 headlightPosWorld = v.pos + v.dir * 8.f + glm::vec3(0,2.f,0);

        SetCityUniforms(g_shCity, wv, glm::vec3(0.05f), v.bodyColor, glm::vec3(0.5f,0.5f,0.6f),
                        30.f, 0, 0, glm::vec3(0), 0.f, 0);
        if (g_headlightsOn) {
            U1i(g_shCity, "hasHeadlight",       1);
            U3f(g_shCity, "headlightPos",        headlightPosWorld);
            U3f(g_shCity, "headlightDir",        v.dir);
            U1f(g_shCity, "headlightCone",       g_headlightCone);
            U3f(g_shCity, "headlightColor",      v.hlColor);
            U1f(g_shCity, "headlightIntensity",  2.5f * g_globalLightInt);
        } else {
            U1i(g_shCity, "hasHeadlight", 0);
        }
        DrawMesh(g_carBody, g_shCity);

        glm::mat4 wr = glm::translate(wv, glm::vec3(0,2.5f,0.5f));
        SetCityUniforms(g_shCity, wr, glm::vec3(0.05f), v.bodyColor * 0.7f, glm::vec3(0.3f),
                        15.f, 0, 0, glm::vec3(0), 0.f, 0);
        U1i(g_shCity, "hasHeadlight", 0);
        DrawMesh(g_carTop, g_shCity);

        if (g_headlightsOn) {
            glm::mat4 whl = glm::translate(wv, glm::vec3( 3.f,0.5f,-7.5f));
            SetCityUniforms(g_shCity, whl, glm::vec3(0.5f), glm::vec3(1.f,0.95f,0.85f), glm::vec3(0.5f),
                            10.f, 0, 1, glm::vec3(1.f,0.95f,0.85f), 3.f, 0);
            DrawMesh(g_lampHead, g_shCity);
            glm::mat4 whr = glm::translate(wv, glm::vec3(-3.f,0.5f,-7.5f));
            SetCityUniforms(g_shCity, whr, glm::vec3(0.5f), glm::vec3(1.f,0.95f,0.85f), glm::vec3(0.5f),
                            10.f, 0, 1, glm::vec3(1.f,0.95f,0.85f), 3.f, 0);
            DrawMesh(g_lampHead, g_shCity);

            glm::mat4 wsh = glm::translate(I, headlightPosWorld);
            float ha = atan2f(v.dir.x, v.dir.z);
            wsh = glm::rotate(wsh, ha + glm::pi<float>(), glm::vec3(0,1,0));
            wsh = glm::rotate(wsh, glm::radians(-80.f), glm::vec3(1,0,0));
            DrawShaft(g_lightShaft, wsh, glm::vec3(1.0f,0.9f,0.7f), g_shaftAlpha * 0.8f);
        }
    }

    DrawRain();
}
