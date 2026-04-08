#pragma once
/*
=============================================================================
COSC 3307 - 3D Night Life
Renderer: shader utilities, mesh builders, draw helpers
=============================================================================
*/
#include <globals.h>
#include <string>

// Shader utilities
std::string ReadFile(const std::string& path);
GLuint CompileShader(GLenum type, const std::string& src, const std::string& label);
GLuint LoadShaders(const std::string& base);

// Uniform helpers
void U1f (GLuint p, const char* n, float v);
void U1i (GLuint p, const char* n, int v);
void U3f (GLuint p, const char* n, glm::vec3 v);
void UM4 (GLuint p, const char* n, glm::mat4 m);
void U3fv(GLuint p, const char* n, int cnt, const float* v);
void U1fv(GLuint p, const char* n, int cnt, const float* v);
void U1iv(GLuint p, const char* n, int cnt, const int* v);

// Mesh builders
Mesh* UploadMesh();
void  BindCityAttribs(Mesh* m, GLuint prog);
void  AddQuad(unsigned int a, unsigned int b, unsigned int c, unsigned int d);
Mesh* MakeBox(float w, float h, float d, glm::vec3 col);
Mesh* MakeFlat(float w, float d, glm::vec3 col, float tileU = 1.f, float tileV = 1.f);
Mesh* MakeCylinder(float r, float h, int segs, glm::vec3 col);
Mesh* MakeCone(float r, float h, int segs);

// Draw helpers
void SetCityUniforms(GLuint prog, glm::mat4 world,
                     glm::vec3 matAmb, glm::vec3 matDif, glm::vec3 matSpec,
                     float shin, int useVC, int emissive,
                     glm::vec3 emCol, float emStr, int hasTexture);
void DrawMesh(Mesh* m, GLuint prog);
void DrawShaft(Mesh* m, glm::mat4 world, glm::vec3 col, float alpha);

// Shadow mapping
void InitShadowMap();
void DrawSceneDepth(glm::mat4 lightSpaceMat);   // depth pass (shadow casters only)
