#pragma once
#include <vulkan/vk_layer.h>



extern "C" __declspec(dllexport) PFN_vkVoidFunction VKAPI_CALL water_chika_debug_layer_GetDeviceProcAddr(
    VkDevice device, const char* pName
);
extern "C" __declspec(dllexport) PFN_vkVoidFunction VKAPI_CALL water_chika_debug_layer_GetInstanceProcAddr(
    VkInstance instance, const char* pName
);