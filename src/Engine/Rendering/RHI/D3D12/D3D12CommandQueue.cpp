//
// Created by 2401Lucas on 2025-11-04.
//

#include "D3D12CommandQueue.h"

#include "D3D12CommandList.h"

D3D12CommandQueue::~D3D12CommandQueue() {
    // Wait for GPU to finish before destroying resources
    if (m_fence && m_commandQueue) {
        WaitIdle();
    }

    // Close fence event handle
    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
}

void D3D12CommandQueue::BeginFrame(uint32_t frameIndex) {
    m_currentFrameIndex = frameIndex;

    // Wait for this specific frame to complete
    uint64_t fenceValue = m_fenceValues[frameIndex];
    if (fenceValue > 0 && m_fence->GetCompletedValue() < fenceValue) {
        WaitForFence(fenceValue);
    }

    // Reset allocator for this frame (safe now that GPU is done with it)
    DX_CHECK(m_allocators[frameIndex]->Reset());
}

void D3D12CommandQueue::Execute(CommandList *commandList) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    ID3D12CommandList *lists[] = {d3d12CommandList->GetNative()};
    m_commandQueue->ExecuteCommandLists(1, lists);
}

void D3D12CommandQueue::Signal(uint64_t fenceValue) {
    DX_CHECK(m_commandQueue->Signal(m_fence.Get(), fenceValue));
    m_fenceValues[m_currentFrameIndex] = fenceValue;
    m_nextFenceValue = fenceValue + 1;
}

void D3D12CommandQueue::WaitForFence(uint64_t fenceValue) {
    if (m_fence->GetCompletedValue() < fenceValue) {
        DX_CHECK(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void D3D12CommandQueue::WaitIdle() {
    // Signal a new fence value and wait for it
    uint64_t fenceValue = m_nextFenceValue++;
    DX_CHECK(m_commandQueue->Signal(m_fence.Get(), fenceValue));
    WaitForFence(fenceValue);
}

void D3D12CommandQueue::AssignCommandList(CommandList* cmdList, uint32_t frameIndex) {
    D3D12CommandList* d3d12CmdList = static_cast<D3D12CommandList*>(cmdList);
    d3d12CmdList->SetAllocator(m_allocators[frameIndex].Get());
}

ID3D12CommandAllocator *D3D12CommandQueue::GetAllocator(uint32_t frameIndex) const {
    if (frameIndex >= m_allocators.size()) {
        throw std::out_of_range("Frame index out of range");
    }
    return m_allocators[frameIndex].Get();
}
