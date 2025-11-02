//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_COMMANDQUEUE_H
#define GPU_PARTICLE_SIM_COMMANDQUEUE_H
#include "CommandList.h"

class CommandQueue {
public:
    virtual ~CommandQueue() = default;

    // Execute command lists
    virtual void Execute(CommandList **commandLists, uint32_t count) = 0;

    // Synchronization
    virtual void Signal(uint64_t signalValue) = 0;

    virtual void WaitForFence(uint64_t fenceValue) = 0;

    virtual uint64_t GetCompletedFenceValue() const = 0;

    virtual void WaitIdle() = 0;

    // Convenience
    void Execute(CommandList *commandList) {
        Execute(&commandList, 1);
    }
};
#endif //GPU_PARTICLE_SIM_COMMANDQUEUE_H
