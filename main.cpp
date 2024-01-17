#include <stdexcept>
#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>
#include <array>
#include <vulkan/vulkan.h>
#include "vulkan_helper.hpp"
#include "spirv_helper.hpp"

class App {
public:
    App() : m_instance{}, m_physical_device{ m_instance.get_first_physical_device() } {}

    void select_physical_device() {
        m_memory_properties = m_physical_device.get_physical_device_memory_properties();
    }
    void select_queue_family() {
        m_queue_family = m_physical_device.find_queue_family_if([](VkQueueFamilyProperties properties) {
            return VK_QUEUE_COMPUTE_BIT & properties.queueFlags;
            });
    }
    void create_device() {
        vulkan_helper::device_create_info info{};
        info.set_queue_family_index(m_queue_family);
        m_device = m_physical_device.create_device(info);

        m_queue = m_device.get_device_queue(m_queue_family, 0);

        m_fence = m_device.create_fence();
    }
    void destroy_device() {
        m_device.destroy_fence(m_fence);
    }

    void record_command_buffer() {
        {
            VkCommandBufferBeginInfo info{};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            auto res = vkBeginCommandBuffer(m_command_buffer, &info);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{"failed to begin command buffer"};
            }
        }
        vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(m_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, 
                m_pipeline_layout, 0, 1, &m_descriptor_set, 0, NULL);
        vkCmdDispatch(m_command_buffer, 1, 1, 1);
        {
            auto res = vkEndCommandBuffer(m_command_buffer);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{"failed to end command buffer"};
            }
        }
    }

    uint32_t findProperties(uint32_t memoryTypeBitsRequirements, VkMemoryPropertyFlags requiredProperty) {
        const uint32_t memoryCount = m_memory_properties.memoryTypeCount;
        for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; memoryIndex++) {
            const uint32_t memoryTypeBits = (1 << memoryIndex);
            const bool isRequiredMemoryType = memoryTypeBitsRequirements & memoryTypeBits;
            const VkMemoryPropertyFlags properties = 
                m_memory_properties.memoryTypes[memoryIndex].propertyFlags;
            const bool hasRequiredProperties =
                (properties & requiredProperty) == requiredProperty;
            if (isRequiredMemoryType && hasRequiredProperties)
                return memoryIndex;
        }
        throw std::runtime_error{"failed find memory property"};
    }




    void create_storage_buffer() {
        m_storage_buffer = m_device.create_buffer(m_queue_family, 128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        m_storage_memory = m_device.alloc_device_memory(m_memory_properties, m_storage_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_storage_memory_ptr = m_device.map_device_memory(m_storage_memory, 0, 128);
    }
    void destroy_storage_buffer() {
        m_device.destroy_buffer(m_storage_buffer);
        m_device.unmap_device_memory(m_storage_memory);
        m_device.free_device_memory(m_storage_memory);
    }

    void init() {
        select_physical_device();
        select_queue_family();
        create_device();
        m_descriptor_set_layout = m_device.create_descriptor_set_layout();
        m_pipeline_layout = m_device.create_pipeline_layout(m_descriptor_set_layout);
        
        auto shader_module = m_device.create_shader_module(spirv_file{"comp.spv"});
        m_pipeline = m_device.create_pipeline(shader_module, m_pipeline_layout);
        m_descriptor_pool = m_device.create_descriptor_pool();
        m_descriptor_set = m_device.allocate_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);
        create_storage_buffer();
        m_device.update_descriptor_set(m_storage_buffer, m_descriptor_set);

        m_command_pool = m_device.create_command_pool(m_queue_family);
        m_command_buffer = m_device.allocate_command_buffer(m_command_pool);
        record_command_buffer();
    }
    void deinit() {
        m_device.destroy_command_pool(m_command_pool);
        
        destroy_storage_buffer();
        m_device.destroy_descriptor_pool(m_descriptor_pool);

        m_device.destroy_pipeline(m_pipeline);
        m_device.destroy_pipeline_layout(m_pipeline_layout);
        m_device.destroy_descriptor_set_layout(m_descriptor_set_layout);
        destroy_device();
    }
    void draw() {
        m_device.reset_fence(m_fence);

        VkCommandBufferSubmitInfo command_buffer_submit_info{};
        {
            auto& info = command_buffer_submit_info;
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            info.commandBuffer = m_command_buffer;
        }
        VkSubmitInfo2 submit_info{};
        {
            auto& info = submit_info;
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
            info.commandBufferInfoCount = 1;
            info.pCommandBufferInfos = &command_buffer_submit_info;
            auto res = vkQueueSubmit2(m_queue, 1, &submit_info, m_fence);
	    if (res != VK_SUCCESS) {
	      throw std::runtime_error{"failed to submit queue"};
	    }
        }

        m_device.wait_for_fence(m_fence);

        {
            uint32_t* data = reinterpret_cast<uint32_t*>(m_storage_memory_ptr);
            std::cout << data[0];
        }
        
    }
    void run() {
      for (int i = 0; i < 8; i++)
          draw();
    }
private:
    vulkan_helper::instance m_instance;
    vulkan_helper::physical_device m_physical_device;
    uint32_t m_queue_family;
    vulkan_helper::device m_device;
    VkQueue m_queue;
    VkFence m_fence;
    VkPhysicalDeviceMemoryProperties m_memory_properties;

    VkPipeline m_pipeline;
    VkPipelineLayout m_pipeline_layout;
    VkDescriptorSetLayout m_descriptor_set_layout;

    VkCommandPool m_command_pool;
    VkCommandBuffer m_command_buffer;

    VkBuffer m_storage_buffer;
    VkDeviceMemory m_storage_memory;
    void* m_storage_memory_ptr;

    VkDescriptorPool m_descriptor_pool;
    VkDescriptorSet m_descriptor_set;
};

int main() {
    try{
        App app;
        app.init();
        app.run();
        app.deinit();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
