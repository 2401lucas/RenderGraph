//
// Created by 2401Lucas on 2025-11-01.
//

#include "Camera.h"

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
    m_fov = fov;
}

const float Camera::GetAspectRatio() const {
    return m_aspectRatio;
}

void Camera::SetAspectRatio(float ar) {
    m_aspectRatio = ar;
}

const float Camera::GetZNear() const {
    return m_zNear;
}

void Camera::SetZNear(float near) {
    m_zNear = near;
}

const float Camera::GetZFar() const {
    return m_zFar;
}

void Camera::SetZFar(float far) {
    m_zFar = far;
}

void Camera::GetPerspective() const {

}

void Camera::GetFrustum() const {
}

void Camera::CalculatePerspectiveMatrix() {
}
