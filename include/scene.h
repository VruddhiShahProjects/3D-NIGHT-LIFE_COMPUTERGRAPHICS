#pragma once
/*
=============================================================================
COSC 3307 - 3D Night Life
Scene: geometry creation, lighting, vehicles, rain, update, draw
=============================================================================
*/
#include <globals.h>

struct BuildingDef { float x, z, w, h, d; glm::vec3 col; };
extern std::vector<BuildingDef> g_buildingDefs;

void CreateCityGeometry();
void CreateStreetLights();
void CreateNeonLights();
void CreateTrafficLights();
void CreateVehicles();
void CreateCarMeshes();
void InitRain();
void InitScene();

void Update(float dt);
void UpdateRain(float dt);

void DrawScene();
void DrawRain();
