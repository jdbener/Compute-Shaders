#ifndef __BOILER_PLATE_VULK_H__
#define __BOILER_PLATE_VULK_H__

#include "VulkanWrapper.hpp"

// Temporary
#include "../dictionary.hpp"
#include <fstream>

// Struct storing all of the general purpose vulkan handles
struct VulkanContext {
    vk::UniqueInstance instance;
    vk::UniqueDebugUtilsMessengerEXT debugMsgr;
    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice device;
    uint32_t computeQueueIndex = -1;
    vk::Queue computeQueue;
    vk::UniqueCommandPool commandPool;
};

static VulkanContext initVulkan(){
    VulkanContext out;

    // Determine required vulkan extensions and layers
    std::vector<const char*> layers = validateInstanceLayers( std::vector<const char*>{"VK_LAYER_KHRONOS_validation"} );
    std::vector<const char*> extens = validateInstanceExtensions( std::vector<const char*>{VK_EXT_DEBUG_UTILS_EXTENSION_NAME});
    // Create Vulkan Instance
    vk::ApplicationInfo appInfo("Compute-Shader", VK_MAKE_VERSION(1, 0, 0), "", VK_MAKE_VERSION(0, 0, 0), VK_API_VERSION_1_2);
    out.instance = vk::createInstanceUnique( {{}, &appInfo, (uint32_t) layers.size(), layers.data(), (uint32_t) extens.size(), extens.data()} );

    // Create Debug Messenger
    using sev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using type = vk::DebugUtilsMessageTypeFlagBitsEXT;
    out.debugMsgr = out.instance->createDebugUtilsMessengerEXTUnique( {{}, /*sev::eVerbose |*/ sev::eWarning | sev::eInfo | sev::eError, type::eGeneral | type::eValidation | type::ePerformance, &debugCallback, nullptr} );

    // Create Vulkan Device
    out.physicalDevice = out.instance->enumeratePhysicalDevices()[0]; // Actual logic to choose a proper device should go here
    // Determine the compute queue index
    auto properties = out.physicalDevice.getQueueFamilyProperties();
    for(uint32_t i = 0; i < properties.size(); i++)
        if(properties[i].queueFlags & (vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer)){
            out.computeQueueIndex = i;
            break;
        }
    float fullPriority = 1;
    vk::DeviceQueueCreateInfo qci({}, out.computeQueueIndex, 1, &fullPriority);
    std::vector<const char*> deviceLayers {};
    std::vector<const char*> deviceExtens {};
    vk::PhysicalDeviceFeatures features {};
    out.device = out.physicalDevice.createDeviceUnique( {{}, 1, &qci, (uint32_t) deviceLayers.size(), deviceLayers.data(), (uint32_t) deviceExtens.size(), deviceExtens.data(), &features} );
    // Refernece the compute queue
    out.computeQueue = out.device->getQueue(out.computeQueueIndex, 0);

    // Create a command pool
    out.commandPool = out.device->createCommandPoolUnique( {{}, out.computeQueueIndex} );

    return out;
}

#endif /* end of include guard: __BOILER_PLATE_VULK_H__ */
