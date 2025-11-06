//
// Created by 2401Lucas on 2025-11-01.
//

#include "Camera.h"

#include <cmath>
#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>

Camera::Camera(Transform transform, float aspectRatio, float fov, float zNear, float zFar) : m_transform(transform),
    m_aspectRatio(aspectRatio), m_zNear(zNear), m_zFar(zFar), m_fov(fov) {
}

void Camera::Update() {
    if (m_updatePerspective)
        CalculatePerspectiveMatrix();
}

Transform &Camera::GetTransform() {
    return m_transform;
}

void Camera::SetTransform(const Transform &transform) {
    m_transform = transform;
}

const float Camera::GetFov() const {
    return m_fov;
}

void Camera::SetFov(float fov) {
    if (m_fov == fov) return;
    m_fov = fov;
    m_updatePerspective = true;
}

const float Camera::GetAspectRatio() const {
    return m_aspectRatio;
}

void Camera::SetAspectRatio(float ar) {
    if (m_aspectRatio == ar) return;
    m_aspectRatio = ar;
    m_updatePerspective = true;
}

const float Camera::GetZNear() const {
    return m_zNear;
}

void Camera::SetZNear(float near) {
    if (m_zNear == near) return;
    m_zNear = near;
    m_updatePerspective = true;
}

const float Camera::GetZFar() const {
    return m_zFar;
}

void Camera::SetZFar(float far) {
    if (m_zFar == far) return;
    m_zFar = far;
    m_updatePerspective = true;
}

glm::mat4x4 Camera::GetPerspective() {
    if (m_updatePerspective) CalculatePerspectiveMatrix();
    return m_perspectiveMat;
}

void Camera::CalculatePerspectiveMatrix() {
    m_perspectiveMat =
            glm::perspective(glm::radians(m_fov), m_aspectRatio, m_zNear, m_zFar);
    m_perspectiveMat[1][1] *= -1;
    m_updatePerspective = false;
}
