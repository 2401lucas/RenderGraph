//
// Created by 2401Lucas on 2025-11-01.
//

#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

Transform::Transform() {
    m_position = glm::vec3(0, 0, 0);
    m_position = glm::vec3(0, 0, 0);
    m_scale = glm::vec3(1, 1, 1);
    m_transformMatUpdated = true;
}

Transform::Transform(glm::vec3 pos, glm::vec3 rot, glm::vec3 sca)
    : m_position(pos), m_rotation(rot), m_scale(sca) {
    m_transformMatUpdated = true;
}

void Transform::CalculateTransformMat() {
    glm::mat4 rotMat = glm::mat4(1.0f);
    rotMat = glm::rotate(rotMat, glm::radians(m_rotation.x * -1.0f),
                         glm::vec3(1.0f, 0.0f, 0.0f));
    rotMat = glm::rotate(rotMat, glm::radians(m_rotation.y),
                         glm::vec3(0.0f, 1.0f, 0.0f));
    rotMat = glm::rotate(rotMat, glm::radians(m_rotation.z),
                         glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 posMat = glm::translate(glm::mat4(1.0f), m_position);
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), m_scale);

    m_transformMat = posMat * rotMat * scaleMat;
    m_transformMatUpdated = false;
}
