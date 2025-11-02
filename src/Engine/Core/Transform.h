//
// Created by 2401Lucas on 2025-11-01.
//

#ifndef GPU_PARTICLE_SIM_TRANSFORM_H
#define GPU_PARTICLE_SIM_TRANSFORM_H


class Transform {
public:
    float matrix[16]; // 4x4 matrix

    static Transform Identity();

    static Transform Translation(float x, float y, float z);

    static Transform Scale(float x, float y, float z);
};


#endif //GPU_PARTICLE_SIM_TRANSFORM_H
