///
/// Project: Bazel_Vulkan_Metal
/// File: vulkan.h
/// Date: 8/18/2024
/// Description:
/// Author: Pruvost Kevin | pruvostkevin (pruvostkevin0@gmail.com)
///
#pragma once

#include "VulkanDebug.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanQueueFamily.h"

#include <common/Application.h>
#include <common/Context.h>

namespace venom
{

class VulkanApplication
    : public ApplicationBackend
    , public VulkanDebugApplication
{
public:
    VulkanApplication();
    ~VulkanApplication();
    Error Run() override;

private:
    Error __Loop();
    Error __InitVulkan();

    Error __InitPhysicalDevices();

    Error __CreateInstance();
    void __Instance_GetRequiredExtensions(VkInstanceCreateInfo * createInfo);

    std::vector<const char *> __instanceExtensions;
    Context __context;
    VulkanPhysicalDevice __physicalDevice;
};
}

extern "C" EXPORT venom::ApplicationBackend* createApplication();