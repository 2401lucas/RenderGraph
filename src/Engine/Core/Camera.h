//
// Created by 2401Lucas on 2025-11-01.
//

#ifndef GPU_PARTICLE_SIM_CAMERA_H
#define GPU_PARTICLE_SIM_CAMERA_H
#include "Transform.h"

// TODO Req: IMPLEMENT CAMERA
class Camera {
public:
    Camera(Transform, float aspectRatio, float fov, float zNear, float zFar);

    void Update();

    Transform &GetTransform();

    void SetTransform(const Transform &transform);

    const float GetFov() const;

    void SetFov(float);

    const float GetAspectRatio() const;

    void SetAspectRatio(float);

    const float GetZNear() const;

    void SetZNear(float);

    const float GetZFar() const;

    void SetZFar(float);

    void GetPerspective() const;

    void GetFrustum() const;

private:
    void CalculatePerspectiveMatrix();

    Transform m_transform;
    bool m_updatePerspective = true;
    float m_perspectiveMat[16];
    float m_aspectRatio;
    float m_fov;
    float m_zNear;
    float m_zFar;
};


#endif //GPU_PARTICLE_SIM_CAMERA_H
