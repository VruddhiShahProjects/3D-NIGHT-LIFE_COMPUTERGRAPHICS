/*
=============================================================================
COSC 3307 - 3D Night Life
Renderer implementation: shaders, mesh builders, draw calls
=============================================================================
*/
#include <renderer.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
//  Shader utilities
// ---------------------------------------------------------------------------
std::string ReadFile(const std::string& p) {
    std::ifstream f(p.c_str());
    if (!f.is_open()) throw std::runtime_error("Cannot open: " + p);
    std::string s, l;
    while (std::getline(f, l)) s += l + "\n";
    return s;
}

GLuint CompileShader(GLenum type, const std::string& src, const std::string& label) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, NULL);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char b[4096]; glGetShaderInfoLog(s, 4096, NULL, b);
        throw std::runtime_error("Compile [" + label + "]: " + b);
    }
    return s;
}

GLuint LoadShaders(const std::string& base) {
    GLuint vs = CompileShader(GL_VERTEX_SHADER,   ReadFile(base + ".vert"), base + ".vert");
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, ReadFile(base + ".frag"), base + ".frag");
    GLuint p  = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char b[4096]; glGetProgramInfoLog(p, 4096, NULL, b);
        throw std::runtime_error("Link [" + base + "]: " + b);
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

// ---------------------------------------------------------------------------
//  Uniform helpers
// ---------------------------------------------------------------------------
void U1f (GLuint p, const char* n, float v)         { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniform1f(l, v); }
void U1i (GLuint p, const char* n, int v)           { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniform1i(l, v); }
void U3f (GLuint p, const char* n, glm::vec3 v)     { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniform3fv(l, 1, glm::value_ptr(v)); }
void UM4 (GLuint p, const char* n, glm::mat4 m)     { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(m)); }
void U3fv(GLuint p, const char* n, int cnt, const float* v) { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniform3fv(l, cnt, v); }
void U1fv(GLuint p, const char* n, int cnt, const float* v) { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniform1fv(l, cnt, v); }
void U1iv(GLuint p, const char* n, int cnt, const int* v)   { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniform1iv(l, cnt, v); }

// ---------------------------------------------------------------------------
//  Mesh builder helpers
// ---------------------------------------------------------------------------
Mesh* UploadMesh() {
    Mesh* m = new Mesh();
    m->indexCount = (GLuint)g_itmp.size();
    glGenVertexArrays(1, &m->vao); glBindVertexArray(m->vao);
    glGenBuffers(1, &m->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * g_vtmp.size(), g_vtmp.data(), GL_DYNAMIC_DRAW);
    glGenBuffers(1, &m->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * g_itmp.size(), g_itmp.data(), GL_STATIC_DRAW);
    g_vtmp.clear(); g_itmp.clear();
    return m;
}

void BindCityAttribs(Mesh* m, GLuint prog) {
    glBindVertexArray(m->vao);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
    GLint va = glGetAttribLocation(prog, "vertex");
    if (va >= 0) { glVertexAttribPointer(va, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);  glEnableVertexAttribArray(va); }
    GLint ca = glGetAttribLocation(prog, "color");
    if (ca >= 0) { glVertexAttribPointer(ca, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12); glEnableVertexAttribArray(ca); }
    GLint na = glGetAttribLocation(prog, "normal");
    if (na >= 0) { glVertexAttribPointer(na, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)24); glEnableVertexAttribArray(na); }
    GLint ta = glGetAttribLocation(prog, "texCoord");
    if (ta >= 0) { glVertexAttribPointer(ta, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)36); glEnableVertexAttribArray(ta); }
}

void AddQuad(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
    g_itmp.push_back(a); g_itmp.push_back(b); g_itmp.push_back(c);
    g_itmp.push_back(a); g_itmp.push_back(c); g_itmp.push_back(d);
}

