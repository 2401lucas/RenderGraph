//
// Created by 2401Lucas on 2025-11-04.
//

#ifndef GPU_PARTICLE_SIM_D3D12COMMANDQUEUE_H
#define GPU_PARTICLE_SIM_D3D12COMMANDQUEUE_H
#include "Rendering/RHI/CommandQueue.h"

#include "D3D12Common.h"


class D3D12CommandQueue : public CommandQueue {
public:
    D3D12CommandQueue() = default;
    ~D3D12CommandQueue() override;

    void BeginFrame(uint32_t frameIndex) override;

    void Execute(CommandList* commandList) override;

    void Signal(uint64_t signalValue) override;

    void WaitForFence(uint64_t fenceValue) override;

    void WaitIdle() override;

    void AssignCommandList(CommandList*, uint32_t) override;

    QueueType GetType() const override { return m_type; }
    ID3D12CommandQueue* GetNative() const { return m_commandQueue.Get(); }
    ID3D12CommandAllocator* GetAllocator(uint32_t frameIndex) const;

private:
    friend class D3D12Device;

    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12Fence> m_fence;
    std::vector<ComPtr<ID3D12CommandAllocator>> m_allocators;
    std::vector<uint64_t> m_fenceValues;

    HANDLE m_fenceEvent = nullptr;
    uint64_t m_nextFenceValue = 1;
    uint32_t m_currentFrameIndex = 0;
    QueueType m_type = QueueType::Graphics;
    D3D12_COMMAND_LIST_TYPE m_d3d12Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
};


#endif //GPU_PARTICLE_SIM_D3D12COMMANDQUEUE_H
