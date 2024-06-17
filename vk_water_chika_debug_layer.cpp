#include "vk_water_chika_debug_layer.hpp"
#include <string>
#include <string_view>
#include <map>
#include <vector>

struct type_next {
    VkStructureType sType;
    type_next* pNext;
};

const void* find_structure(const void* chain, VkStructureType sType) {
    for (auto pNext = reinterpret_cast<const type_next*>(chain); pNext != nullptr; pNext = pNext->pNext) {
        if (pNext->sType == sType) {
            return reinterpret_cast<const void*>(pNext);
        }
    }
    return nullptr;
}

const VkLayerInstanceCreateInfo* get_chain_info(const void* chain, VkStructureType sType, VkLayerFunction function) {
    for (auto pNext = reinterpret_cast<const type_next*>(chain); pNext != nullptr; pNext = pNext->pNext) {
        if (pNext->sType == sType &&
            reinterpret_cast<const VkLayerInstanceCreateInfo*>(pNext)->function == function) {
            return reinterpret_cast<const VkLayerInstanceCreateInfo*>(pNext);
        }
    }
    return nullptr;
}
class water_chika_debug_layer;

class water_chika_debug_device_layer {
public:
    static void add_device(
        water_chika_debug_layer* instance_layer,
        VkPhysicalDevice physical_device,
        VkDevice device,
        PFN_vkGetDeviceProcAddr get_device_proc_addr) {
        water_chika_debug_device_layer device_layer{ instance_layer, physical_device, device, get_device_proc_addr };
        g_devices.emplace(device, std::move(device_layer));
    }
    water_chika_debug_device_layer() = default;
    water_chika_debug_device_layer(const water_chika_debug_device_layer&) = delete;
    water_chika_debug_device_layer(water_chika_debug_device_layer&&) = default;
    water_chika_debug_device_layer& operator=(const water_chika_debug_device_layer&) = delete;
    water_chika_debug_device_layer& operator=(water_chika_debug_device_layer&&) = default;
    ~water_chika_debug_device_layer() = default;
    water_chika_debug_device_layer(
        water_chika_debug_layer* instance_layer, const VkPhysicalDevice physical_device, VkDevice device,
        PFN_vkGetDeviceProcAddr get_device_proc_addr)
        :m_instance_layer{ instance_layer }, m_physical_device{ physical_device }, m_device{ device },
        m_get_next_device_proc_addr{ get_device_proc_addr } {
    }
    PFN_vkVoidFunction GetDeviceProcAddr(const char* pName) {
        return get_next_device_proc_addr(pName);
    }
    static PFN_vkVoidFunction GetDeviceProcAddr(
        VkDevice device, const char* pName
    ) {
        auto& layer = g_devices[device];
        return layer.GetDeviceProcAddr(pName);
    }
    PFN_vkVoidFunction get_next_device_proc_addr(const char* pName) {
        if (m_dispatch_table.contains(pName)) {
            return reinterpret_cast<PFN_vkVoidFunction>(m_dispatch_table[pName]);
        }
        auto procAddr = m_get_next_device_proc_addr(m_device, pName);
        m_dispatch_table.emplace(pName, procAddr);
        return procAddr;
    }
    VkResult MapMemory(
        VkDeviceMemory memory,
        VkDeviceSize offset,
        VkDeviceSize size,
        VkMemoryMapFlags flags,
        void** ppData
    ) {
        auto nextMapMemory = reinterpret_cast<PFN_vkMapMemory>(get_next_device_proc_addr("vkMapMemory"));
        return nextMapMemory(m_device, memory, offset, size, flags, ppData);
    }
    static VkResult MapMemory(
        VkDevice device,
        VkDeviceMemory memory,
        VkDeviceSize offset,
        VkDeviceSize size,
        VkMemoryMapFlags flags,
        void** ppData
    ) {
        auto& layer = g_devices[device];
        return layer.MapMemory(memory, offset, size, flags, ppData);
    }
    void UnmapMemory(VkDeviceMemory memory) {
        auto nextUnmapMemory = reinterpret_cast<PFN_vkUnmapMemory>(get_next_device_proc_addr("vkUnmapMemory"));
        return nextUnmapMemory(m_device, memory);
    }
    static void UnmapMemory(VkDevice device, VkDeviceMemory memory) {
        auto& layer = g_devices[device];
        return layer.UnmapMemory(memory);
    }
    VkResult FlushMappedMemoryRanges(
        uint32_t memoryRangeCount,
        const VkMappedMemoryRange* pMemoryRanges
    ) {
        auto nextFlushMappedMemoryRanges = reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(get_next_device_proc_addr("vkFlushMappedMemoryRanges"));
        return nextFlushMappedMemoryRanges(m_device, memoryRangeCount, pMemoryRanges);
    }
    static VkResult FlushMappedMemoryRanges(
        VkDevice device,
        uint32_t memoryRangeCount,
        const VkMappedMemoryRange* pMemoryRanges
    ) {
        auto& layer = g_devices[device];
        return layer.FlushMappedMemoryRanges(memoryRangeCount, pMemoryRanges);
    }
    VkResult InvalidateMappedMemoryRanges(
        uint32_t memoryRangeCount,
        const VkMappedMemoryRange* pMemoryRanges
    ) {
        auto nextInvalidateMappedMemoryRanges = reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(get_next_device_proc_addr("vkInvalidateMappedMemoryRanges"));
        return nextInvalidateMappedMemoryRanges(m_device, memoryRangeCount, pMemoryRanges);
    }
    static VkResult InvalidateMappedMemoryRanges(
        VkDevice device,
        uint32_t memoryRangeCount,
        const VkMappedMemoryRange* pMemoryRanges
    ) {
        auto& layer = g_devices[device];
        return layer.InvalidateMappedMemoryRanges(memoryRangeCount, pMemoryRanges);
    }
private:
    inline static std::map<VkDevice, water_chika_debug_device_layer> g_devices;
    water_chika_debug_layer* m_instance_layer;
    VkPhysicalDevice m_physical_device;
    VkDevice m_device;
    PFN_vkGetDeviceProcAddr m_get_next_device_proc_addr;
    std::map<std::string, void*> m_dispatch_table;
};
class water_chika_debug_layer {
public:

