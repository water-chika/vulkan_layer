#pragma once
#include <vulkan/vk_layer.h>

#if WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT PFN_vkVoidFunction VKAPI_CALL water_chika_debug_layer_GetDeviceProcAddr(
    VkDevice device, const char* pName
);
extern "C" DLLEXPORT PFN_vkVoidFunction VKAPI_CALL water_chika_debug_layer_GetInstanceProcAddr(
    VkInstance instance, const char* pName
);
