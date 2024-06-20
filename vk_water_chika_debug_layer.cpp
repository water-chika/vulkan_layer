#include "vk_water_chika_debug_layer.hpp"
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <span>
#include <algorithm>
#include <set>

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
    water_chika_debug_device_layer(
        water_chika_debug_layer* instance_layer, const VkPhysicalDevice physical_device, VkDevice device,
        PFN_vkGetDeviceProcAddr get_device_proc_addr)
        :m_instance_layer{ instance_layer }, m_physical_device{ physical_device }, m_device{ device },
        m_get_next_device_proc_addr{ get_device_proc_addr } {
    }
    ~water_chika_debug_device_layer() = default;
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
        auto res = nextMapMemory(m_device, memory, offset, size, flags, ppData);
        return res;
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
    VkResult CreateGraphicsPipelines(
        VkPipelineCache pipelineCache,
        uint32_t createInfoCount,
        const VkGraphicsPipelineCreateInfo* pCreateInfos,
        const VkAllocationCallbacks* pAllocator,
        VkPipeline* pPipeline
    ) {
        auto nextCreateGraphicsPipelines = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(get_next_device_proc_addr("vkCreateGraphicsPipelines"));
        return nextCreateGraphicsPipelines(m_device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipeline);
    }
    static VkResult CreateGraphicsPipelines(
        VkDevice device,
        VkPipelineCache pipelineCache,
        uint32_t createInfoCount,
        const VkGraphicsPipelineCreateInfo* pCreateInfos,
        const VkAllocationCallbacks* pAllocator,
        VkPipeline* pPipeline
    ) {
        auto& layer = g_devices[device];

        return layer.CreateGraphicsPipelines(pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipeline);
    }
    VkResult CreateShaderModule(
        const VkShaderModuleCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkShaderModule* pShaderModule
    ) {
        auto nextCreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(get_next_device_proc_addr("vkCreateShaderModule"));
        auto res = nextCreateShaderModule(m_device, pCreateInfo, pAllocator, pShaderModule);
        return res;
    }
    static VkResult CreateShaderModule(
        VkDevice device,
        const VkShaderModuleCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkShaderModule* pShaderModule
    ) {
        auto& layer = g_devices[device];
        auto res = layer.CreateShaderModule(pCreateInfo, pAllocator, pShaderModule);
        return res;
    }
    void GetDeviceQueue(uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue) {
        auto nextGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(get_next_device_proc_addr("vkGetDeviceQueue"));
        nextGetDeviceQueue(m_device, queueFamilyIndex, queueIndex, pQueue);
        g_queues.emplace(*pQueue, m_device);
    }
    static void GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue) {
        auto& layer = g_devices[device];
        return layer.GetDeviceQueue(queueFamilyIndex, queueIndex, pQueue);
    }
    VkResult queue_present(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
        auto nextQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(get_next_device_proc_addr("vkQueuePresentKHR"));
        //auto nextWaitQueueIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(get_next_device_proc_addr("vkQueueWaitIdle"));
        auto res = nextQueuePresentKHR(queue, pPresentInfo);
        //nextWaitQueueIdle(queue);
        return res;
    }
    static VkResult QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
        auto device = g_queues[queue];
        auto& layer = g_devices[device];
        return layer.queue_present(queue, pPresentInfo);
    }
    VkResult AllocateCommandBuffers(
        const VkCommandBufferAllocateInfo* pAllocateInfo,
        VkCommandBuffer* pCommandBuffers
    ) {
        auto nextAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(get_next_device_proc_addr("vkAllocateCommandBuffers"));
        auto res = nextAllocateCommandBuffers(m_device, pAllocateInfo, pCommandBuffers);
        if (res == VK_SUCCESS) {
            for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
                g_command_buffers.emplace(pCommandBuffers[i], m_device);
            }
        }
        return res;
    }
    static VkResult AllocateCommandBuffers(
        VkDevice device,
        const VkCommandBufferAllocateInfo* pAllocateInfo,
        VkCommandBuffer* pCommandBuffers
    ) {
        auto& layer = g_devices[device];
        return layer.AllocateCommandBuffers(pAllocateInfo, pCommandBuffers);
    }
    void cmd_draw_indexed(
        VkCommandBuffer command_buffer,
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t first_index,
        int32_t vertex_offset,
        uint32_t first_instance
    ) {
        auto nextCmdDrawIndexed = reinterpret_cast<PFN_vkCmdDrawIndexed>(get_next_device_proc_addr("vkCmdDrawIndexed"));
        /* {
            auto nextCmdPipelineBarrier = reinterpret_cast<PFN_vkCmdPipelineBarrier>(get_next_device_proc_addr("vkCmdPipelineBarrier"));
            VkMemoryBarrier memory_barrier{
                .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
            };
            nextCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT ,
                0, 1, &memory_barrier, 0, nullptr, 0, nullptr);
        }*/
        nextCmdDrawIndexed(command_buffer, indexCount, instanceCount, first_index, vertex_offset, first_instance);
    }
    static void CmdDrawIndexed(
        VkCommandBuffer command_buffer,
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t first_index,
        int32_t vertex_offset,
        uint32_t first_instance
    ) {
        auto device = g_command_buffers[command_buffer];
        auto& layer = g_devices[device];
        layer.cmd_draw_indexed(command_buffer, indexCount, instanceCount, first_index, vertex_offset, first_instance);
    }
    VkResult queue_submit(
        VkQueue queue,
        uint32_t submit_count,
        const VkSubmitInfo* pSubmits,
        VkFence fence
    ) {
        auto nextQueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(get_next_device_proc_addr("vkQueueSubmit"));
        return nextQueueSubmit(queue, submit_count, pSubmits, fence);
    }
    static VkResult QueueSubmit(
        VkQueue queue,
        uint32_t submitCount,
        const VkSubmitInfo* pSubmits,
        VkFence fence
    ) {
        auto device = g_queues[queue];
        auto& layer = g_devices[device];
        return layer.queue_submit(queue, submitCount, pSubmits, fence);
    }
    VkResult AllocateMemory(
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory
    ) {
        auto nextAllocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(get_next_device_proc_addr("vkAllocateMemory"));
        auto res = nextAllocateMemory(m_device, pAllocateInfo, pAllocator, pMemory);
        return res;
    }
    static VkResult AllocateMemory(
        VkDevice device,
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory
    ) {
        auto& layer = g_devices[device];
        return layer.AllocateMemory(pAllocateInfo, pAllocator, pMemory);
    }
    void FreeMemory(
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator
    ) {
        auto nextFreeMemory = reinterpret_cast<PFN_vkFreeMemory>(get_next_device_proc_addr("vkFreeMemory"));
        nextFreeMemory(m_device, memory, pAllocator);
    }
    static void FreeMemory(
        VkDevice device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator
    ) {
        auto& layer = g_devices[device];
        layer.FreeMemory(memory, pAllocator);
    }
    VkResult BindBufferMemory(
        VkBuffer buffer,
        VkDeviceMemory memory,
        VkDeviceSize memoryOffset
    ) {
        auto nextBindBufferMemory = reinterpret_cast<PFN_vkBindBufferMemory>(get_next_device_proc_addr("vkBindBufferMemory"));
        auto res = nextBindBufferMemory(m_device, buffer, memory, memoryOffset);
        return res;
    }
    static VkResult BindBufferMemory(
        VkDevice device,
        VkBuffer buffer,
        VkDeviceMemory memory,
        VkDeviceSize memoryOffset) {
        auto& layer = g_devices[device];
        return layer.BindBufferMemory(buffer, memory, memoryOffset);
    }
    void cmd_bind_index_buffer(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkDeviceSize offset,
        VkIndexType indexType
        ) {
        auto nextCmdBindIndexBuffer = reinterpret_cast<PFN_vkCmdBindIndexBuffer>(get_next_device_proc_addr("vkCmdBindIndexBuffer"));
        return nextCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
    }
    static void CmdBindIndexBuffer(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkDeviceSize offset,
        VkIndexType indexType
    ) {
        auto device = g_command_buffers[commandBuffer];
        auto& layer = g_devices[device];
        return layer.cmd_bind_index_buffer(commandBuffer, buffer, offset, indexType);
    }
    void cmd_bind_pipeline(
        VkCommandBuffer command_buffer,
        VkPipelineBindPoint pipeline_bind_point,
        VkPipeline pipeline) {
        auto nextBindPipeline = reinterpret_cast<PFN_vkCmdBindPipeline>(get_next_device_proc_addr("vkCmdBindPipeline"));
        nextBindPipeline(command_buffer, pipeline_bind_point, pipeline);
    }
    static void CmdBindPipeline(
        VkCommandBuffer command_buffer,
        VkPipelineBindPoint pipeline_bind_point,
        VkPipeline pipeline) {
        auto device = g_command_buffers[command_buffer];
        auto& layer = g_devices[device];
        return layer.cmd_bind_pipeline(command_buffer, pipeline_bind_point, pipeline);
    }
    void cmd_bind_vertex_buffers(
        VkCommandBuffer command_buffer,
        uint32_t first_binding,
        uint32_t binding_count,
        const VkBuffer* pBuffers,
        const VkDeviceSize* pOffsets
    ) {
        auto nextCmdBindVertexBuffers = reinterpret_cast<PFN_vkCmdBindVertexBuffers>(get_next_device_proc_addr("vkCmdBindVertexBuffers"));
        return nextCmdBindVertexBuffers(command_buffer, first_binding, binding_count, pBuffers, pOffsets);
    }
    static void CmdBindVertexBuffers(
        VkCommandBuffer command_buffer,
        uint32_t first_binding,
        uint32_t binding_count,
        const VkBuffer* pBuffers,
        const VkDeviceSize* pOffsets
    ) {
        auto device = g_command_buffers[command_buffer];
        auto& layer = g_devices[device];
        return layer.cmd_bind_vertex_buffers(command_buffer, first_binding, binding_count, pBuffers, pOffsets);
    }