    static VkResult CreateInstance(const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance) {
        auto chain_info = reinterpret_cast<const VkLayerInstanceCreateInfo*>(get_chain_info(pCreateInfo, VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO, VK_LAYER_LINK_INFO));

        water_chika_debug_layer layer{};

        layer.m_get_next_instance_proc_addr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;



        auto nextCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(layer.get_next_instance_proc_addr("vkCreateInstance"));

        VkResult res = nextCreateInstance(pCreateInfo, pAllocator, pInstance);
        if (res == VK_SUCCESS) {
            layer.m_instance = *pInstance;
            g_instances.emplace(*pInstance, layer);
        }
        return res;
    }
    static VkResult EnumeratePhysicalDevices(
        VkInstance instance,
        uint32_t* pPhysicalDeviceCount,
        VkPhysicalDevice* pPhysicalDevices
    ) {
        water_chika_debug_layer& layer = g_instances[instance];
        return layer.EnumeratePhysicalDevices(pPhysicalDeviceCount, pPhysicalDevices);
    }
    VkResult EnumeratePhysicalDevices(
        uint32_t* pPhysicalDeviceCount,
        VkPhysicalDevice* pPhysicalDevices
    ) {
        auto nextEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(get_next_instance_proc_addr("vkEnumeratePhysicalDevices"));
        auto res = nextEnumeratePhysicalDevices(m_instance, pPhysicalDeviceCount, pPhysicalDevices);
        if (pPhysicalDevices == nullptr) {
            return res;
        }
        if (res == VK_SUCCESS || res == VK_INCOMPLETE) {
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                g_physical_devices.emplace(pPhysicalDevices[i], m_instance);
            }
        }
        return res;
    }
    VkResult create_device(
        const VkPhysicalDevice physical_device,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice
    ) {
        auto nextCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(get_next_instance_proc_addr("vkCreateDevice"));
        auto res = nextCreateDevice(physical_device, pCreateInfo, pAllocator, pDevice);
        auto device_chain_info = reinterpret_cast<const VkLayerDeviceCreateInfo*>(get_chain_info(pCreateInfo, VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO, VK_LAYER_LINK_INFO));
        m_get_next_device_proc_addr = device_chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
        if (res == VK_SUCCESS) {
            water_chika_debug_device_layer::add_device(
                this,
                physical_device,
                *pDevice,
                m_get_next_device_proc_addr);
        }
        return res;
    }
    static VkResult CreateDevice(
        const VkPhysicalDevice physical_device,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice
    ) {
        VkInstance instance = g_physical_devices[physical_device];
        auto& layer = g_instances[instance];

        return layer.create_device(physical_device, pCreateInfo, pAllocator, pDevice);
    }

    static PFN_vkVoidFunction GetInstanceProcAddr(
        VkInstance instance, const char* pName
    ) {
        water_chika_debug_layer& layer = g_instances[instance];
        return reinterpret_cast<PFN_vkVoidFunction>(layer.get_next_instance_proc_addr(pName));
    }
    PFN_vkVoidFunction get_next_instance_proc_addr(const char* pName) {
        if (m_dispatch_table.contains(pName)) {
            return reinterpret_cast<PFN_vkVoidFunction>(m_dispatch_table[pName]);
        }
        auto procAddr = m_get_next_instance_proc_addr(m_instance, pName);
        m_dispatch_table.emplace(pName, procAddr);
        return procAddr;
    }
