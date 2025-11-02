//
// Created by 2401Lucas on 2025-11-01.
//

#include "Transform.h"

Transform Transform::Identity() {
    Transform t;
    for (int i = 0; i < 16; i++) t.matrix[i] = 0.0f;
    t.matrix[0] = t.matrix[5] = t.matrix[10] = t.matrix[15] = 1.0f;
    return t;
}

Transform Transform::Translation(float x, float y, float z) {
    Transform t = Identity();
    t.matrix[12] = x;
    t.matrix[13] = y;
    t.matrix[14] = z;
    return t;
}

Transform Transform::Scale(float x, float y, float z) {
    Transform t = Identity();
    t.matrix[0] = x;
    t.matrix[5] = y;
    t.matrix[10] = z;
    return t;
}
