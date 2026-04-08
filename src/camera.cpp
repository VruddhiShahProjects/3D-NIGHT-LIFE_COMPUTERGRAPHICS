/*
COSC 3307 - 3D Night Life Project
Camera implementation
*/
#define GLM_FORCE_RADIANS
#include <camera.h>

Camera::Camera(void)
    : position(0,0,0), forwardVector(0,0,1), upVector(0,1,0),
      fieldOfView(60.f), aspectRatio(800.f/600.f), nearPlane(0.01f), farPlane(5000.f)
{
    orientation_ = glm::quat();
}
Camera::~Camera(void){}

int Camera::Roll(float deg){
    orientation_ = glm::normalize(glm::angleAxis(glm::radians(deg), GetForward()) * orientation_);
    return 0;
}
int Camera::Pitch(float deg){
    orientation_ = glm::normalize(glm::angleAxis(glm::radians(deg), GetSide()) * orientation_);
    return 0;
}
int Camera::Yaw(float deg){
    orientation_ = glm::normalize(glm::angleAxis(glm::radians(deg), GetUp()) * orientation_);
    return 0;
}

glm::vec3 Camera::MoveForward (float n){ position += n*GetForward(); return position; }
glm::vec3 Camera::MoveBackward(float n){ return MoveForward(-n); }
glm::vec3 Camera::MoveRight   (float n){ position += n*GetSide();    return position; }
glm::vec3 Camera::moveLeft    (float n){ return MoveRight(-n); }
glm::vec3 Camera::MoveUp      (float n){ position += n*GetUp();      return position; }
glm::vec3 Camera::MoveDown    (float n){ return MoveUp(-n); }

glm::vec3 Camera::GetPosition   (){ return position; }
glm::vec3 Camera::GetLookAtPoint(){ return position + GetForward(); }
glm::vec3 Camera::GetForward() const { return glm::normalize(orientation_ * forwardVector); }
glm::vec3 Camera::GetSide()    const { return glm::normalize(glm::cross(GetForward(), GetUp())); }
glm::vec3 Camera::GetUp()      const { return glm::normalize(orientation_ * upVector); }

glm::mat4 Camera::GetViewMatrix(glm::mat4* out){
    glm::mat4 m = glm::lookAt(position, GetLookAtPoint(), GetUp());
    if(out) *out = m;
    return m;
}
glm::mat4 Camera::GetProjectionMatrix(glm::mat4* out){
    glm::mat4 m = glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
    if(out) *out = m;
    return m;
}
void Camera::SetCamera(glm::vec3 pos, glm::vec3 look, glm::vec3 up){
    position      = pos;
    forwardVector = glm::normalize(look - pos);
    upVector      = glm::normalize(up);
    orientation_  = glm::quat();
}
int Camera::SetPerspectiveView(float fov, float ar, float np, float fp){
    fieldOfView=fov; aspectRatio=ar; nearPlane=np; farPlane=fp;
    return 0;
}

void Camera::ZoomIn(float amount){
    fieldOfView -= amount;
    if(fieldOfView < 5.0f) fieldOfView = 5.0f;
}

void Camera::ZoomOut(float amount){
    fieldOfView += amount;
    if(fieldOfView > 170.0f) fieldOfView = 170.0f;
}

void Camera::ChangeForwardVector(float x, float y, float z){
    forwardVector = glm::normalize(glm::vec3(x, y, z));
    orientation_  = glm::quat();   // reset rotation so new forward takes effect
}