private:
    inline static std::map<VkInstance, water_chika_debug_layer> g_instances;
    inline static std::map<VkPhysicalDevice, VkInstance> g_physical_devices;
    VkInstance m_instance;
    PFN_vkGetInstanceProcAddr m_get_next_instance_proc_addr;
    std::map<std::string, void*> m_dispatch_table;
    PFN_vkGetDeviceProcAddr m_get_next_device_proc_addr;
};


extern "C" __declspec(dllexport) VkResult VKAPI_CALL water_chika_debug_layer_CreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance
) {
    return water_chika_debug_layer::CreateInstance(pCreateInfo, pAllocator, pInstance);
}

extern "C" __declspec(dllexport) VkResult VKAPI_CALL water_chika_debug_layer_CreateDevice(
    const VkPhysicalDevice physical_device,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pInstance
) {
    return water_chika_debug_layer::CreateDevice(physical_device, pCreateInfo, pAllocator, pInstance);
}

extern "C" __declspec(dllexport) VkResult VKAPI_CALL water_chika_debug_layer_MapMemory(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData
) {

    return water_chika_debug_device_layer::MapMemory(device, memory, offset, size, flags, ppData);
}

extern "C" __declspec(dllexport) void VKAPI_CALL water_chika_debug_layer_UnmapMemory(
    VkDevice device,
    VkDeviceMemory memory
) {
    return water_chika_debug_device_layer::UnmapMemory(device, memory);
}

extern "C" __declspec(dllexport) VkResult water_chika_debug_layer_EnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t * pPhysicalDeviceCount,
    VkPhysicalDevice * pPhysicalDevices
) {
    return water_chika_debug_layer::EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}
extern "C" __declspec(dllexport) VkResult water_chika_debug_layer_FlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    const VkMappedMemoryRange * pMemoryRanges
) {
    return water_chika_debug_device_layer::FlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

auto get_device_layer_procs() {
    std::map<std::string, void*> funcs{
        {"vkGetDeviceProcAddr", water_chika_debug_layer_GetDeviceProcAddr},
        {"vkMapMemory", water_chika_debug_layer_MapMemory},
        {"vkUnmapMemory", water_chika_debug_layer_UnmapMemory},
        {"vkFlushMappedMemoryRanges", water_chika_debug_layer_FlushMappedMemoryRanges},
    };
    return funcs;
}
auto get_instance_layer_procs() {
    std::map<std::string, void*> funcs{
    {"vkGetInstanceProcAddr", water_chika_debug_layer_GetInstanceProcAddr},
    {"vkCreateInstance", water_chika_debug_layer_CreateInstance},
    {"vkEnumeratePhysicalDevices", water_chika_debug_layer_EnumeratePhysicalDevices},
    {"vkGetDeviceProcAddr", water_chika_debug_layer_GetDeviceProcAddr},
    {"vkCreateDevice", water_chika_debug_layer_CreateDevice},
    };
    return funcs;
}

extern "C" __declspec(dllexport) PFN_vkVoidFunction VKAPI_CALL water_chika_debug_layer_GetDeviceProcAddr(
    VkDevice device, const char* pName
) {
    auto funcs = get_device_layer_procs();
    if (funcs.contains(pName)) {
        return reinterpret_cast<PFN_vkVoidFunction>(funcs[pName]);
    }
    return water_chika_debug_device_layer::GetDeviceProcAddr(device, pName);
}



extern "C" __declspec(dllexport) PFN_vkVoidFunction VKAPI_CALL water_chika_debug_layer_GetInstanceProcAddr(
    VkInstance instance, const char* pName
) {
    auto funcs = get_instance_layer_procs();
    if (funcs.contains(pName)) {
        return reinterpret_cast<PFN_vkVoidFunction>(funcs[pName]);
    }
    return water_chika_debug_layer::GetInstanceProcAddr(instance, pName);
}

extern "C" __declspec(dllexport) VkResult VKAPI_CALL water_chika_debug_layer_NegotiateLoaderLayerInterfaceVersion(
    VkNegotiateLayerInterface* pVersionStruct
){
    pVersionStruct->loaderLayerInterfaceVersion;
    pVersionStruct->pfnGetInstanceProcAddr = water_chika_debug_layer_GetInstanceProcAddr;
    pVersionStruct->pfnGetDeviceProcAddr = water_chika_debug_layer_GetDeviceProcAddr;
    return VK_SUCCESS;
}