Mesh* MakeBox(float w, float h, float d, glm::vec3 col) {
    float hw = w * 0.5f, hd = d * 0.5f;
    glm::vec3 top(0,1,0), bot(0,-1,0), front(0,0,1), back(0,0,-1), right(1,0,0), left(-1,0,0);
    auto PV = [&](float x, float y, float z, glm::vec3 n, float u, float v) {
        Vertex vt; vt.pos = glm::vec3(x,y,z); vt.color = col; vt.normal = n; vt.texCoord = glm::vec2(u,v);
        g_vtmp.push_back(vt);
    };
    unsigned int base = (unsigned int)g_vtmp.size();
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

Mesh* MakeFlat(float w, float d, glm::vec3 col, float tileU, float tileV) {
    float hw = w * 0.5f, hd = d * 0.5f;
    glm::vec3 n(0,1,0);
    g_vtmp.push_back({glm::vec3(-hw,0,-hd), col, n, glm::vec2(0,0)});
    g_vtmp.push_back({glm::vec3( hw,0,-hd), col, n, glm::vec2(tileU,0)});
    g_vtmp.push_back({glm::vec3( hw,0, hd), col, n, glm::vec2(tileU,tileV)});
    g_vtmp.push_back({glm::vec3(-hw,0, hd), col, n, glm::vec2(0,tileV)});
    AddQuad(0,1,2,3);
    return UploadMesh();
}

Mesh* MakeCylinder(float r, float h, int segs, glm::vec3 col) {
    for (int i = 0; i < segs; i++) {
        float a0 = 2.f * glm::pi<float>() * i / segs;
        float a1 = 2.f * glm::pi<float>() * (i+1) / segs;
        glm::vec3 n0(cosf(a0),0,sinf(a0)), n1(cosf(a1),0,sinf(a1));
        unsigned int vi = (unsigned int)g_vtmp.size();
        g_vtmp.push_back({glm::vec3(r*cosf(a0),0,r*sinf(a0)), col, n0, glm::vec2(0,0)});
        g_vtmp.push_back({glm::vec3(r*cosf(a1),0,r*sinf(a1)), col, n1, glm::vec2(1,0)});
        g_vtmp.push_back({glm::vec3(r*cosf(a1),h,r*sinf(a1)), col, n1, glm::vec2(1,1)});
        g_vtmp.push_back({glm::vec3(r*cosf(a0),h,r*sinf(a0)), col, n0, glm::vec2(0,1)});
        AddQuad(vi, vi+1, vi+2, vi+3);
    }
    return UploadMesh();
}

Mesh* MakeCone(float r, float h, int segs) {
    glm::vec3 col(1, 1, 0.8f);
    unsigned int apex = (unsigned int)g_vtmp.size();
    g_vtmp.push_back({glm::vec3(0,0,0), col, glm::vec3(0,1,0), glm::vec2(0.5f,0)});
    for (int i = 0; i < segs; i++) {
        float a0 = 2.f * glm::pi<float>() * i / segs;
        float a1 = 2.f * glm::pi<float>() * (i+1) / segs;
        unsigned int vi = (unsigned int)g_vtmp.size();
        g_vtmp.push_back({glm::vec3(r*cosf(a0),-h,r*sinf(a0)), col, glm::vec3(0,-1,0), glm::vec2(0,1)});
        g_vtmp.push_back({glm::vec3(r*cosf(a1),-h,r*sinf(a1)), col, glm::vec3(0,-1,0), glm::vec2(1,1)});
        g_itmp.push_back(apex); g_itmp.push_back(vi); g_itmp.push_back(vi+1);
    }
    return UploadMesh();
}

// ---------------------------------------------------------------------------
//  SetCityUniforms
// ---------------------------------------------------------------------------
void SetCityUniforms(GLuint prog, glm::mat4 world,
                     glm::vec3 matAmb, glm::vec3 matDif, glm::vec3 matSpec,
                     float shin, int useVC, int emissive,
                     glm::vec3 emCol, float emStr, int hasTexture)
{
    glUseProgram(prog);
    glm::mat4 view = g_cam->GetViewMatrix(NULL);
    glm::mat4 proj = g_cam->GetProjectionMatrix(NULL);
    UM4(prog, "world_mat",       world);
    UM4(prog, "view_mat",        view);
    UM4(prog, "projection_mat",  proj);

    // Professor convention names (renamed from mat_ambient etc.)
    U3f(prog, "ambient_color",   matAmb);
    U3f(prog, "diffuse_color",   matDif);
    U3f(prog, "specular_color",  matSpec);
    U1f(prog, "shine",           shin);

    // Separate coefficient uniforms (T7)
    U1f(prog, "coefA", 1.0f);
    U1f(prog, "coefD", 1.0f);
    U1f(prog, "coefS", 1.0f);

    // T7 toggle uniforms — all on by default, half-vector = Blinn-Phong
    U1i(prog, "ambientOn",     1);
    U1i(prog, "diffuseOn",     1);
    U1i(prog, "specularOn",    1);
    U1i(prog, "useHalfVector", 1);

    U1i(prog, "useVertexColor",  useVC);
    U1i(prog, "isEmissive",      emissive);
    U3f(prog, "emissiveColor",   emCol);
    U1f(prog, "emissiveStrength",emStr);
    U1i(prog, "hasTexture",      hasTexture);
    U3f(prog, "fogColor",        g_fogColor);
    U1f(prog, "fogDensity",      g_fogDensity);
    U1f(prog, "fogStart",        5.0f);
    U1f(prog, "fogEnd",          800.0f);
    U1f(prog, "time",            g_time);
    U1i(prog, "rainEnabled",     g_rainOn ? 1 : 0);

    // Spotlight defaults
    U1i(prog, "useSpotlight",    1);
    U1f(prog, "spotlightAtten",  4.0f);

    // Pack active lights
    static glm::vec3 posArr[16];
    static glm::vec3 colArr[16];
    static float     intArr[16];
    static float     radArr[16];
    int cnt = 0;

    if (g_streetLightsOn) {
        for (auto& lp : g_streetLights) {
            if (!lp.enabled || cnt >= 16) continue;
            posArr[cnt] = lp.pos;
            colArr[cnt] = lp.color * g_globalLightInt;
            intArr[cnt] = lp.intensity;
            radArr[cnt] = lp.radius;
            cnt++;
        }
    }
    if (g_neonOn) {
        for (auto& lp : g_neonLights) {
            if (!lp.enabled || cnt >= 16) continue;
            float flicker = 1.0f;
            if (g_neonFlicker == 1) flicker = 0.85f + 0.15f * sinf(g_time * 3.0f + lp.pos.x * 0.1f);
            if (g_neonFlicker == 2) flicker = (sinf(g_time * 15.f + lp.pos.z * 0.2f) > 0.3f) ? 1.f : 0.4f;
            posArr[cnt] = lp.pos;
            colArr[cnt] = lp.color * g_globalLightInt * flicker;
            intArr[cnt] = lp.intensity;
            radArr[cnt] = lp.radius;
            cnt++;
        }
    }
    {
        glm::vec3 tc = (g_trafficState == 0) ? glm::vec3(1,0,0) :
                       (g_trafficState == 1) ? glm::vec3(1,0.6f,0) :
                                               glm::vec3(0,1,0);
        for (auto& lp : g_trafficLights) {
            if (!lp.enabled || cnt >= 16) continue;
            posArr[cnt] = lp.pos;
            colArr[cnt] = tc * g_globalLightInt;
            intArr[cnt] = lp.intensity;
            radArr[cnt] = lp.radius;
            cnt++;
        }
    }

    U1i(prog, "numLights", cnt);
    if (cnt > 0) {
        U3fv(prog, "lightPos",       cnt, (float*)posArr);
        U3fv(prog, "lightColor",     cnt, (float*)colArr);
        U1fv(prog, "lightIntensity", cnt, intArr);
        U1fv(prog, "lightRadius",    cnt, radArr);
    }
    UM4(prog, "view_mat", view);
}

// ---------------------------------------------------------------------------
//  DrawMesh / DrawShaft
// ---------------------------------------------------------------------------
void DrawMesh(Mesh* m, GLuint prog) {
    BindCityAttribs(m, prog);
    glDrawElements(GL_TRIANGLES, m->indexCount, GL_UNSIGNED_INT, 0);
}

void DrawShaft(Mesh* m, glm::mat4 world, glm::vec3 col, float alpha) {
    glUseProgram(g_shShaft);
    glm::mat4 view = g_cam->GetViewMatrix(NULL);
    glm::mat4 proj = g_cam->GetProjectionMatrix(NULL);
    UM4(g_shShaft, "world_mat",      world);
    UM4(g_shShaft, "view_mat",       view);
    UM4(g_shShaft, "projection_mat", proj);
    U3f(g_shShaft, "shaftColor",     col);
    U1f(g_shShaft, "shaftAlpha",     alpha);
    U1f(g_shShaft, "fogDensity",     g_fogDensity);
    BindCityAttribs(m, g_shShaft);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDrawElements(GL_TRIANGLES, m->indexCount, GL_UNSIGNED_INT, 0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
