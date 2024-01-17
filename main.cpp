#include <stdexcept>
#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>
#include <array>
#include <vulkan/vulkan.h>
#include "vulkan_helper.hpp"
#include "spirv_helper.hpp"

class first_physical_device : public vulkan_helper::physical_device {
public:
    first_physical_device() : physical_device{
        [](vulkan_helper::instance& instance) {
            return instance.get_first_physical_device();
        }
    }
    {}
};

class compute_queue_physical_device : public first_physical_device {
public:
    compute_queue_physical_device() : m_compute_queue_family_index {
        physical_device::find_queue_family_if(
            [](VkQueueFamilyProperties properties) {
                return VK_QUEUE_COMPUTE_BIT & properties.queueFlags;
            }
        )
    }
    {}
            uint32_t get_compute_queue_family_index() {
                return m_compute_queue_family_index;
    }
private:
    uint32_t m_compute_queue_family_index;
};

class compute_queue_device : public vulkan_helper::device<compute_queue_physical_device> {
public:
    compute_queue_device() :
        device{ [](compute_queue_physical_device& physical_device) {
        vulkan_helper::device_create_info info{};
        info.set_queue_family_index(physical_device.get_compute_queue_family_index());
        return info;
            }
    }
    {}
};

class compute_queue : public compute_queue_device {
public:
    compute_queue() :
        m_queue{
        device::get_device_queue(compute_queue_device::get_compute_queue_family_index(), 0)
    }
    {}
    VkQueue get_queue() {
        return m_queue;
    }
private:
    VkQueue m_queue;
};

template<class PD>
class physical_device_cached_memory_properties : public PD {
public:
    physical_device_cached_memory_properties() : 
        m_memory_properties{
        PD::get_memory_properties()
    }
    {}
    const auto& get_memory_properties() const {
        return m_memory_properties;
    }
private:
    VkPhysicalDeviceMemoryProperties m_memory_properties;
};

using app_parent = 
    vulkan_helper::command_pool<
    vulkan_helper::fence<
    physical_device_cached_memory_properties<
    vulkan_helper::pipeline_layout<
    vulkan_helper::descriptor_set<
    vulkan_helper::descriptor_pool<
    vulkan_helper::descriptor_set_layout<
    compute_queue
    >>>>>>>;


class App : public app_parent{
public:
    using D = compute_queue_device;
    App() : 
        app_parent{
        [](D& device) {
            return device.create_command_pool(device.get_compute_queue_family_index());
        }}
    {}

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
        auto descriptor_set = app_parent::get_descriptor_set();
        vkCmdBindDescriptorSets(m_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, 
                app_parent::get_pipeline_layout(), 0, 1, &descriptor_set, 0, NULL);
        vkCmdDispatch(m_command_buffer, 1, 1, 1);
        {
            auto res = vkEndCommandBuffer(m_command_buffer);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{"failed to end command buffer"};
            }
        }
    }

    uint32_t findProperties(uint32_t memoryTypeBitsRequirements, VkMemoryPropertyFlags requiredProperty) {
        const uint32_t memoryCount = app_parent::get_memory_properties().memoryTypeCount;
        for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; memoryIndex++) {
            const uint32_t memoryTypeBits = (1 << memoryIndex);
            const bool isRequiredMemoryType = memoryTypeBitsRequirements & memoryTypeBits;
            const VkMemoryPropertyFlags properties = 
                app_parent::get_memory_properties().memoryTypes[memoryIndex].propertyFlags;
            const bool hasRequiredProperties =
                (properties & requiredProperty) == requiredProperty;
            if (isRequiredMemoryType && hasRequiredProperties)
                return memoryIndex;
        }
        throw std::runtime_error{"failed find memory property"};
    }

    void create_storage_buffer() {
        m_storage_buffer = D::create_buffer(D::get_compute_queue_family_index(), 128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        m_storage_memory = D::alloc_device_memory(app_parent::get_memory_properties(), m_storage_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_storage_memory_ptr = D::map_device_memory(m_storage_memory, 0, 128);
    }
    void destroy_storage_buffer() {
        D::destroy_buffer(m_storage_buffer);
        D::unmap_device_memory(m_storage_memory);
        D::free_device_memory(m_storage_memory);
    }

    void init() {
        {
            auto shader_module = D::create_shader_module(spirv_file{ "comp.spv" });
            m_pipeline = D::create_pipeline(shader_module, app_parent::get_pipeline_layout());
            D::destroy_shader_module(shader_module);
        }
        create_storage_buffer();
        D::update_descriptor_set(m_storage_buffer, app_parent::get_descriptor_set());

        m_command_buffer = D::allocate_command_buffer(app_parent::get_command_pool());
        record_command_buffer();
    }
    void deinit() {
        destroy_storage_buffer();

        D::destroy_pipeline(m_pipeline);
    }
    void draw() {
        fence::reset();

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
            auto res = vkQueueSubmit2(compute_queue::get_queue(), 1, &submit_info, fence::get_fence());
	    if (res != VK_SUCCESS) {
	      throw std::runtime_error{"failed to submit queue"};
	    }
        }

        fence::wait_for();

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
    VkPipeline m_pipeline;

    VkCommandBuffer m_command_buffer;

    VkBuffer m_storage_buffer;
    VkDeviceMemory m_storage_memory;
    void* m_storage_memory_ptr;
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
