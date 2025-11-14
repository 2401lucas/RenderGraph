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

    /// <summary>
    /// Executes a command list
    /// </summary>
    virtual void Execute(CommandList *commandList) = 0;

    /// <summary>
    /// Waits for a Queue to become Idle (D3D12 has no Device->WaitIdle() functionality)
    /// </summary>
    virtual void WaitIdle() = 0;

    virtual void Signal(uint64_t fenceValue) = 0;

    virtual void WaitForFence(uint64_t fenceValue) = 0;

    /// <summary>
    /// Wait for frame fence and reset the frame resources
    /// </summary>
    virtual void BeginFrame(uint32_t frameIndex) = 0;

    virtual QueueType GetType() const = 0;

    virtual uint64_t GetCompletedFenceValue() const = 0;

    /// <summary>
    /// Assigns the memory for a CommandList from a Command Queue
    /// </summary>
    /// <remarks>
    /// This is required for DX12 but unsure for vulkan, so the vulkan impl will be empty
    /// TODO: Investigate this
    /// </remarks>
    virtual void AssignCommandList(CommandList *, uint32_t) = 0;
};
#endif //GPU_PARTICLE_SIM_COMMANDQUEUE_H
