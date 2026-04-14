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
#include <deque>

#include <spirv_parser.hpp>

#include <vulkan/vulkan_beta.h>
#include <vk_cmd_buf_split_info.hpp>

#undef max

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

struct pipeline_info {
    bool hasCooperativeMatrixKHR;
    bool hasDotProductInput4x8BitPackedKHR;
};

class water_chika_debug_command_buffer_info : public water_chika::cmd_buf_split_info<water_chika_debug_command_buffer_info> {
public:
    water_chika_debug_command_buffer_info() = default;
    water_chika_debug_command_buffer_info(
        VkDevice device,
        PFN_vkGetDeviceProcAddr get_device_proc_addr,
        PFN_vkSetDeviceLoaderData set_device_loader_data,
        VkCommandPool pool,
        std::unordered_map<VkPipeline, pipeline_info>* pipeline_infos
            )
    :
        m_device{device},
        m_get_next_device_proc_addr{ get_device_proc_addr },
        m_set_device_loader_data{ set_device_loader_data },
        m_command_pool{pool},
        m_pipeline_infos{pipeline_infos}
    {
        add_split_command_buffer();
    }
    PFN_vkVoidFunction get_next_device_proc_addr(const char* pName) {
        if (m_dispatch_table.contains(pName)) {
            return reinterpret_cast<PFN_vkVoidFunction>(m_dispatch_table[pName]);
        }
        auto procAddr = m_get_next_device_proc_addr(m_device, pName);
        m_dispatch_table.emplace(std::string{pName}, (void*)procAddr);
        return procAddr;
    }

    VkResult ResetCommandBuffer(
        VkCommandBuffer commandBuffer,
        VkCommandBufferResetFlags flags
    ) {
        auto next_call = reinterpret_cast<PFN_vkResetCommandBuffer>(get_next_device_proc_addr("vkResetCommandBuffer"));
        auto res = next_call(commandBuffer, flags);
        for (auto& c : m_command_buffers) {
            res = next_call(c, flags);
            assert(VK_SUCCESS == res);
        }
        reset();
        return res;
    }
    void reset() {
        m_pipelines.clear();
        for (auto iter = m_command_buffers.begin()+1; iter < m_command_buffers.end(); ++iter) {
            m_unused_command_buffers.emplace_back(*iter);
        }
        m_command_buffers.resize(1);
    }

