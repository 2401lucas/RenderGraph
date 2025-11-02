//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_FENCE_H
#define GPU_PARTICLE_SIM_FENCE_H

class Fence {
    public:
    virtual ~Fence() = default;

    /// Signal the fence from the GPU
    virtual void Signal(CommandQueue* queue, uint64_t value) = 0;

    /// CPU wait until fence reaches `value`
    virtual void WaitCPU(uint64_t value) = 0;

    /// Check current completed fence value
    virtual uint64_t GetCompletedValue() const = 0;

    /// Reset fence to initial value (optional)
    virtual void Reset(uint64_t value = 0) = 0;
};

#endif //GPU_PARTICLE_SIM_FENCE_H