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

template<class D>
class add_compute_command_pool : public vulkan_helper::command_pool<D> {
public:
    add_compute_command_pool() :
        vulkan_helper::command_pool<D>{
        [](D& device) {
            return device.create_command_pool(device.get_compute_queue_family_index());
        }
    }
    {}
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

template<class D>
class app_pipeline : public vulkan_helper::pipeline<D> {
public:
    app_pipeline() : vulkan_helper::pipeline<D>{ 
        [](D& device) { 
            return vulkan_helper::shader_module<D>{device, spirv_file{ "comp.spv" }};
        }
    }
    {}
};

using app_parent = 
    vulkan_helper::add_storage_memory_ptr<
    vulkan_helper::add_storage_memory<
    vulkan_helper::add_storage_buffer<
    vulkan_helper::command_buffer<
    add_compute_command_pool<
    vulkan_helper::fence<
    physical_device_cached_memory_properties<
    app_pipeline<
    vulkan_helper::pipeline_layout<
    vulkan_helper::descriptor_set<
    vulkan_helper::descriptor_pool<
    vulkan_helper::descriptor_set_layout<
    compute_queue
    >>>>>>>>>>>>;


class App : public app_parent{
public:
    App()
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = app_parent::get_storage_buffer();
        buffer_info.offset = 0;
        buffer_info.range = 128;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = app_parent::get_descriptor_set();
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &buffer_info;
        app_parent::update_descriptor_set(write);
        record_command_buffer();
    }

    void record_command_buffer() {
        command_buffer::begin();
        command_buffer::bind_pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, app_parent::get_pipeline());
        command_buffer::bind_descriptor_set(VK_PIPELINE_BIND_POINT_COMPUTE, 
                app_parent::get_pipeline_layout(), app_parent::get_descriptor_set());
        command_buffer::dispatch(1, 1, 1);
        command_buffer::end();
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
    void draw() {
        fence::reset();

        VkCommandBufferSubmitInfo command_buffer_submit_info{};
        {
            auto& info = command_buffer_submit_info;
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            info.commandBuffer = app_parent::get_command_buffer();
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
            uint32_t* data = reinterpret_cast<uint32_t*>(app_parent::get_storage_memory_ptr());
            std::cout << data[0];
        }
        
    }
    void run() {
      for (int i = 0; i < 8; i++)
          draw();
    }
};

int main() {
    try{
        App app;
        app.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