    void CmdBindPipeline(
        VkCommandBuffer command_buffer,
        VkPipelineBindPoint pipeline_bind_point,
        VkPipeline pipeline) {
        auto nextBindPipeline = reinterpret_cast<PFN_vkCmdBindPipeline>(get_next_device_proc_addr("vkCmdBindPipeline"));

        m_pipelines.emplace_back(pipeline_bind_point, pipeline);
        auto& pipelines = m_pipelines;
        //std::cerr << "command bind pipeline:" << std::endl;
        //std::cerr << "bound pipelines:" << std::endl;
        for (auto& [point, pipeline] : pipelines) {
            auto pipeline_info = (*m_pipeline_infos)[pipeline];
            //std::cerr << point << " " << pipeline << ": ";
            //std::cerr << pipeline_info.hasCooperativeMatrixKHR << ",";
            //std::cerr << pipeline_info.hasDotProductInput4x8BitPackedKHR<< std::endl;
        }

        nextBindPipeline(command_buffer, pipeline_bind_point, pipeline);

        auto pipeline_info = (*m_pipeline_infos)[pipeline];
        bool decode_start = pipeline_info.hasDotProductInput4x8BitPackedKHR && !pipeline_info.hasCooperativeMatrixKHR;
        bool decode_end = pipeline_info.hasCooperativeMatrixKHR && !pipeline_info.hasDotProductInput4x8BitPackedKHR;
        bool split_this = false;
        if (decode_start && !m_decode_enabled) {
            m_decode_enabled = true;
            split_this = true;
        }
        else if (decode_end && m_decode_enabled) {
            m_decode_enabled = false;
            split_this = true;
        }

        if (could_split) {
            if (split_this) {
                add_split_command_buffer();
                auto cmd_buf = m_command_buffers.back();
                auto nextBeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(get_next_device_proc_addr("vkBeginCommandBuffer"));
                auto begin_info = VkCommandBufferBeginInfo{
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                };
                nextBeginCommandBuffer(cmd_buf, &begin_info);
                if (!m_pushed_constants.empty()) {
                    auto& pushed_constant = m_pushed_constants.back();
                    auto nextCmdPushConstants = reinterpret_cast<PFN_vkCmdPushConstants>(get_next_device_proc_addr("vkCmdPushConstants"));
                    nextCmdPushConstants(cmd_buf, pushed_constant.layout, pushed_constant.stage_flags, pushed_constant.offset,
                            pushed_constant.values.size(), pushed_constant.values.data());
                }
                if (!m_bound_descriptor_sets.empty()) {
                    auto& sets = m_bound_descriptor_sets.back();
                    auto nextCmdBindDescriptorSets = reinterpret_cast<PFN_vkCmdBindDescriptorSets>(get_next_device_proc_addr("vkCmdBindDescriptorSets"));
                    nextCmdBindDescriptorSets(cmd_buf,
                            sets.pipeline_bind_point, sets.layout, sets.first_set,
                            sets.descriptor_sets.size(), sets.descriptor_sets.data(),
                            sets.dynamic_offsets.size(), sets.dynamic_offsets.data());
                }
            }
            auto cmd_buf = m_command_buffers.back();
            nextBindPipeline(cmd_buf, pipeline_bind_point, pipeline);
        }
    }
    void CmdPushConstants(
        VkCommandBuffer command_buffer,
        VkPipelineLayout layout,
        VkShaderStageFlags stageFlags,
        uint32_t offset,
        uint32_t size,
        const void* values
    ) {
        auto next_call = reinterpret_cast<PFN_vkCmdPushConstants>(get_next_device_proc_addr("vkCmdPushConstants"));

        auto cmd_buf = command_buffer;
        next_call(cmd_buf, layout, stageFlags, offset, size, values);
        if (could_split) {
            cmd_buf = m_command_buffers.back();
            next_call(cmd_buf, layout, stageFlags, offset, size, values);

            m_pushed_constants.emplace_back(layout, stageFlags, offset,
                    std::vector<uint8_t>{
                        reinterpret_cast<const uint8_t*>(values), reinterpret_cast<const uint8_t*>(values)+size});
        }
    }
    void CmdBindDescriptorSets(
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
        next_call(cmd_buf, pipeline_bind_point, layout, first_set, descriptor_set_count, descriptor_sets,
                dynamic_offset_count, dynamic_offsets);
        if (could_split) {
            cmd_buf = m_command_buffers.back();
            next_call(cmd_buf, pipeline_bind_point, layout, first_set, descriptor_set_count, descriptor_sets,
                    dynamic_offset_count, dynamic_offsets);

            m_bound_descriptor_sets.emplace_back(
                    pipeline_bind_point, layout, first_set,
                    std::vector<VkDescriptorSet>{descriptor_sets, descriptor_sets + descriptor_set_count},
                    std::vector<uint32_t>{dynamic_offsets, dynamic_offsets + dynamic_offset_count});
        }
    }
    void CmdCopyBuffer (
        VkCommandBuffer commandBuffer,
        VkBuffer srcBuffer,
        VkBuffer dstBuffer,
        uint32_t regionCount,
        const VkBufferCopy* pRegions) {
        auto next_call = reinterpret_cast<PFN_vkCmdCopyBuffer>(get_next_device_proc_addr("vkCmdCopyBuffer"));
        next_call(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
        if (could_split) {
            next_call(m_command_buffers.back(), srcBuffer, dstBuffer, regionCount, pRegions);
        }
    }
    void CmdFillBuffer (
        VkCommandBuffer commandBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize dstOffset,
        VkDeviceSize size,
        uint32_t data) {
        auto next_call = reinterpret_cast<PFN_vkCmdFillBuffer>(get_next_device_proc_addr("vkCmdFillBuffer"));
        next_call(commandBuffer, dstBuffer, dstOffset, size, data);
        if (could_split) {
            next_call(m_command_buffers.back(), dstBuffer, dstOffset, size, data);
        }
    }
    void CmdPipelineBarrier (
         VkCommandBuffer commandBuffer,
         VkPipelineStageFlags srcStageMask,
         VkPipelineStageFlags dstStageMask,
         VkDependencyFlags dependencyFlags,
         uint32_t memoryBarrierCount,
         const VkMemoryBarrier* pMemoryBarriers,
         uint32_t bufferMemoryBarrierCount,
         const VkBufferMemoryBarrier* pBufferMemoryBarriers,
         uint32_t imageMemoryBarrierCount,
         const VkImageMemoryBarrier* pImageMemoryBarriers) {
        auto next_call = reinterpret_cast<PFN_vkCmdPipelineBarrier>(get_next_device_proc_addr("vkCmdPipelineBarrier"));
        next_call(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
        if (could_split) {
            next_call(m_command_buffers.back(), srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
        }
    }
    void CmdDispatch(
        VkCommandBuffer command_buffer,
        uint32_t group_count_x,
        uint32_t group_count_y,
        uint32_t group_count_z
    ) {
        auto next_call = reinterpret_cast<PFN_vkCmdDispatch>(get_next_device_proc_addr("vkCmdDispatch"));
        auto cmd_buf = command_buffer;
        next_call(cmd_buf, group_count_x, group_count_y, group_count_z);
        if (could_split) {
            cmd_buf = m_command_buffers.back();
            next_call(cmd_buf, group_count_x, group_count_y, group_count_z);
        }
    }
    VkResult BeginCommandBuffer(
        VkCommandBuffer command_buffer,
        const VkCommandBufferBeginInfo* begin_info
    ) {
        auto next_call = reinterpret_cast<PFN_vkBeginCommandBuffer>(get_next_device_proc_addr("vkBeginCommandBuffer"));
        auto res = next_call(command_buffer, begin_info);
        if (could_split) {
            next_call(m_command_buffers.back(), begin_info);
        }
        return res;
    }
    VkResult EndCommandBuffer(
        VkCommandBuffer command_buffer
    ) {
        auto next_call = reinterpret_cast<PFN_vkEndCommandBuffer>(get_next_device_proc_addr("vkEndCommandBuffer"));
        auto cmd_buf = command_buffer;
        auto res = next_call(cmd_buf);

        if (could_split) {
            for (auto cmd_buf : m_command_buffers) {
                auto res_i = next_call(cmd_buf);
                if (VK_SUCCESS != res_i) {
                    res = res_i;
                }
            }
            //std::cerr << "command is splitted to " << m_command_buffers.size() << std::endl;
        }
        return res;
    }

    void add_split_command_buffer() {
        VkCommandBuffer cmd_buf;
        if (m_unused_command_buffers.empty()) {
            auto nextAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(get_next_device_proc_addr("vkAllocateCommandBuffers"));
            VkCommandBufferAllocateInfo create_info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = m_command_pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1
            };
            auto res = nextAllocateCommandBuffers(m_device, &create_info, &cmd_buf);
            assert(VK_SUCCESS == res);
            m_set_device_loader_data(m_device, cmd_buf);
        }
        else {
            cmd_buf = m_unused_command_buffers.back();
            m_unused_command_buffers.pop_back();
        }
        m_command_buffers.push_back(cmd_buf);
    }

    auto& splitted_command_buffers() {
        return m_command_buffers;
    }
    auto is_splittable() {
        return could_split;
    }
    auto is_splitted() {
        return could_split && m_command_buffers.size() > 1;
    }

    static inline std::unordered_map<VkCommandBuffer, water_chika_debug_command_buffer_info> g_command_buffers;
private:
    VkDevice m_device;
    PFN_vkGetDeviceProcAddr m_get_next_device_proc_addr;
    PFN_vkSetDeviceLoaderData m_set_device_loader_data;
    std::map<std::string, void*> m_dispatch_table;

    VkCommandPool m_command_pool;
    std::vector<VkCommandBuffer> m_command_buffers;
    std::vector<std::pair<VkPipelineBindPoint, VkPipeline>> m_pipelines;

    std::vector<VkCommandBuffer> m_unused_command_buffers;

    struct constant {
        VkPipelineLayout layout;
        VkShaderStageFlags stage_flags;
        uint32_t offset;
        std::vector<uint8_t> values;
    };
    std::vector<constant> m_pushed_constants;

    struct descriptor_sets {
        VkPipelineBindPoint pipeline_bind_point;
        VkPipelineLayout layout;
        uint32_t first_set;
        std::vector<VkDescriptorSet> descriptor_sets;
        std::vector<uint32_t> dynamic_offsets;
    };
    std::vector<descriptor_sets> m_bound_descriptor_sets;
    std::unordered_map<VkPipeline, pipeline_info>* m_pipeline_infos;
    bool m_decode_enabled;
};
class water_chika_debug_layer;
class water_chika_debug_device_layer{
public:
    static void add_device(
        water_chika_debug_layer* instance_layer,
        VkPhysicalDevice physical_device,
        VkDevice device,
        PFN_vkGetDeviceProcAddr get_device_proc_addr,
        PFN_vkSetDeviceLoaderData set_device_loader_data
        ) {
        water_chika_debug_device_layer device_layer{
            instance_layer, physical_device, device,
            get_device_proc_addr,
            set_device_loader_data
        };
        g_devices.emplace(device, std::move(device_layer));
    }
    water_chika_debug_device_layer() = default;
    water_chika_debug_device_layer(const water_chika_debug_device_layer&) = delete;
    water_chika_debug_device_layer(water_chika_debug_device_layer&&) = default;
    water_chika_debug_device_layer& operator=(const water_chika_debug_device_layer&) = delete;
    water_chika_debug_device_layer& operator=(water_chika_debug_device_layer&&) = default;
    water_chika_debug_device_layer(
        water_chika_debug_layer* instance_layer, const VkPhysicalDevice physical_device, VkDevice device,
        PFN_vkGetDeviceProcAddr get_device_proc_addr,
        PFN_vkSetDeviceLoaderData set_device_loader_data
        )
        :m_instance_layer{ instance_layer }, m_physical_device{ physical_device }, m_device{ device },
        m_get_next_device_proc_addr{ get_device_proc_addr },
        m_set_device_loader_data{ set_device_loader_data }
        {
    }
    ~water_chika_debug_device_layer() {
        auto used_fences = std::vector<VkFence>(used_fence_semaphores.size());
        std::transform(used_fence_semaphores.begin(), used_fence_semaphores.end(),
                used_fences.begin(),
                [](auto& p) {
                    return p.first;
                });
        auto wait_for_fences = reinterpret_cast<PFN_vkWaitForFences>(get_next_device_proc_addr("vkWaitForFences"));
        auto res = wait_for_fences(m_device,
                used_fences.size(), used_fences.data(), VK_TRUE,
                std::numeric_limits<uint64_t>::max());
        if (VK_SUCCESS == res) {
            auto destroy_fence = reinterpret_cast<PFN_vkDestroyFence>(get_next_device_proc_addr("vkDestroyFence"));
            auto destroy_semaphore = reinterpret_cast<PFN_vkDestroySemaphore>(get_next_device_proc_addr("vkDestroySemaphore"));
            for (auto& [fence,semaphores] : used_fence_semaphores) {
                destroy_fence(m_device, fence, nullptr);
                for (auto semaphore : semaphores) {
                    destroy_semaphore(m_device, semaphore, nullptr);
                }
            }
            for (auto fence : unused_fences) {
                destroy_fence(m_device, fence, nullptr);
            }
            for (auto semaphore : unused_semaphores) {
                destroy_semaphore(m_device, semaphore, nullptr);
            }
        }
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
        m_dispatch_table.emplace(std::string{pName}, (void*)procAddr);
        return procAddr;
    }

    VkResult CreateGraphicsPipelines(
        VkDevice device,
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
    VkResult CreateRayTracingPipelinesKHR(
        VkDevice device,
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
    VkResult CreateComputePipelines(
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
    void DestroyPipeline(
        VkDevice device,
        VkPipeline pipeline,
        const VkAllocationCallbacks* allocator
    ) {
        auto next_call = reinterpret_cast<PFN_vkDestroyPipeline>(get_next_device_proc_addr("vkDestroyPipeline"));
        next_call(device, pipeline, allocator);
        pipeline_infos.erase(pipeline);
    }
    VkResult CreateShaderModule(
        VkDevice device,
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
    void DestroyShaderModule(
        VkDevice device,
        VkShaderModule shader_module,
        const VkAllocationCallbacks* allocator
    ) {
        auto next_call = reinterpret_cast<PFN_vkDestroyShaderModule>(get_next_device_proc_addr("vkDestroyShaderModule"));
        next_call(device, shader_module, allocator);
        shader_module_infos.erase(shader_module);
    }
    void GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue) {
        auto nextGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(get_next_device_proc_addr("vkGetDeviceQueue"));
        nextGetDeviceQueue(m_device, queueFamilyIndex, queueIndex, pQueue);
        g_queues.emplace(*pQueue, m_device);
    }
    VkResult QueuePresent(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
        auto nextQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(get_next_device_proc_addr("vkQueuePresentKHR"));
        auto res = nextQueuePresentKHR(queue, pPresentInfo);
        return res;
    }
    VkResult CreateCommandPool(
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
    VkResult ResetCommandPool(
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
    void DestroyCommandPool(
        VkDevice device,
        VkCommandPool command_pool,
        const VkAllocationCallbacks* allocator
    ) {
        auto next_call = reinterpret_cast<PFN_vkDestroyCommandPool>(get_next_device_proc_addr("vkDestroyCommandPool"));
        destroy(command_pool);
        next_call(device, command_pool, allocator);
    }
    VkResult AllocateCommandBuffers(
        VkDevice device,
        const VkCommandBufferAllocateInfo* pAllocateInfo,
        VkCommandBuffer* pCommandBuffers
    ) {
        auto nextAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(get_next_device_proc_addr("vkAllocateCommandBuffers"));
        auto res = nextAllocateCommandBuffers(m_device, pAllocateInfo, pCommandBuffers);
        if (res == VK_SUCCESS) {
            for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
                add_command_buffer(pCommandBuffers[i], m_device, pAllocateInfo->commandPool);
                //std::cerr << "allocate command buffer: " << pCommandBuffers[i] << std::endl;
                add_command_buffer(pAllocateInfo->commandPool, pCommandBuffers[i]);
            }
        }
        return res;
    }
    void FreeCommandBuffers(
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
    VkResult QueueSubmit(
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
                auto& info = water_chika_debug_command_buffer_info::g_command_buffers[cmd_buf];
                if (info.is_splitted()) {
                    every_command_not_split = false;
                }
            }
        }

        if (16 < used_fence_semaphores.size()) {
            auto& [fence, semaphores] = used_fence_semaphores.front();
            auto wait_for_fences = reinterpret_cast<PFN_vkWaitForFences>(get_next_device_proc_addr("vkWaitForFences"));
            auto res = wait_for_fences(m_device, 1, &fence, VK_TRUE, 0);
            if (VK_SUCCESS == res) {
                auto reset_fences = reinterpret_cast<PFN_vkResetFences>(get_next_device_proc_addr("vkResetFences"));
                reset_fences(m_device, 1, &fence);
                unused_fences.push_back(fence);
                for (auto semaphore : semaphores) {
                    unused_semaphores.push_back(semaphore);
                }
                used_fence_semaphores.pop_front();
            }
        }

        if (false && every_command_not_split) {
            return nextQueueSubmit(queue, submit_count, pSubmits, fence);
        }
        else {
            for (auto& submit : std::span{pSubmits, submit_count}) {
                auto semaphores = std::vector<VkSemaphore>();
                auto semaphore = get_or_create_semaphore();
                semaphores.push_back(semaphore);
                auto cmd_submit_info = VkSubmitInfo{
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                    .pNext = submit.pNext,
                    .waitSemaphoreCount = submit.waitSemaphoreCount,
                    .pWaitSemaphores = submit.pWaitSemaphores,
                    .pWaitDstStageMask = submit.pWaitDstStageMask,
                    .signalSemaphoreCount = 1,
                    .pSignalSemaphores = &semaphore
                };
                nextQueueSubmit(queue, 1, &cmd_submit_info, nullptr);
                auto prev_semaphore = semaphore;
                for (auto& cmd_buf : std::span{submit.pCommandBuffers, submit.commandBufferCount}) {
                    auto& info = water_chika_debug_command_buffer_info::g_command_buffers[cmd_buf];
                    if (info.is_splitted()) {
                        for (auto inner_cmd : info.splitted_command_buffers()) {
                            auto semaphore = get_or_create_semaphore();
                            semaphores.push_back(semaphore);
                            VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                            auto cmd_submit_info = VkSubmitInfo{
                                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .waitSemaphoreCount = 1,
                                .pWaitSemaphores = &prev_semaphore,
                                .pWaitDstStageMask = &wait_dst_stage_mask,
                                .commandBufferCount = 1,
                                .pCommandBuffers = &inner_cmd,
                                .signalSemaphoreCount = 1,
                                .pSignalSemaphores = &semaphore,
                            };
                            auto res = nextQueueSubmit(queue, 1, &cmd_submit_info, nullptr);
                            prev_semaphore = semaphore;
                            if (VK_SUCCESS != res) {
                                return res;
                            }
                        }
                    }
                    else {
                        auto semaphore = get_or_create_semaphore();
                        semaphores.push_back(semaphore);
                        VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                        auto cmd_submit_info = VkSubmitInfo{
                            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                            .waitSemaphoreCount = 1,
                            .pWaitSemaphores = &prev_semaphore,
                            .pWaitDstStageMask = &wait_dst_stage_mask,
                            .commandBufferCount = 1,
                            .pCommandBuffers = &cmd_buf,
                            .signalSemaphoreCount = 1,
                            .pSignalSemaphores = &semaphore
                        };
                        auto res = nextQueueSubmit(queue, 1, &cmd_submit_info, nullptr);
                        prev_semaphore = semaphore;
                        if (VK_SUCCESS != res) {
                            return res;
                        }
                    }
                }
                {
                    VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                    auto cmd_submit_info = VkSubmitInfo{
                        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                        .pNext = submit.pNext,
                        .waitSemaphoreCount = 1,
                        .pWaitSemaphores = &prev_semaphore,
                        .pWaitDstStageMask = &wait_dst_stage_mask,
                        .signalSemaphoreCount = submit.signalSemaphoreCount,
                        .pSignalSemaphores = submit.pSignalSemaphores
                    };
                    auto fence = get_or_create_fence();
                    nextQueueSubmit(queue, 1, &cmd_submit_info, fence);
                    used_fence_semaphores.emplace_back(fence, std::move(semaphores));
                }
            }
            //nextQueueWaitIdle(queue);
            nextQueueSubmit(queue, 0, nullptr, fence);
        }
        return VK_SUCCESS;
    }

    inline static std::unordered_map<VkDevice, water_chika_debug_device_layer> g_devices;
    inline static std::unordered_map<VkQueue, VkDevice> g_queues;

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
    inline static void reset(VkCommandBuffer command_buffer) {
        auto& info = water_chika_debug_command_buffer_info::g_command_buffers[command_buffer];
        info.reset();
    }
    inline void destroy(VkCommandPool command_pool) {
        for (const auto& command_buffer : command_buffers(command_pool)) {
            free_command_buffer(command_buffer);
        }
        command_pools.erase(command_pool);
    }
    inline void add_command_buffer(
            VkCommandBuffer cmd,
            VkDevice device, VkCommandPool command_pool) {
        water_chika_debug_command_buffer_info::g_command_buffers.emplace(cmd, water_chika_debug_command_buffer_info{m_device, m_get_next_device_proc_addr, m_set_device_loader_data, command_pool, &pipeline_infos});
    }
    inline static void free_command_buffer(VkCommandBuffer command_buffer) {
        reset(command_buffer);
    }

    VkFence get_or_create_fence() {
        if (unused_fences.empty()) {
            auto create_fence = reinterpret_cast<PFN_vkCreateFence>(get_next_device_proc_addr("vkCreateFence"));
            VkFence fence;
            auto create_info = VkFenceCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            };
            auto res = create_fence(m_device, &create_info, nullptr, &fence);
            assert(VK_SUCCESS == res);
            return fence;
        }
        else {
            auto fence = unused_fences.back();
            unused_fences.pop_back();
            return fence;
        }
    }
    VkSemaphore get_or_create_semaphore() {
        if (unused_semaphores.empty()) {
            auto create_semaphore = reinterpret_cast<PFN_vkCreateSemaphore>(get_next_device_proc_addr("vkCreateSemaphore"));
            VkSemaphore semaphore;
            auto create_info = VkSemaphoreCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            };
            auto res = create_semaphore(m_device, &create_info, nullptr, &semaphore);
            assert(VK_SUCCESS == res);
            return semaphore;
        }
        else {
            auto semaphore = unused_semaphores.back();
            unused_semaphores.pop_back();
            return semaphore;
        }
    }
private:
    struct command_pool_info {
        std::set<VkCommandBuffer> command_buffers;
    };
    std::map<VkCommandPool, command_pool_info> command_pools;


    struct shader_module_info {
        bool hasCooperativeMatrixKHR;
        bool hasDotProductInput4x8BitPackedKHR;
    };
    std::unordered_map<VkShaderModule, shader_module_info> shader_module_infos;
    std::unordered_map<VkPipeline, pipeline_info> pipeline_infos;

    std::deque<std::pair<VkFence, std::vector<VkSemaphore>>> used_fence_semaphores;
    std::vector<VkFence> unused_fences;
    std::vector<VkSemaphore> unused_semaphores;

    water_chika_debug_layer* m_instance_layer;
    VkPhysicalDevice m_physical_device;
    VkDevice m_device;
    PFN_vkGetDeviceProcAddr m_get_next_device_proc_addr;
    std::unordered_map<std::string, void*> m_dispatch_table;
    PFN_vkSetDeviceLoaderData m_set_device_loader_data;
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

        auto chain_info = reinterpret_cast<VkLayerDeviceCreateInfo*>(get_chain_info(pCreateInfo,
                    VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO,
                    VK_LOADER_DATA_CALLBACK));
        auto set_device_loader_data = chain_info->u.pfnSetDeviceLoaderData;

        auto nextCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(get_next_instance_proc_addr("vkCreateDevice"));
        auto res = nextCreateDevice(physical_device, pCreateInfo, pAllocator, pDevice);
        if (res == VK_SUCCESS) {
            water_chika_debug_device_layer::add_device(
                this,
                physical_device,
                *pDevice,
                m_get_next_device_proc_addr,
                set_device_loader_data
                );
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

extern "C" DLLEXPORT VkResult VKAPI_CALL water_chika_debug_layer_EnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t * pPhysicalDeviceCount,
    VkPhysicalDevice * pPhysicalDevices
) {
    return water_chika_debug_layer::EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}
auto get_instance_layer_procs() {
    std::unordered_map<std::string, void*> funcs{
        std::pair<std::string,void*>{std::string{"vkGetInstanceProcAddr"}, (void*)water_chika_debug_layer_GetInstanceProcAddr},
        {"vkCreateInstance", (void*)water_chika_debug_layer_CreateInstance},
        {"vkEnumeratePhysicalDevices", (void*)water_chika_debug_layer_EnumeratePhysicalDevices},
        {"vkGetDeviceProcAddr", (void*)water_chika_debug_layer_GetDeviceProcAddr},
        {"vkCreateDevice", (void*)water_chika_debug_layer_CreateDevice},
    };
    return funcs;
}

#include <device_dispatch.hpp>

extern "C" DLLEXPORT PFN_vkVoidFunction VKAPI_CALL water_chika_debug_layer_GetDeviceProcAddr(
    VkDevice device, const char* pName
) {
    auto funcs = get_device_procs();
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

