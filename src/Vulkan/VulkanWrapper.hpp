#ifndef __VULKAN_WRAPPER_H__
#define __VULKAN_WRAPPER_H__

#include <vulkan/vulkan.hpp>
#include <iostream>


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static std::vector<const char*> validateInstanceExtensions(std::vector<const char*> in){
    std::vector<const char*> out;
    auto available = vk::enumerateInstanceExtensionProperties();
    for(std::string needle: in){
        bool found = false;
        for(auto haystack: available)
            if(needle == haystack.extensionName){
                found = true;
                break;
            }
        if(!found) {std::cerr << "Extension: " << needle << " not found!"; assert(0); }
    }

    return in;
}

static std::vector<const char*> validateInstanceLayers(std::vector<const char*> in){
    std::vector<const char*> out;
    auto available = vk::enumerateInstanceLayerProperties();
    for(std::string needle: in){
        bool found = false;
        for(auto haystack: available)
            if(needle == haystack.layerName){
                found = true;
                break;
            }
        if(!found) {std::cerr << "Layer: " << needle << " not found!"; assert(0); }
    }

    return in;
}


  /////  Manually Loaded Functions  /////

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger){
    static PFN_vkCreateDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>( vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT") );
	return func(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator){
    static PFN_vkDestroyDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>( vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT") );
	func(instance, messenger, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkSubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData ){
    static PFN_vkSubmitDebugUtilsMessageEXT func = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>( vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT") );
	func(instance, messageSeverity, messageTypes, pCallbackData);
}



#endif /* end of include guard: __VULKAN_WRAPPER_H__ */