private:
    inline static std::map<VkDevice, water_chika_debug_device_layer> g_devices;
    inline static std::map<VkQueue, VkDevice> g_queues;
    inline static std::map<VkCommandBuffer, VkDevice> g_command_buffers;
    water_chika_debug_layer* m_instance_layer;
    VkPhysicalDevice m_physical_device;
    VkDevice m_device;
    PFN_vkGetDeviceProcAddr m_get_next_device_proc_addr;
    std::map<std::string, void*> m_dispatch_table;
};

// Vulkan Instance Layer
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
    void get_physical_device_memory_properties(
        VkPhysicalDevice physical_device,
        VkPhysicalDeviceMemoryProperties* pMemoryProperties
    ) {
        auto nextGetPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(get_next_instance_proc_addr("vkGetPhysicalDeviceMemoryProperties"));
        nextGetPhysicalDeviceMemoryProperties(physical_device, pMemoryProperties);
        for (uint32_t i = 0; i < pMemoryProperties->memoryTypeCount; i++) {
            //pMemoryProperties->memoryTypes[i].propertyFlags &= ~VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }
    }
    static void GetPhysicalDeviceMemoryProperties(
        VkPhysicalDevice physical_device,
        VkPhysicalDeviceMemoryProperties* pMemoryProperties
    ) {
        auto instance = g_physical_devices[physical_device];
        auto& layer = g_instances[instance];
        layer.get_physical_device_memory_properties(physical_device, pMemoryProperties);
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

extern "C" __declspec(dllexport) VkResult VKAPI_CALL water_chika_debug_layer_EnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t * pPhysicalDeviceCount,
    VkPhysicalDevice * pPhysicalDevices
) {
    return water_chika_debug_layer::EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}
extern "C" __declspec(dllexport) VkResult VKAPI_CALL water_chika_debug_layer_FlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    const VkMappedMemoryRange * pMemoryRanges
) {
    return water_chika_debug_device_layer::FlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

extern "C" __declspec(dllexport) void VKAPI_CALL water_chika_debug_layer_GetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physical_device,
    VkPhysicalDeviceMemoryProperties * pMemoryProperties
) {
    return water_chika_debug_layer::GetPhysicalDeviceMemoryProperties(physical_device, pMemoryProperties);
}

