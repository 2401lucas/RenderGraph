//
// Created by 2401Lucas on 2025-11-04.
//

#include "D3D12Pipeline.h"

void *D3D12Pipeline::GetNativeHandle() const {
    return pso.Get();
}
