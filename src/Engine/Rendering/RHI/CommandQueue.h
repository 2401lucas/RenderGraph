//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_COMMANDQUEUE_H
#define GPU_PARTICLE_SIM_COMMANDQUEUE_H
#include "CommandList.h"


enum class QueueType {
    Graphics, // Can do graphics, compute, and copy
    Compute, // Can do compute and copy
    Transfer // Can only do copy operations
};

struct CommandQueueCreateInfo {
    QueueType type = QueueType::Graphics;
    const char *debugName = nullptr;
};

class CommandQueue {
public:
    virtual ~CommandQueue() = default;

    virtual void Execute(CommandList *commandList) = 0;

    virtual void WaitIdle() = 0;

    virtual void Signal(uint64_t fenceValue) = 0;

    virtual void WaitForFence(uint64_t fenceValue) = 0;

    virtual void BeginFrame(uint32_t frameIndex) = 0;

    virtual QueueType GetType() const = 0;

    virtual void AssignCommandList(CommandList *, uint32_t) = 0;
};
#endif //GPU_PARTICLE_SIM_COMMANDQUEUE_H
