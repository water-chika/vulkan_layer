#include "vk_water_chika_debug_layer.hpp"
#include <string>
#include <string_view>
#include <map>
#include <unordered_map>
#include <vector>
#include <span>
#include <algorithm>
#include <set>
#include <iostream>
#include <cassert>
#include <mutex>

#include <spirv_parser.hpp>

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

VkLayerInstanceCreateInfo* get_chain_info(const void* chain, VkStructureType sType, VkLayerFunction function) {
    for (auto pNext = reinterpret_cast<const type_next*>(chain); pNext != nullptr; pNext = pNext->pNext) {
        if (pNext->sType == sType &&
            reinterpret_cast<const VkLayerInstanceCreateInfo*>(pNext)->function == function) {
            return reinterpret_cast<VkLayerInstanceCreateInfo*>(const_cast<type_next*>(pNext));
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
        m_dispatch_table.emplace(std::string{pName}, (void*)procAddr);
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
    VkResult create_graphics_pipelines(
        VkPipelineCache pipeline_cache,
        uint32_t create_info_count,
        const VkGraphicsPipelineCreateInfo* create_infos,
        const VkAllocationCallbacks* allocator,
        VkPipeline* pipelines
    ) {
        auto nextCreateGraphicsPipelines = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(get_next_device_proc_addr("vkCreateGraphicsPipelines"));
        auto res = nextCreateGraphicsPipelines(m_device, pipeline_cache, create_info_count, create_infos, allocator, pipelines);
        if (VK_SUCCESS == res) {
            for (uint32_t i = 0; i < create_info_count; ++i) {
                pipeline_infos.emplace(
                        pipelines[i],
                        pipeline_info{
                        });
            }
        }
        return res;
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

        return layer.create_graphics_pipelines(pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipeline);
    }
    VkResult create_ray_tracing_pipelines_khr(
        VkDeferredOperationKHR deferred_operation,
        VkPipelineCache pipeline_cache,
        uint32_t create_info_count,
        const VkRayTracingPipelineCreateInfoKHR* create_infos,
        const VkAllocationCallbacks* allocator,
        VkPipeline* pipelines
    ) {
        auto nextCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(get_next_device_proc_addr("vkCreateRayTracingPipelinesKHR"));
        auto res = nextCreateRayTracingPipelinesKHR(m_device, deferred_operation, pipeline_cache, create_info_count, create_infos, allocator, pipelines);
        if (VK_SUCCESS == res) {
            for (uint32_t i = 0; i < create_info_count; ++i) {
                pipeline_infos.emplace(
                        pipelines[i],
                        pipeline_info{
                        });
            }
        }
        return res;
    }
    static VkResult CreateRayTracingPipelinesKHR(
        VkDevice device,
        VkDeferredOperationKHR deferredOperation,
        VkPipelineCache pipelineCache,
        uint32_t createInfoCount,
        const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
        const VkAllocationCallbacks* pAllocator,
        VkPipeline* pPipelines
    ) {
        auto& layer = g_devices[device];
        return layer.create_ray_tracing_pipelines_khr(deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    }
    VkResult create_compute_pipelines(
        VkDevice device,
        VkPipelineCache pipeline_cache,
        uint32_t create_info_count,
        const VkComputePipelineCreateInfo* create_infos,
        const VkAllocationCallbacks* allocator,
        VkPipeline* pipelines
    ) {
        auto next_call = reinterpret_cast<PFN_vkCreateComputePipelines>(get_next_device_proc_addr("vkCreateComputePipelines"));
        auto res = next_call(device, pipeline_cache, create_info_count, create_infos, allocator, pipelines);
        if (VK_SUCCESS == res) {
            for (uint32_t i = 0; i < create_info_count; ++i) {
                auto shader_module = create_infos[i].stage.module;
                auto shader_module_info = shader_module_infos[shader_module];
                pipeline_infos.emplace(
                        pipelines[i],
                        pipeline_info{
                            .hasCooperativeMatrixKHR = shader_module_info.hasCooperativeMatrixKHR,
                            .hasDotProductInput4x8BitPackedKHR = shader_module_info.hasDotProductInput4x8BitPackedKHR,
                        });
            }
        }
        return res;
    }
    static VkResult CreateComputePipelines(
        VkDevice device,
        VkPipelineCache pipelineCache,
        uint32_t createInfoCount,
        const VkComputePipelineCreateInfo* pCreateInfos,
        const VkAllocationCallbacks* pAllocator,
        VkPipeline* pPipelines
    ) {
        auto& layer = g_devices[device];
        return layer.create_compute_pipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    }
    void destroy_pipeline(
        VkDevice device,
        VkPipeline pipeline,
        const VkAllocationCallbacks* allocator
    ) {
        auto next_call = reinterpret_cast<PFN_vkDestroyPipeline>(get_next_device_proc_addr("vkDestroyPipeline"));
        next_call(device, pipeline, allocator);
        pipeline_infos.erase(pipeline);
    }
    static void DestroyPipeline(
        VkDevice device,
        VkPipeline pipeline,
        const VkAllocationCallbacks* pAllocator
    ) {
        auto& layer = g_devices[device];
        layer.destroy_pipeline(device, pipeline, pAllocator);
    }
    VkResult create_shader_module(
        const VkShaderModuleCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkShaderModule* pShaderModule
    ) {
        auto nextCreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(get_next_device_proc_addr("vkCreateShaderModule"));
        auto res = nextCreateShaderModule(m_device, pCreateInfo, pAllocator, pShaderModule);

        bool hasCooperativeMatrixKHR = false;
        bool hasDotProductInput4x8BitPackedKHR = false;
        try{
        if (VK_SUCCESS == res) {
            using namespace spirv_parser;
            auto& out = std::cerr;
            auto spirv_code = std::span{const_cast<word*>(pCreateInfo->pCode), pCreateInfo->codeSize/sizeof(word)};

            auto m = spirv_parser::module_binary{ spirv_code };
            if (m.get_magic_number() != spv::MagicNumber) {
                throw std::runtime_error{ std::format("file magic number is {} != {}", m.get_magic_number(), spv::MagicNumber) };
            }
            for (auto inst : m) {
                const auto encode = get_instruction_encode(inst.get_opcode());
                if (encode.op != inst.get_opcode()) {
                    throw std::runtime_error{std::format("unknow opcode {}",spv::OpToString(inst.get_opcode()))};
                }
                if (encode.op != spv::OpCapability) {
                    continue;
                }
                //out << inst.get_opcode();
                auto word = inst.words+1;
                for (const auto& arg : encode.args) {
                    if (arg == instruction_argument::none || word >= inst.words + inst.get_word_count()) {
                        break;
                    }
                    auto arg_binary_ref = instruction_argument_binary_ref{arg, word, inst.words + inst.get_word_count()};
                    if (*arg_binary_ref.argument_word == spv::CapabilityCooperativeMatrixKHR) {
                        hasCooperativeMatrixKHR = true;
                    }
                    if (*arg_binary_ref.argument_word == spv::CapabilityDotProductInput4x8BitPackedKHR) {
                        hasDotProductInput4x8BitPackedKHR = true;
                    }
                    //out << " " << arg_binary_ref;
                    word += get_word_count(arg_binary_ref);
                }
                //out << std::endl;
            }
        }
        }
        catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }

        if (VK_SUCCESS == res) {
            shader_module_infos.emplace(
                    *pShaderModule,
                    shader_module_info{
                        .hasCooperativeMatrixKHR = hasCooperativeMatrixKHR,
                        .hasDotProductInput4x8BitPackedKHR = hasDotProductInput4x8BitPackedKHR
                    });
        }

        return res;
    }
    static VkResult CreateShaderModule(
        VkDevice device,
        const VkShaderModuleCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkShaderModule* pShaderModule
    ) {
        auto& layer = g_devices[device];
        auto res = layer.create_shader_module(pCreateInfo, pAllocator, pShaderModule);
        return res;
    }
    void destroy_shader_module(
        VkDevice device,
        VkShaderModule shader_module,
        const VkAllocationCallbacks* allocator
    ) {
        auto next_call = reinterpret_cast<PFN_vkDestroyShaderModule>(get_next_device_proc_addr("vkDestroyShaderModule"));
        next_call(device, shader_module, allocator);
        shader_module_infos.erase(shader_module);
    }
    static void DestroyShaderModule(
        VkDevice device,
        VkShaderModule shaderModule,
        const VkAllocationCallbacks* pAllocator
    ) {
        auto& layer = g_devices[device];
        layer.destroy_shader_module(device, shaderModule, pAllocator);
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
    VkResult create_command_pool(
        VkDevice device,
        const VkCommandPoolCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkCommandPool* pCommandPool
    ) {
        auto nextCall = reinterpret_cast<PFN_vkCreateCommandPool>(get_next_device_proc_addr("vkCreateCommandPool"));
        auto res = nextCall(device, pCreateInfo, pAllocator, pCommandPool);
        if (VK_SUCCESS == res) {
            create(*pCommandPool);
        }
        return res;
    }
    static VkResult CreateCommandPool(
        VkDevice device,
        const VkCommandPoolCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkCommandPool* pCommandPool
    ) {
        auto& layer = g_devices[device];
        return layer.create_command_pool(device, pCreateInfo, pAllocator, pCommandPool);
    }
    VkResult reset_command_pool(
        VkDevice device,
        VkCommandPool command_pool,
        VkCommandPoolResetFlags flags
    ) {
        auto next_call = reinterpret_cast<PFN_vkResetCommandPool>(get_next_device_proc_addr("vkResetCommandPool"));
        auto res = next_call(device, command_pool, flags);
        if (VK_SUCCESS == res) {
            reset(command_pool);
        }
        return res;
    }
    static VkResult ResetCommandPool(
        VkDevice device,
        VkCommandPool commandPool,
        VkCommandPoolResetFlags flags
    ) {
        auto& layer = g_devices[device];
        return layer.reset_command_pool(device, commandPool, flags);
    }
    void destroy_command_pool(
        VkDevice device,
        VkCommandPool command_pool,
        const VkAllocationCallbacks* allocator
    ) {
        auto next_call = reinterpret_cast<PFN_vkDestroyCommandPool>(get_next_device_proc_addr("vkDestroyCommandPool"));
        destroy(command_pool);
        next_call(device, command_pool, allocator);
    }
    static void DestroyCommandPool(
        VkDevice device,
        VkCommandPool commandPool,
        const VkAllocationCallbacks* pAllocator
    ) {
        auto& layer = g_devices[device];
        return layer.destroy_command_pool(device, commandPool, pAllocator);
    }
    VkResult AllocateCommandBuffers(
        const VkCommandBufferAllocateInfo* pAllocateInfo,
        VkCommandBuffer* pCommandBuffers
    ) {
        auto nextAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(get_next_device_proc_addr("vkAllocateCommandBuffers"));
        auto res = nextAllocateCommandBuffers(m_device, pAllocateInfo, pCommandBuffers);
        if (res == VK_SUCCESS) {
            for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
                add_command_buffer(pCommandBuffers[i], m_device, pAllocateInfo->commandPool);
                std::cerr << "allocate command buffer: " << pCommandBuffers[i] << std::endl;
                add_command_buffer(pAllocateInfo->commandPool, pCommandBuffers[i]);
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
    VkResult reset_command_buffer(
        VkCommandBuffer command_buffer,
        VkCommandBufferResetFlags flags
    ) {
        auto next_call = reinterpret_cast<PFN_vkResetCommandBuffer>(get_next_device_proc_addr("vkResetCommandBuffer"));
        auto& info = g_command_buffers[command_buffer];
        for (auto& c : info.command_buffers) {
            auto res = next_call(c, flags);
            assert(VK_SUCCESS == res);
        }
        reset(command_buffer);
        return VK_SUCCESS;
    }
    static VkResult ResetCommandBuffer(
        VkCommandBuffer commandBuffer,
        VkCommandBufferResetFlags flags
    ) {
        auto device = command_buffer_to_device(commandBuffer);
        auto& layer = g_devices[device];
        return layer.reset_command_buffer(commandBuffer, flags);
    }
    void free_command_buffers(
        VkDevice device,
        VkCommandPool command_pool,
        uint32_t command_buffer_count,
        const VkCommandBuffer* command_buffers
    ) {
        auto next_call = reinterpret_cast<PFN_vkFreeCommandBuffers>(get_next_device_proc_addr("vkFreeCommandBuffers"));
        next_call(device, command_pool, command_buffer_count, command_buffers);
        for (uint32_t i = 0; i < command_buffer_count; i++) {
            free_command_buffer(command_buffers[i]);
        }
    }
    static void FreeCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        uint32_t commandBufferCount,
        const VkCommandBuffer* pCommandBuffers
    ) {
        auto& layer = g_devices[device];
        return layer.free_command_buffers(device, commandPool, commandBufferCount, pCommandBuffers);
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
        auto device = command_buffer_to_device(command_buffer);
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
        auto nextQueueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(get_next_device_proc_addr("vkQueueWaitIdle"));
        bool every_command_not_split = true;
        for (auto& submit : std::span{pSubmits, submit_count}) {
            for (auto& cmd_buf : std::span{submit.pCommandBuffers, submit.commandBufferCount}) {
                auto& info = g_command_buffers[cmd_buf];
                if (info.command_buffers.size() != 1) {
                    every_command_not_split = false;
                }
            }
        }

        if (false && every_command_not_split) {
            return nextQueueSubmit(queue, submit_count, pSubmits, fence);
        }
        else {
            for (auto& submit : std::span{pSubmits, submit_count}) {
                auto cmd_submit_info = VkSubmitInfo{
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                    .pNext = submit.pNext,
                    .waitSemaphoreCount = submit.waitSemaphoreCount,
                    .pWaitSemaphores = submit.pWaitSemaphores,
                    .pWaitDstStageMask = submit.pWaitDstStageMask,
                };
                nextQueueSubmit(queue, 1, &cmd_submit_info, nullptr);
                for (auto& cmd_buf : std::span{submit.pCommandBuffers, submit.commandBufferCount}) {
                    auto& info = g_command_buffers[cmd_buf];
                    for (auto inner_cmd : info.command_buffers) {
                        auto cmd_submit_info = VkSubmitInfo{
                            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                            .commandBufferCount = 1,
                            .pCommandBuffers = &inner_cmd,
                        };
                        nextQueueWaitIdle(queue);
                        auto res = nextQueueSubmit(queue, 1, &cmd_submit_info, nullptr);
                        if (VK_SUCCESS != res) {
                            return res;
                        }
                    }
                }
                {
                    auto cmd_submit_info = VkSubmitInfo{
                        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                        .pNext = submit.pNext,
                        .signalSemaphoreCount = submit.signalSemaphoreCount,
                        .pSignalSemaphores = submit.pSignalSemaphores
                    };
                    nextQueueWaitIdle(queue);
                    nextQueueSubmit(queue, 1, &cmd_submit_info, nullptr);
                }
            }
            nextQueueWaitIdle(queue);
            nextQueueSubmit(queue, 0, nullptr, fence);
        }
        return VK_SUCCESS;
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
        auto device = command_buffer_to_device(commandBuffer);
        auto& layer = g_devices[device];
        return layer.cmd_bind_index_buffer(commandBuffer, buffer, offset, indexType);
    }
    void cmd_bind_pipeline(
        VkCommandBuffer command_buffer,
        VkPipelineBindPoint pipeline_bind_point,
        VkPipeline pipeline) {
        auto nextBindPipeline = reinterpret_cast<PFN_vkCmdBindPipeline>(get_next_device_proc_addr("vkCmdBindPipeline"));

        add_command_buffer_pipeline(command_buffer, pipeline_bind_point, pipeline);
        auto pipelines = get_command_buffer_pipelines(command_buffer);
        //std::cerr << "command bind pipeline:" << std::endl;
        //std::cerr << "bound pipelines:" << std::endl;
        for (auto& [point, pipeline] : pipelines) {
            auto pipeline_info = pipeline_infos[pipeline];
            //std::cerr << point << " " << pipeline << ": ";
            //std::cerr << pipeline_info.hasCooperativeMatrixKHR << ",";
            //std::cerr << pipeline_info.hasDotProductInput4x8BitPackedKHR<< std::endl;
        }

        VkCommandBuffer cmd_buf = command_buffer;
        auto& info = g_command_buffers[command_buffer];

        if (true) {
            if (info.unused_command_buffers.empty()) {
                auto nextAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(get_next_device_proc_addr("vkAllocateCommandBuffers"));
                VkCommandBufferAllocateInfo create_info{
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .commandPool = info.command_pool,
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount = 1
                };
                auto res = nextAllocateCommandBuffers(info.device, &create_info, &cmd_buf);
                assert(VK_SUCCESS == res);
            }
            else {
                cmd_buf = info.unused_command_buffers.back();
                info.unused_command_buffers.pop_back();
            }
            info.command_buffers.emplace_back(cmd_buf);
            auto nextBeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(get_next_device_proc_addr("vkBeginCommandBuffer"));
            auto begin_info = VkCommandBufferBeginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            };
            nextBeginCommandBuffer(cmd_buf, &begin_info);
            if (!info.pushed_constants.empty()) {
                auto& pushed_constant = info.pushed_constants.back();
                auto nextCmdPushConstants = reinterpret_cast<PFN_vkCmdPushConstants>(get_next_device_proc_addr("vkCmdPushConstants"));
                nextCmdPushConstants(cmd_buf, pushed_constant.layout, pushed_constant.stage_flags, pushed_constant.offset,
                        pushed_constant.values.size(), pushed_constant.values.data());
            }
            if (!info.bound_descriptor_sets.empty()) {
                auto& sets = info.bound_descriptor_sets.back();
                auto nextCmdBindDescriptorSets = reinterpret_cast<PFN_vkCmdBindDescriptorSets>(get_next_device_proc_addr("vkCmdBindDescriptorSets"));
                nextCmdBindDescriptorSets(cmd_buf,
                        sets.pipeline_bind_point, sets.layout, sets.first_set,
                        sets.descriptor_sets.size(), sets.descriptor_sets.data(),
                        sets.dynamic_offsets.size(), sets.dynamic_offsets.data());
            }

            cmd_buf = current_command_buffer(command_buffer);
        }

        nextBindPipeline(cmd_buf, pipeline_bind_point, pipeline);
    }
    static void CmdBindPipeline(
        VkCommandBuffer command_buffer,
        VkPipelineBindPoint pipeline_bind_point,
        VkPipeline pipeline) {
        auto device = command_buffer_to_device(command_buffer);
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
        auto device = command_buffer_to_device(command_buffer);
        auto& layer = g_devices[device];
        return layer.cmd_bind_vertex_buffers(command_buffer, first_binding, binding_count, pBuffers, pOffsets);
    }
    void cmd_push_constants(
        VkCommandBuffer command_buffer,
        VkPipelineLayout layout,
        VkShaderStageFlags stageFlags,
        uint32_t offset,
        uint32_t size,
        const void* values
    ) {
        auto next_call = reinterpret_cast<PFN_vkCmdPushConstants>(get_next_device_proc_addr("vkCmdPushConstants"));

        auto cmd_buf = command_buffer;
        cmd_buf = current_command_buffer(command_buffer);
        next_call(cmd_buf, layout, stageFlags, offset, size, values);

        auto& info = g_command_buffers[command_buffer];
        info.pushed_constants.emplace_back(layout, stageFlags, offset,
                std::vector<uint8_t>{
                    reinterpret_cast<const uint8_t*>(values), reinterpret_cast<const uint8_t*>(values)+size});
    }
    static void CmdPushConstants(
        VkCommandBuffer commandBuffer,
        VkPipelineLayout layout,
        VkShaderStageFlags stageFlags,
        uint32_t offset,
        uint32_t size,
        const void* pValues
    ) {
        auto device = command_buffer_to_device(commandBuffer);
        auto& layer = g_devices[device];
        return layer.cmd_push_constants(commandBuffer, layout, stageFlags, offset, size, pValues);
    }
    void cmd_bind_descriptor_sets(
        VkCommandBuffer command_buffer,
        VkPipelineBindPoint pipeline_bind_point,
        VkPipelineLayout layout,
        uint32_t first_set,
        uint32_t descriptor_set_count,
        const VkDescriptorSet* descriptor_sets,
        uint32_t dynamic_offset_count,
        const uint32_t* dynamic_offsets
    ) {
        auto next_call = reinterpret_cast<PFN_vkCmdBindDescriptorSets>(get_next_device_proc_addr("vkCmdBindDescriptorSets"));

        auto cmd_buf = command_buffer;
        cmd_buf = current_command_buffer(command_buffer);
        next_call(cmd_buf, pipeline_bind_point, layout, first_set, descriptor_set_count, descriptor_sets,
                dynamic_offset_count, dynamic_offsets);

        auto& info = g_command_buffers[command_buffer];
        info.bound_descriptor_sets.emplace_back(
                pipeline_bind_point, layout, first_set,
                std::vector<VkDescriptorSet>{descriptor_sets, descriptor_sets + descriptor_set_count},
                std::vector<uint32_t>{dynamic_offsets, dynamic_offsets + dynamic_offset_count});
    }
    static void CmdBindDescriptorSets(
        VkCommandBuffer commandBuffer,
        VkPipelineBindPoint pipelineBindPoint,
        VkPipelineLayout layout,
        uint32_t firstSet,
        uint32_t descriptorSetCount,
        const VkDescriptorSet* pDescriptorSets,
        uint32_t dynamicOffsetCount,
        const uint32_t* pDynamicOffsets
    ) {
        auto device = command_buffer_to_device(commandBuffer);
        auto& layer = g_devices[device];
        return layer.cmd_bind_descriptor_sets(commandBuffer, pipelineBindPoint, layout,
            firstSet, descriptorSetCount,  pDescriptorSets,
            dynamicOffsetCount, pDynamicOffsets
        );
    }
    void cmd_dispatch(
        VkCommandBuffer command_buffer,
        uint32_t group_count_x,
        uint32_t group_count_y,
        uint32_t group_count_z
    ) {
        auto next_call = reinterpret_cast<PFN_vkCmdDispatch>(get_next_device_proc_addr("vkCmdDispatch"));

        auto cmd_buf = command_buffer;
        cmd_buf = current_command_buffer(command_buffer);
        next_call(cmd_buf, group_count_x, group_count_y, group_count_z);
    }
    static void CmdDispatch(
        VkCommandBuffer commandBuffer,
        uint32_t groupCountX,
        uint32_t groupCountY,
        uint32_t groupCountZ
    ) {
        auto device = command_buffer_to_device(commandBuffer);
        auto& layer = g_devices[device];
        return layer.cmd_dispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
    }
    VkResult begin_command_buffer(
        VkCommandBuffer command_buffer,
        const VkCommandBufferBeginInfo* begin_info
    ) {
        auto next_call = reinterpret_cast<PFN_vkBeginCommandBuffer>(get_next_device_proc_addr("vkBeginCommandBuffer"));
        auto res = next_call(command_buffer, begin_info);
        return res;
    }
    static VkResult BeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        const VkCommandBufferBeginInfo* pBeginInfo
    ) {
        auto device = command_buffer_to_device(commandBuffer);
        auto& layer = g_devices[device];
        return layer.begin_command_buffer(commandBuffer, pBeginInfo);
    }
    VkResult end_command_buffer(
        VkCommandBuffer command_buffer
    ) {
        auto next_call = reinterpret_cast<PFN_vkEndCommandBuffer>(get_next_device_proc_addr("vkEndCommandBuffer"));
        auto cmd_buf = command_buffer;
        cmd_buf = current_command_buffer(command_buffer);
        auto& info = g_command_buffers[command_buffer];
        auto res = VK_SUCCESS;
        for (auto cmd_buf : info.command_buffers) {
            auto res_i = next_call(cmd_buf);
            if (VK_SUCCESS != res_i) {
                res = res_i;
            }
        }
        return res;
    }
    static VkResult EndCommandBuffer(
        VkCommandBuffer commandBuffer
    ) {
        auto device = command_buffer_to_device(commandBuffer);
        auto& layer = g_devices[device];
        return layer.end_command_buffer(commandBuffer);
    }

protected:
    inline void create(VkCommandPool command_pool) {
        command_pools.emplace(command_pool, command_pool_info{});
    }
    inline std::set<VkCommandBuffer>& command_buffers(VkCommandPool command_pool) {
        return command_pools[command_pool].command_buffers;
    }
    inline void add_command_buffer(VkCommandPool command_pool, VkCommandBuffer cmd_buf) {
        command_pools[command_pool].command_buffers.emplace(cmd_buf);
    }
    inline void reset(VkCommandPool command_pool) {
        for (const auto& command_buffer : command_buffers(command_pool)) {
            reset(command_buffer);
        }
    }
    inline void destroy(VkCommandPool command_pool) {
        for (const auto& command_buffer : command_buffers(command_pool)) {
            free_command_buffer(command_buffer);
        }
        command_pools.erase(command_pool);
    }
    inline static void add_command_buffer(
            VkCommandBuffer cmd,
            VkDevice device, VkCommandPool command_pool) {
        g_command_buffers.emplace(cmd, command_buffer{device, command_pool, {cmd}, {}, {}});
        assert(!g_command_buffers[cmd].command_buffers.empty());
    }
    inline static std::vector<std::pair<VkPipelineBindPoint, VkPipeline>> get_command_buffer_pipelines(VkCommandBuffer cmd) {
        return g_command_buffers[cmd].pipelines;
    }
    inline static void add_command_buffer_pipeline(VkCommandBuffer cmd, VkPipelineBindPoint bind_point, VkPipeline pipeline) {
        g_command_buffers[cmd].pipelines.emplace_back(bind_point, pipeline);
    }
    inline static VkDevice command_buffer_to_device(VkCommandBuffer cmd) {
        assert(g_command_buffers.contains(cmd));
        return g_command_buffers[cmd].device;
    }
    inline static void reset(VkCommandBuffer command_buffer) {
        auto& info = g_command_buffers[command_buffer];
        info.pipelines.clear();
        for (auto iter = info.command_buffers.begin()+1; iter < info.command_buffers.end(); ++iter) {
            info.unused_command_buffers.emplace_back(*iter);
        }
        info.command_buffers.resize(1);
    }
    inline static void free_command_buffer(VkCommandBuffer command_buffer) {
        reset(command_buffer);
    }
    inline VkCommandBuffer current_command_buffer(VkCommandBuffer cmd) {
        assert(g_command_buffers.contains(cmd));
        return g_command_buffers[cmd].command_buffers.back();
    }
private:
    inline static std::unordered_map<VkDevice, water_chika_debug_device_layer> g_devices;
    inline static std::unordered_map<VkQueue, VkDevice> g_queues;
    struct command_pool_info {
        std::set<VkCommandBuffer> command_buffers;
    };
    std::map<VkCommandPool, command_pool_info> command_pools;

    struct command_buffer {
        VkDevice device;
        VkCommandPool command_pool;
        std::vector<VkCommandBuffer> command_buffers;
        std::vector<std::pair<VkPipelineBindPoint, VkPipeline>> pipelines;

        std::vector<VkCommandBuffer> unused_command_buffers;

        struct constant {
            VkPipelineLayout layout;
            VkShaderStageFlags stage_flags;
            uint32_t offset;
            std::vector<uint8_t> values;
        };
        std::vector<constant> pushed_constants;

        struct descriptor_sets {
            VkPipelineBindPoint pipeline_bind_point;
            VkPipelineLayout layout;
            uint32_t first_set;
            std::vector<VkDescriptorSet> descriptor_sets;
            std::vector<uint32_t> dynamic_offsets;
        };
        std::vector<descriptor_sets> bound_descriptor_sets;
    };
    inline static std::unordered_map<VkCommandBuffer, command_buffer> g_command_buffers;

    struct shader_module_info {
        bool hasCooperativeMatrixKHR;
        bool hasDotProductInput4x8BitPackedKHR;
    };
    std::unordered_map<VkShaderModule, shader_module_info> shader_module_infos;
    struct pipeline_info {
        bool hasCooperativeMatrixKHR;
        bool hasDotProductInput4x8BitPackedKHR;
    };
    std::unordered_map<VkPipeline, pipeline_info> pipeline_infos;

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
        auto chain_info = reinterpret_cast<VkLayerInstanceCreateInfo*>(get_chain_info(pCreateInfo, VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO, VK_LAYER_LINK_INFO));

        water_chika_debug_layer layer{};

        layer.m_get_next_instance_proc_addr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;

        chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

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
        auto device_chain_info = reinterpret_cast<VkLayerDeviceCreateInfo*>(get_chain_info(pCreateInfo, VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO, VK_LAYER_LINK_INFO));
        m_get_next_device_proc_addr = device_chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
        device_chain_info->u.pLayerInfo = device_chain_info->u.pLayerInfo->pNext;

        auto nextCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(get_next_instance_proc_addr("vkCreateDevice"));
        auto res = nextCreateDevice(physical_device, pCreateInfo, pAllocator, pDevice);
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
        m_dispatch_table.emplace(std::string{pName}, (void*)procAddr);
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


extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_CreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance
) {
    return water_chika_debug_layer::CreateInstance(pCreateInfo, pAllocator, pInstance);
}

extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_CreateDevice(
    const VkPhysicalDevice physical_device,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pInstance
) {
    return water_chika_debug_layer::CreateDevice(physical_device, pCreateInfo, pAllocator, pInstance);
}

extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_MapMemory(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData
) {

    return water_chika_debug_device_layer::MapMemory(device, memory, offset, size, flags, ppData);
}

extern "C" DLLEXPORT void VKAPI_CALL water_chika_debug_layer_UnmapMemory(
    VkDevice device,
    VkDeviceMemory memory
) {
    return water_chika_debug_device_layer::UnmapMemory(device, memory);
}

extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_EnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t * pPhysicalDeviceCount,
    VkPhysicalDevice * pPhysicalDevices
) {
    return water_chika_debug_layer::EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}
extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_FlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    const VkMappedMemoryRange * pMemoryRanges
) {
    return water_chika_debug_device_layer::FlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

extern "C" DLLEXPORT void VKAPI_CALL water_chika_debug_layer_GetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physical_device,
    VkPhysicalDeviceMemoryProperties * pMemoryProperties
) {
    return water_chika_debug_layer::GetPhysicalDeviceMemoryProperties(physical_device, pMemoryProperties);
}

extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_InvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    const VkMappedMemoryRange * pMemoryRanges
) {
    return water_chika_debug_device_layer::InvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_CreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo * pCreateInfos,
    const VkAllocationCallbacks * pAllocator,
    VkPipeline * pPipeline
) {
    return water_chika_debug_device_layer::CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipeline);
}

extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_CreateRayTracingPipelinesKHR(
    VkDevice device,
    VkDeferredOperationKHR deferredOperation,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines
) {
    return water_chika_debug_device_layer::CreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}
extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_CreateComputePipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkComputePipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines
) {
    return water_chika_debug_device_layer::CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}
extern "C" DLLEXPORT void VKAPI_CALL water_chika_debug_layer_DestroyPipeline(
    VkDevice device,
    VkPipeline pipeline,
    const VkAllocationCallbacks* pAllocator
) {
    water_chika_debug_device_layer::DestroyPipeline(device, pipeline, pAllocator);
}
extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_CreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo * pCreateInfo,
    const VkAllocationCallbacks * pAllocator,
    VkShaderModule * pShaderModule
) {
    return water_chika_debug_device_layer::CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}
extern "C" DLLEXPORT void water_chika_debug_layer_DestroyShaderModule(
    VkDevice device,
    VkShaderModule shaderModule,
    const VkAllocationCallbacks* pAllocator
) {
    return water_chika_debug_device_layer::DestroyShaderModule(device, shaderModule, pAllocator);
}

extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_QueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR * pPresentInfo
) {
    return water_chika_debug_device_layer::QueuePresentKHR(queue, pPresentInfo);
}
extern "C" DLLEXPORT void VKAPI_CALL water_chika_debug_layer_GetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue * pQueue
) {
    return water_chika_debug_device_layer::GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}
extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_CreateCommandPool(
    VkDevice device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkCommandPool* pCommandPool
) {
    return water_chika_debug_device_layer::CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}
extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_ResetCommandPool(
    VkDevice device,
    VkCommandPool commandPool,
    VkCommandPoolResetFlags flags
) {
    return water_chika_debug_device_layer::ResetCommandPool(device, commandPool, flags);
}
extern "C" DLLEXPORT void VKAPI_CALL water_chika_debug_layer_DestroyCommandPool(
    VkDevice device,
    VkCommandPool commandPool,
    const VkAllocationCallbacks* pAllocator
) {
    water_chika_debug_device_layer::DestroyCommandPool(device, commandPool, pAllocator);
}
extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_AllocateCommandBuffers(
    VkDevice device,
    const VkCommandBufferAllocateInfo * pAllocateInfo,
    VkCommandBuffer * pCommandBuffers
) {
    return water_chika_debug_device_layer::AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}
extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_ResetCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkCommandBufferResetFlags flags
) {
    return water_chika_debug_device_layer::ResetCommandBuffer(commandBuffer, flags);
}
extern "C" DLLEXPORT void VKAPI_CALL water_chika_debug_layer_FreeCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers
) {
    return water_chika_debug_device_layer::FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
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
void VKAPI_CALL water_chika_debug_layer_CmdPushConstants(
    VkCommandBuffer commandBuffer,
    VkPipelineLayout layout,
    VkShaderStageFlags stageFlags,
    uint32_t offset,
    uint32_t size,
    const void* pValues
) {
    return water_chika_debug_device_layer::CmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}
void VKAPI_CALL water_chika_debug_layer_CmdBindDescriptorSets(
    VkCommandBuffer commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout,
    uint32_t firstSet,
    uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets,
    uint32_t dynamicOffsetCount,
    const uint32_t* pDynamicOffsets
) {
    return water_chika_debug_device_layer::CmdBindDescriptorSets(
        commandBuffer, pipelineBindPoint,
        layout, firstSet,
        descriptorSetCount, pDescriptorSets,
        dynamicOffsetCount, pDynamicOffsets
    );
}

void VKAPI_CALL water_chika_debug_layer_CmdDispatch(
    VkCommandBuffer commandBuffer,
    uint32_t groupCountX,
    uint32_t groupCountY,
    uint32_t groupCountZ
) {
    return water_chika_debug_device_layer::CmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}
VkResult VKAPI_CALL water_chika_debug_layer_BeginCommandBuffer(
    VkCommandBuffer commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo
) {
    return water_chika_debug_device_layer::BeginCommandBuffer(commandBuffer, pBeginInfo);
}
VkResult VKAPI_CALL water_chika_debug_layer_EndCommandBuffer(
    VkCommandBuffer commandBuffer
) {
    return water_chika_debug_device_layer::EndCommandBuffer(commandBuffer);
}

auto get_device_layer_procs() {
    std::unordered_map<std::string, void*> funcs{
        {"vkGetDeviceProcAddr", (void*)water_chika_debug_layer_GetDeviceProcAddr},
        {"vkMapMemory", (void*)water_chika_debug_layer_MapMemory},
        {"vkUnmapMemory", (void*)water_chika_debug_layer_UnmapMemory},
        {"vkFlushMappedMemoryRanges", (void*)water_chika_debug_layer_FlushMappedMemoryRanges},
        //{"vkInvalidateMappedMemoryRanges", (void*)water_chika_debug_layer_InvalidateMappedMemoryRanges},

        {"vkCreateGraphicsPipelines", (void*)water_chika_debug_layer_CreateGraphicsPipelines},
        {"vkCreateRayTracingPipelinesKHR", (void*)water_chika_debug_layer_CreateRayTracingPipelinesKHR},
        {"vkCreateComputePipelines", (void*)water_chika_debug_layer_CreateComputePipelines},
        {"vkDestroyPipeline", (void*)water_chika_debug_layer_DestroyPipeline},

        {"vkCreateShaderModule", (void*)water_chika_debug_layer_CreateShaderModule },
        {"vkDestroyShaderModule", (void*)water_chika_debug_layer_DestroyShaderModule},

        {"vkGetDeviceQueue", (void*)water_chika_debug_layer_GetDeviceQueue},
        {"vkQueuePresentKHR", (void*)water_chika_debug_layer_QueuePresentKHR},

        {"vkCreateCommandPool", (void*)water_chika_debug_layer_CreateCommandPool},
        {"vkResetCommandPool", (void*)water_chika_debug_layer_ResetCommandPool},
        {"vkDestroyCommandPool", (void*)water_chika_debug_layer_DestroyCommandPool},
        {"vkAllocateCommandBuffers", (void*)water_chika_debug_layer_AllocateCommandBuffers},
        {"vkResetCommandBuffer", (void*)water_chika_debug_layer_ResetCommandBuffer},
        {"vkFreeCommandBuffers", (void*)water_chika_debug_layer_FreeCommandBuffers},

        {"vkQueueSubmit", (void*)water_chika_debug_layer_QueueSubmit},
        {"vkAllocateMemory", (void*)water_chika_debug_layer_AllocateMemory},
        {"vkFreeMemory", (void*)water_chika_debug_layer_FreeMemory},
        {"vkBindBufferMemory", (void*)water_chika_debug_layer_BindBufferMemory},

        {"vkCmdBindIndexBuffer", (void*)water_chika_debug_layer_CmdBindIndexBuffer},
        {"vkCmdDrawIndexed", (void*)water_chika_debug_layer_CmdDrawIndexed},
        {"vkCmdBindPipeline", (void*)water_chika_debug_layer_CmdBindPipeline},
        {"vkCmdBindVertexBuffers", (void*)water_chika_debug_layer_CmdBindVertexBuffers},
        {"vkCmdPushConstants", (void*)water_chika_debug_layer_CmdPushConstants},
        {"vkCmdBindDescriptorSets", (void*)water_chika_debug_layer_CmdBindDescriptorSets},
        {"vkCmdDispatch", (void*)water_chika_debug_layer_CmdDispatch},

        {"vkBeginCommandBuffer", (void*)water_chika_debug_layer_BeginCommandBuffer},
        {"vkEndCommandBuffer", (void*)water_chika_debug_layer_EndCommandBuffer},
    };
    return funcs;
}
auto get_instance_layer_procs() {
    std::unordered_map<std::string, void*> funcs{
        std::pair<std::string,void*>{std::string{"vkGetInstanceProcAddr"}, (void*)water_chika_debug_layer_GetInstanceProcAddr},
        {"vkCreateInstance", (void*)water_chika_debug_layer_CreateInstance},
        {"vkEnumeratePhysicalDevices", (void*)water_chika_debug_layer_EnumeratePhysicalDevices},
        {"vkGetDeviceProcAddr", (void*)water_chika_debug_layer_GetDeviceProcAddr},
        {"vkCreateDevice", (void*)water_chika_debug_layer_CreateDevice},
        {"vkGetPhysicalDeviceMemoryProperties", (void*)water_chika_debug_layer_GetPhysicalDeviceMemoryProperties},
    };
    return funcs;
}

extern "C" DLLEXPORT PFN_vkVoidFunction VKAPI_CALL water_chika_debug_layer_GetDeviceProcAddr(
    VkDevice device, const char* pName
) {
    auto funcs = get_device_layer_procs();
    if (funcs.contains(pName)) {
        return reinterpret_cast<PFN_vkVoidFunction>(funcs[pName]);
    }
    return water_chika_debug_device_layer::GetDeviceProcAddr(device, pName);
}

extern "C" DLLEXPORT PFN_vkVoidFunction VKAPI_CALL water_chika_debug_layer_GetInstanceProcAddr(
    VkInstance instance, const char* pName
) {
    auto funcs = get_instance_layer_procs();
    if (funcs.contains(pName)) {
        return reinterpret_cast<PFN_vkVoidFunction>(funcs[pName]);
    }
    return water_chika_debug_layer::GetInstanceProcAddr(instance, pName);
}

extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_NegotiateLoaderLayerInterfaceVersion(
    VkNegotiateLayerInterface* pVersionStruct
){
    pVersionStruct->loaderLayerInterfaceVersion;
    pVersionStruct->pfnGetInstanceProcAddr = water_chika_debug_layer_GetInstanceProcAddr;
    pVersionStruct->pfnGetDeviceProcAddr = water_chika_debug_layer_GetDeviceProcAddr;
    return VK_SUCCESS;
}

