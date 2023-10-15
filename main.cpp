#include <stdexcept>
#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>
#include <array>
#include <vulkan/vulkan.h>

class App {
public:
    void create_instance() {
        VkApplicationInfo application_info{};
	{
	  auto& info = application_info;
	  info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	  info.apiVersion = VK_API_VERSION_1_3;
	}
        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &application_info;
        auto res = vkCreateInstance(&create_info, NULL, &m_instance);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance");
        }
    }
    void destroy_instance() {
        vkDestroyInstance(m_instance, NULL);
    }
    void select_physical_device() {
        constexpr uint32_t COUNT = 8;
        std::array<VkPhysicalDevice, COUNT> physical_devices{};
        uint32_t count = COUNT;
        auto res = vkEnumeratePhysicalDevices(m_instance, &count, physical_devices.data());
        if (res == VK_INCOMPLETE) {
            throw std::runtime_error("too more physical device");
        }
        else if (res != VK_SUCCESS) {
            throw std::runtime_error{"failed to enumerate physical devices"};
        }
        assert(count > 0);
        m_physical_device = physical_devices[0];

        vkGetPhysicalDeviceMemoryProperties(m_physical_device, &m_memory_properties);
    }
    void select_queue_family() {
        constexpr uint32_t COUNT = 8;
        std::array<VkQueueFamilyProperties, COUNT> properties{};
        uint32_t count = COUNT;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &count, properties.data());
        assert(count > 0);
        for (uint32_t i = 0; i < count; i++) {
            auto& property = properties[i];
            if (VK_QUEUE_COMPUTE_BIT & property.queueFlags) {
                m_queue_family = i;
                return;
            }
        }
        throw std::runtime_error{"failed to get compute queue family"};
    }
    void create_device() {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = m_queue_family;
        queue_create_info.queueCount = 1;
        float priority = 1.0;
        queue_create_info.pQueuePriorities = &priority;

        VkDeviceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;
        auto res = vkCreateDevice(m_physical_device, &create_info, NULL, &m_device);
        if (res != VK_SUCCESS) {
            throw std::runtime_error{"failed to create device"};
        }

        vkGetDeviceQueue(m_device, m_queue_family, 0, &m_queue);

        VkFenceCreateInfo fence_create_info{};
        {
            auto& info = fence_create_info;
            info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            auto res = vkCreateFence(m_device, &fence_create_info, NULL, &m_fence);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{"failed to create fence"};
            }
        }
    }
    void destroy_device() {
        vkDestroyFence(m_device, m_fence, NULL);
        vkDestroyDevice(m_device, NULL);
    }
    VkShaderModule create_shader_module(const std::string& file_path) {
        std::vector<uint32_t> spirv_codes;
        std::ifstream spirv_file{file_path};
        while (!spirv_file.eof()) {
            uint32_t code;
            spirv_file.read(reinterpret_cast<char*>(&code), sizeof(code));
            if (spirv_file.gcount() < sizeof(code)) {
                break;
            }
            spirv_codes.push_back(code);
        }

        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = spirv_codes.size()*sizeof(spirv_codes[0]);
        create_info.pCode = spirv_codes.data();
        VkShaderModule shader_module;
        auto res = vkCreateShaderModule(m_device, &create_info, NULL, &shader_module);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module");
        }
        return shader_module;
    }
    void destroy_shader_module(VkShaderModule shader_module) {
        vkDestroyShaderModule(m_device, shader_module, NULL);
    }
    void create_descriptor_set_layout() {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        
        VkDescriptorSetLayoutCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.bindingCount = 1;
        create_info.pBindings = &binding;
        auto res = vkCreateDescriptorSetLayout(m_device, &create_info, NULL, &m_descriptor_set_layout);
        if (res != VK_SUCCESS) {
            throw std::runtime_error{"failed to create descriptor set layout"};
        }
    }
    void destroy_descriptor_set_layout() {
        vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, NULL);
    }
        
    void create_pipeline_layout() {
        VkPipelineLayoutCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.flags = 0;
        create_info.setLayoutCount = 1;
        create_info.pSetLayouts = &m_descriptor_set_layout;
        auto res = vkCreatePipelineLayout(m_device, &create_info, NULL, &m_pipeline_layout);
        if (res != VK_SUCCESS) {
            throw std::runtime_error{"failed to create pipeline layout"};
        }
    }
    void destroy_pipeline_layout() {
        vkDestroyPipelineLayout(m_device, m_pipeline_layout, NULL);
    }
    void create_pipeline() {
        VkComputePipelineCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        create_info.flags = 0;
        create_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        create_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        create_info.stage.module = create_shader_module("comp.spv");
        create_info.stage.pName = "main";
        create_info.layout = m_pipeline_layout;

        auto res = vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &create_info, NULL, &m_pipeline);
        destroy_shader_module(create_info.stage.module);
        if (res != VK_SUCCESS) {
            throw std::runtime_error{"failed to create compute pipeline"};
        }
    }
    void destroy_pipeline() {
        vkDestroyPipeline(m_device, m_pipeline, NULL);
    }
    void create_command_pool() {
        VkCommandPoolCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = m_queue_family;
        auto res = vkCreateCommandPool(m_device, &create_info, NULL, &m_command_pool);
        if (res != VK_SUCCESS) {
            throw std::runtime_error{"failed to create command pool"};
        }
    }
    void destroy_command_pool() {
        vkDestroyCommandPool(m_device, m_command_pool, NULL);
    }
    void allocate_command_buffer() {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = m_command_pool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = 1;
        auto ret = vkAllocateCommandBuffers(m_device, &info, &m_command_buffer);
        if (ret != VK_SUCCESS) {
            throw std::runtime_error{"failed to allocate command buffer"};
        }
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

    VkBuffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage) {
        VkBufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = size;
        create_info.usage = usage;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 1;
        create_info.pQueueFamilyIndices = &m_queue_family;
        
        VkBuffer buffer;
        auto res = vkCreateBuffer(m_device, &create_info, NULL, &buffer);
        if (res != VK_SUCCESS) {
            throw std::runtime_error{"failed to create buffer"};
        }
        return buffer;
    }
    void destroy_buffer(VkBuffer buffer) {
        vkDestroyBuffer(m_device, buffer, NULL);
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
    VkDeviceMemory alloc_device_memory(VkBuffer buffer, VkMemoryPropertyFlags property) {
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(m_device, buffer, &requirements);
        uint32_t memoryType = findProperties(requirements.memoryTypeBits, property);

        VkDeviceMemory device_memory{};
        {
            VkMemoryAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            info.allocationSize = requirements.size;
            info.memoryTypeIndex = memoryType;
            auto res = vkAllocateMemory(m_device, &info, NULL, &device_memory);
	    if (res != VK_SUCCESS) {
	      throw std::runtime_error{"failed to allocate device memory"};
	    }
        }

        vkBindBufferMemory(m_device, buffer, device_memory, 0);
        return device_memory;
    }
    void free_device_memory(VkDeviceMemory device_memory) {
        vkFreeMemory(m_device, device_memory, NULL);
    }
    void* map_device_memory(VkDeviceMemory device_memory, VkDeviceSize offset, VkDeviceSize size) {
        void* ptr{};
        auto res = vkMapMemory(m_device, device_memory, offset, size, 0, &ptr);
	if (res != VK_SUCCESS) {
	  throw std::runtime_error{"failed to map device memory"};
	}
        return ptr;
    }
    void unmap_device_memory(VkDeviceMemory device_memory) {
        vkUnmapMemory(m_device, device_memory);
    }
    void create_storage_buffer() {
        m_storage_buffer = create_buffer(128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        m_storage_memory = alloc_device_memory(m_storage_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_storage_memory_ptr = map_device_memory(m_storage_memory, 0, 128);
    }
    void destroy_storage_buffer() {
        destroy_buffer(m_storage_buffer);
        unmap_device_memory(m_storage_memory);
        free_device_memory(m_storage_memory);
    }

    void create_descriptor_pool() {
        VkDescriptorPoolSize pool_size{};
        pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_size.descriptorCount = 1;

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.maxSets = 1;
        info.poolSizeCount = 1;
        info.pPoolSizes = &pool_size;

        vkCreateDescriptorPool(m_device, &info, NULL, &m_descriptor_pool);
    }

    void destroy_descriptor_pool() {
        vkDestroyDescriptorPool(m_device, m_descriptor_pool, NULL);
    }

    void allocate_descriptor_set() {
        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = m_descriptor_pool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &m_descriptor_set_layout;
        auto res = vkAllocateDescriptorSets(m_device, &info, &m_descriptor_set);
        if (res != VK_SUCCESS) {
            throw std::runtime_error{"failed to allocate descriptor set"};
        }
    }
    void update_descriptor_set() {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = m_storage_buffer;
        buffer_info.offset = 0;
        buffer_info.range = 128;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_descriptor_set;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(m_device, 1, &write, 0, NULL);
    }
    void init() {
        create_instance();
        select_physical_device();
        select_queue_family();
        create_device();
        create_descriptor_set_layout();
        create_pipeline_layout();
        create_pipeline();
        create_descriptor_pool();
        allocate_descriptor_set();
        create_storage_buffer();
        update_descriptor_set();

        create_command_pool();
        allocate_command_buffer();
        record_command_buffer();
    }
    void deinit() {
        destroy_command_pool();
        
        destroy_storage_buffer();
        destroy_descriptor_pool();

        destroy_pipeline();
        destroy_pipeline_layout();
        destroy_descriptor_set_layout();
        destroy_device();
        destroy_instance();
    }
    void draw() {
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

        {
            auto res = vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{"wait fence fail"};
            }
        }
        {
            VkMappedMemoryRange memory_range{};
            memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memory_range.memory = m_storage_memory;
            memory_range.offset = 0;
            memory_range.size = 128;
            auto res = vkInvalidateMappedMemoryRanges(m_device, 1, &memory_range);
	    if (res != VK_SUCCESS) {
	      throw std::runtime_error{"failed to invalidate mapped memory"};
	    }
        }
        {
            uint32_t* data = reinterpret_cast<uint32_t*>(m_storage_memory_ptr);
            std::cout << data[0];
        }
        
    }
    void run() {
        while (1)
          draw();
    }
private:
    VkInstance m_instance;
    VkPhysicalDevice m_physical_device;
    uint32_t m_queue_family;
    VkDevice m_device;
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