extern "C" __declspec(dllexport) VkResult VKAPI_CALL water_chika_debug_layer_InvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    const VkMappedMemoryRange * pMemoryRanges
) {
    return water_chika_debug_device_layer::InvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

extern "C" __declspec(dllexport) VkResult VKAPI_CALL water_chika_debug_layer_CreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo * pCreateInfos,
    const VkAllocationCallbacks * pAllocator,
    VkPipeline * pPipeline
) {
    return water_chika_debug_device_layer::CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipeline);
}

extern "C" __declspec(dllexport)VkResult VKAPI_CALL water_chika_debug_layer_CreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo * pCreateInfo,
    const VkAllocationCallbacks * pAllocator,
    VkShaderModule * pShaderModule
) {
    return water_chika_debug_device_layer::CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}
extern "C" __declspec(dllexport) VkResult VKAPI_CALL water_chika_debug_layer_QueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR * pPresentInfo
) {
    return water_chika_debug_device_layer::QueuePresentKHR(queue, pPresentInfo);
}
extern "C" __declspec(dllexport) void VKAPI_CALL water_chika_debug_layer_GetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue * pQueue
) {
    return water_chika_debug_device_layer::GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}
extern "C" __declspec(dllexport) VkResult VKAPI_CALL water_chika_debug_layer_AllocateCommandBuffers(
    VkDevice device,
    const VkCommandBufferAllocateInfo * pAllocateInfo,
    VkCommandBuffer * pCommandBuffers
) {
    return water_chika_debug_device_layer::AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

void VKAPI_CALL water_chika_debug_layer_CmdDrawIndexed(
    VkCommandBuffer commandBuffer,
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex,
    int32_t vertexOffset,
    uint32_t firstInstance
) {
    return water_chika_debug_device_layer::CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VkResult VKAPI_CALL water_chika_debug_layer_QueueSubmit(
    VkQueue queue,
    uint32_t submitCount,
    const VkSubmitInfo* pSubmits,
    VkFence fence
) {
    return water_chika_debug_device_layer::QueueSubmit(queue, submitCount, pSubmits, fence);
}

VkResult VKAPI_CALL water_chika_debug_layer_AllocateMemory(
    VkDevice device,
    const VkMemoryAllocateInfo* pAllocateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDeviceMemory* pMemory
) {
    return water_chika_debug_device_layer::AllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}
void VKAPI_CALL water_chika_debug_layer_FreeMemory(
    VkDevice device,
    VkDeviceMemory memory,
    const VkAllocationCallbacks* pAllocator
) {
    return water_chika_debug_device_layer::FreeMemory(device, memory, pAllocator);
}

VkResult VKAPI_CALL water_chika_debug_layer_BindBufferMemory(
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory memory,
    VkDeviceSize memoryOffset) {
    return water_chika_debug_device_layer::BindBufferMemory(device, buffer, memory, memoryOffset);
}

void VKAPI_CALL water_chika_debug_layer_CmdBindIndexBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkIndexType indexType
) {
    return water_chika_debug_device_layer::CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

void VKAPI_CALL water_chika_debug_layer_CmdBindPipeline(
    VkCommandBuffer commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline pipeline
) {
    return water_chika_debug_device_layer::CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

void VKAPI_CALL water_chika_debug_layer_CmdBindVertexBuffers(
    VkCommandBuffer commandBuffer,
    uint32_t firstBinding,
    uint32_t bindingCount,
    const VkBuffer* pBuffers,
    const VkDeviceSize* pOffsets
) {
    return water_chika_debug_device_layer::CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

auto get_device_layer_procs() {
    std::map<std::string, void*> funcs{
        {"vkGetDeviceProcAddr", water_chika_debug_layer_GetDeviceProcAddr},
        {"vkMapMemory", water_chika_debug_layer_MapMemory},
        {"vkUnmapMemory", water_chika_debug_layer_UnmapMemory},
        {"vkFlushMappedMemoryRanges", water_chika_debug_layer_FlushMappedMemoryRanges},
        //{"vkInvalidateMappedMemoryRanges", water_chika_debug_layer_InvalidateMappedMemoryRanges},
        {"vkCreateGraphicsPipelines", water_chika_debug_layer_CreateGraphicsPipelines},
        {"vkCreateShaderModule", water_chika_debug_layer_CreateShaderModule },
        {"vkGetDeviceQueue", water_chika_debug_layer_GetDeviceQueue},
        {"vkQueuePresentKHR", water_chika_debug_layer_QueuePresentKHR},
        {"vkAllocateCommandBuffers", water_chika_debug_layer_AllocateCommandBuffers},
        {"vkQueueSubmit", water_chika_debug_layer_QueueSubmit},
        {"vkAllocateMemory", water_chika_debug_layer_AllocateMemory},
        {"vkFreeMemory", water_chika_debug_layer_FreeMemory},
        {"vkBindBufferMemory", water_chika_debug_layer_BindBufferMemory},
        {"vkCmdBindIndexBuffer", water_chika_debug_layer_CmdBindIndexBuffer},
        {"vkCmdDrawIndexed", water_chika_debug_layer_CmdDrawIndexed},
        {"vkCmdBindPipeline", water_chika_debug_layer_CmdBindPipeline},
        {"vkCmdBindVertexBuffers", water_chika_debug_layer_CmdBindVertexBuffers},
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
        {"vkGetPhysicalDeviceMemoryProperties", water_chika_debug_layer_GetPhysicalDeviceMemoryProperties},
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

