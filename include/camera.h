/*
COSC 3307 - 3D Night Life Project
Camera class header
*/
#ifndef CAMERA_H_
#define CAMERA_H_

#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#pragma once
class Camera {
public:
    Camera(void);
    ~Camera(void);

    int Roll (float angleDeg);
    int Pitch(float angleDeg);
    int Yaw  (float angleDeg);

    glm::vec3 MoveForward (float n);
    glm::vec3 MoveBackward(float n);
    glm::vec3 MoveRight   (float n);
    glm::vec3 moveLeft    (float n);
    glm::vec3 MoveUp      (float n);
    glm::vec3 MoveDown    (float n);

    glm::vec3 GetPosition   (void);
    glm::vec3 GetLookAtPoint(void);
    glm::vec3 GetSide       (void) const;
    glm::vec3 GetForward    (void) const;
    glm::vec3 GetUp         (void) const;

    glm::mat4 GetViewMatrix      (glm::mat4* out);
    glm::mat4 GetProjectionMatrix(glm::mat4* out);

    void SetCamera         (glm::vec3 pos, glm::vec3 lookAt, glm::vec3 up);
    int  SetPerspectiveView(float fov, float ar, float nearP, float farP);

private:
    glm::vec3 position, upVector, forwardVector;
    glm::quat orientation_;
    float fieldOfView, aspectRatio, nearPlane, farPlane;
};

#endif
