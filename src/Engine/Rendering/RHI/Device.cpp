//
// Created by 2401Lucas on 2025-10-31.
//

#include "Device.h"

#ifdef RHI_D3D12
#include "D3D12/D3D12Device.h"
#endif

#ifdef RHI_VULKAN
#include "Vulkan/VulkanDevice.h"
#endif


std::unique_ptr<Device> Device::Create(const DeviceCreateInfo &desc) {
    // Choose backend based on platform or config
#ifdef RHI_D3D12
    return std::make_unique<D3D12Device>(desc);
#elif defined(RHI_VULKAN)
    return std::make_unique<VulkanDevice> (desc);
#else
#error "No RHI backend defined"
#endif
}