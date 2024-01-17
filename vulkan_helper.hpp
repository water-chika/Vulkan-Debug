#pragma once

#include <vulkan/vulkan.h>

#include "spirv_helper.hpp"

#include <concepts>

namespace vulkan_helper {
	class instance {
    public:
        instance() {
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
        ~instance() {
            vkDestroyInstance(m_instance, NULL);
        }
        VkPhysicalDevice get_first_physical_device() {
            uint32_t count = 1;
            VkPhysicalDevice physical_device;
            auto res = vkEnumeratePhysicalDevices(m_instance, &count, &physical_device);
            return physical_device;
        }
        template<int NUM>
        uint32_t enumerate_limit_physical_devices(std::array<VkPhysicalDevice, NUM>& physical_devices) {
            uint32_t count = physical_devices.size();
            auto res = vkEnumeratePhysicalDevices(m_instance, &count, physical_devices.data());
            if (res != VK_INCOMPLETE || res != VK_SUCCESS) {
                throw std::runtime_error{ "failed to enumerate physical devices" };
            }
            return count;
        }
        void foreach_physical_device(std::invocable<VkPhysicalDevice> auto&& fun) {
            constexpr uint32_t COUNT = 8;
            std::array<VkPhysicalDevice, COUNT> physical_devices{};
            uint32_t count = COUNT;
            auto res = vkEnumeratePhysicalDevices(m_instance, &count, physical_devices.data());
            if (res == VK_INCOMPLETE) {
                throw std::runtime_error("too more physical device");
            }
            else if (res != VK_SUCCESS) {
                throw std::runtime_error{ "failed to enumerate physical devices" };
            }
            for (int i = 0; i < count; i++) {
                fun(physical_devices[i]);
            }
        }
        VkInstance m_instance;
	};

    template<class T>
    class info_chain {
    public:
        
    private:
        T info;
        std::unique_ptr<void> next;
    };

    class device_create_info {
    public:
        constexpr device_create_info() : m_create_info{} {
            m_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        }
        void enable_synchronization2() {
            if (m_create_info.pNext == nullptr) {
                auto* vulkan_1_3_features = new VkPhysicalDeviceVulkan13Features;
                vulkan_1_3_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
                vulkan_1_3_features->synchronization2 = VK_TRUE;
                auto* features2 = new VkPhysicalDeviceFeatures2;
                features2->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                features2->pNext = vulkan_1_3_features;
            }
            else {

            }
        }
        void enable_maintenance4() {
            if (m_create_info.pNext == nullptr) {
                auto* vulkan_1_3_features = new VkPhysicalDeviceVulkan13Features;
                vulkan_1_3_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
                vulkan_1_3_features->maintenance4 = VK_TRUE;
                auto* features2 = new VkPhysicalDeviceFeatures2;
                features2->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                features2->pNext = vulkan_1_3_features;
            }
            else {

            }
        }
        void set_queue_family_index(uint32_t index) {
            m_queue_family_index = index;
        }
        uint32_t get_queue_family_index() const {
            return m_queue_family_index;
        }
    private:
        VkDeviceCreateInfo m_create_info;
        int m_queue_family_index;
    };
    class physical_device : public instance{
    public:
        physical_device(std::invocable<instance&> auto&& select_physical_device) : m_physical_device{ select_physical_device(*this)} {}

        uint32_t find_queue_family_if(std::predicate<VkQueueFamilyProperties> auto&& fun) {
            constexpr uint32_t COUNT = 8;
            std::array<VkQueueFamilyProperties, COUNT> properties{};
            uint32_t count = COUNT;
            vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &count, properties.data());
            assert(count > 0);
            for (uint32_t i = 0; i < count; i++) {
                auto& property = properties[i];
                if (fun(property)) {
                    return i;
                }
            }
            throw std::runtime_error{ "failed to find queue family" };
        }
        auto get_physical_device_memory_properties() {
            VkPhysicalDeviceMemoryProperties properties;
            vkGetPhysicalDeviceMemoryProperties(m_physical_device, &properties);
            return properties;
        }
        auto get_memory_properties() {
            return get_physical_device_memory_properties();
        }
        auto create_device(const device_create_info& info) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = info.get_queue_family_index();
            queue_create_info.queueCount = 1;
            float priority = 1.0;
            queue_create_info.pQueuePriorities = &priority;

            VkPhysicalDeviceVulkan13Features vulkan_1_3_features{};
            vulkan_1_3_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            vulkan_1_3_features.synchronization2 = VK_TRUE;
            vulkan_1_3_features.maintenance4 = VK_TRUE;

            VkPhysicalDeviceFeatures2 features2{};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features2.pNext = &vulkan_1_3_features;

            VkDeviceCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            create_info.pNext = &features2;
            create_info.queueCreateInfoCount = 1;
            create_info.pQueueCreateInfos = &queue_create_info;

            VkDevice device;
            auto res = vkCreateDevice(m_physical_device, &create_info, NULL, &device);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{ "failed to create device" };
            }
            return device;
        }
        auto create_device() {
            device_create_info info{};
            info.set_queue_family_index(0);
            return create_device(info);
        }
        VkPhysicalDevice operator()(VkPhysicalDevice) {
            return m_physical_device;
        }
    private:
        VkPhysicalDevice m_physical_device;
    };

    template<std::derived_from<physical_device> PD = physical_device>
    class device : public PD {
    public:
        device(std::invocable<PD&> auto&& gen_info)
            :
            m_device{ PD::create_device(gen_info(*this)) }
        {}
        device() = delete;
        device(const device& device) = delete;
        device(device&& device) = delete;
        ~device() noexcept {
            vkDestroyDevice(m_device, nullptr);
        }
        device& operator=(const device& device) = delete;
        device& operator=(device&& device) = delete;

        VkQueue get_device_queue(uint32_t queue_family_index, uint32_t queue_index) {
            VkQueue queue;
            vkGetDeviceQueue(m_device, queue_family_index, queue_index, &queue);
            return queue;
        }
        VkFence create_fence() {
            VkFenceCreateInfo fence_create_info{};
            VkFence fence;
            {
                auto& info = fence_create_info;
                info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                auto res = vkCreateFence(m_device, &fence_create_info, NULL, &fence);
                if (res != VK_SUCCESS) {
                    throw std::runtime_error{ "failed to create fence" };
                }
            }
            return fence;
        }
        void destroy_fence(VkFence fence) {
            vkDestroyFence(m_device, fence, nullptr);
        }
        VkShaderModule create_shader_module(const spirv_file& file) {
            VkShaderModuleCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = file.size();
            create_info.pCode = file.data();
            VkShaderModule shader_module;
            auto res = vkCreateShaderModule(m_device, &create_info, NULL, &shader_module);
            if (res != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module");
            }
            return shader_module;
        }

        void destroy_shader_module(VkShaderModule shader_module) {
            vkDestroyShaderModule(m_device, shader_module, nullptr);
        }

        auto create_descriptor_set_layout() {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = 0;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = 1;
            create_info.pBindings = &binding;

            VkDescriptorSetLayout descriptor_set_layout;
            auto res = vkCreateDescriptorSetLayout(m_device, &create_info, NULL, &descriptor_set_layout);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{ "failed to create descriptor set layout" };
            }
            return descriptor_set_layout;
        }
        void destroy_descriptor_set_layout(VkDescriptorSetLayout layout) {
            vkDestroyDescriptorSetLayout(m_device, layout, NULL);
        }

        auto create_pipeline_layout(VkDescriptorSetLayout descriptor_set_layout) {
            VkPipelineLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            create_info.flags = 0;
            create_info.setLayoutCount = 1;
            create_info.pSetLayouts = &descriptor_set_layout;
            VkPipelineLayout pipeline_layout;
            auto res = vkCreatePipelineLayout(m_device, &create_info, NULL, &pipeline_layout);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{ "failed to create pipeline layout" };
            }
            return pipeline_layout;
        }
        void destroy_pipeline_layout(VkPipelineLayout pipeline_layout) {
            vkDestroyPipelineLayout(m_device, pipeline_layout, NULL);
        }
        auto create_pipeline(VkShaderModule shader_module, VkPipelineLayout pipeline_layout) {
            VkComputePipelineCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            create_info.flags = 0;
            create_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            create_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            create_info.stage.module = shader_module;
            create_info.stage.pName = "main";
            create_info.layout = pipeline_layout;

            VkPipeline pipeline;
            auto res = vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{ "failed to create compute pipeline" };
            }
            return pipeline;
        }
        void destroy_pipeline(VkPipeline pipeline) {
            vkDestroyPipeline(m_device, pipeline, NULL);
        }

        auto create_command_pool(uint32_t queue_family_index) {
            VkCommandPoolCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            create_info.queueFamilyIndex = queue_family_index;
            VkCommandPool command_pool;
            auto res = vkCreateCommandPool(m_device, &create_info, NULL, &command_pool);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{ "failed to create command pool" };
            }
            return command_pool;
        }
        void destroy_command_pool(VkCommandPool command_pool) {
            vkDestroyCommandPool(m_device, command_pool, NULL);
        }

        auto allocate_command_buffer(VkCommandPool command_pool) {
            VkCommandBufferAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            info.commandPool = command_pool;
            info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            info.commandBufferCount = 1;
            VkCommandBuffer command_buffer;
            auto ret = vkAllocateCommandBuffers(m_device, &info, &command_buffer);
            if (ret != VK_SUCCESS) {
                throw std::runtime_error{ "failed to allocate command buffer" };
            }
            return command_buffer;
        }

        VkBuffer create_buffer(uint32_t queue_family_index, VkDeviceSize size, VkBufferUsageFlags usage) {
            VkBufferCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            create_info.size = size;
            create_info.usage = usage;
            create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 1;
            create_info.pQueueFamilyIndices = &queue_family_index;

            VkBuffer buffer;
            auto res = vkCreateBuffer(m_device, &create_info, NULL, &buffer);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{ "failed to create buffer" };
            }
            return buffer;
        }
        void destroy_buffer(VkBuffer buffer) {
            vkDestroyBuffer(m_device, buffer, NULL);
        }

        uint32_t findProperties(VkPhysicalDeviceMemoryProperties memory_properties, uint32_t memoryTypeBitsRequirements, VkMemoryPropertyFlags requiredProperty) {
            const uint32_t memoryCount = memory_properties.memoryTypeCount;
            for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; memoryIndex++) {
                const uint32_t memoryTypeBits = (1 << memoryIndex);
                const bool isRequiredMemoryType = memoryTypeBitsRequirements & memoryTypeBits;
                const VkMemoryPropertyFlags properties =
                    memory_properties.memoryTypes[memoryIndex].propertyFlags;
                const bool hasRequiredProperties =
                    (properties & requiredProperty) == requiredProperty;
                if (isRequiredMemoryType && hasRequiredProperties)
                    return memoryIndex;
            }
            throw std::runtime_error{ "failed find memory property" };
        }
        VkDeviceMemory alloc_device_memory(VkPhysicalDeviceMemoryProperties memory_properties,  VkBuffer buffer, VkMemoryPropertyFlags property) {
            VkMemoryRequirements requirements;
            vkGetBufferMemoryRequirements(m_device, buffer, &requirements);
            uint32_t memoryType = findProperties(memory_properties, requirements.memoryTypeBits, property);

            VkDeviceMemory device_memory{};
            {
                VkMemoryAllocateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                info.allocationSize = requirements.size;
                info.memoryTypeIndex = memoryType;
                auto res = vkAllocateMemory(m_device, &info, NULL, &device_memory);
                if (res != VK_SUCCESS) {
                    throw std::runtime_error{ "failed to allocate device memory" };
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
                throw std::runtime_error{ "failed to map device memory" };
            }
            return ptr;
        }
        void unmap_device_memory(VkDeviceMemory device_memory) {
            vkUnmapMemory(m_device, device_memory);
        }

        auto create_descriptor_pool() {
            VkDescriptorPoolSize pool_size{};
            pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            pool_size.descriptorCount = 1;

            VkDescriptorPoolCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            info.maxSets = 1;
            info.poolSizeCount = 1;
            info.pPoolSizes = &pool_size;

            VkDescriptorPool descriptor_pool;
            vkCreateDescriptorPool(m_device, &info, NULL, &descriptor_pool);
            return descriptor_pool;
        }

        void destroy_descriptor_pool(VkDescriptorPool descriptor_pool) {
            vkDestroyDescriptorPool(m_device, descriptor_pool, NULL);
        }

        auto allocate_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout) {
            VkDescriptorSetAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            info.descriptorPool = descriptor_pool;
            info.descriptorSetCount = 1;
            info.pSetLayouts = &descriptor_set_layout;
            VkDescriptorSet descriptor_set;
            auto res = vkAllocateDescriptorSets(m_device, &info, &descriptor_set);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{ "failed to allocate descriptor set" };
            }
            return descriptor_set;
        }
        void update_descriptor_set(VkBuffer storage_buffer, VkDescriptorSet descriptor_set) {
            VkDescriptorBufferInfo buffer_info{};
            buffer_info.buffer = storage_buffer;
            buffer_info.offset = 0;
            buffer_info.range = 128;
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptor_set;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.pBufferInfo = &buffer_info;

            vkUpdateDescriptorSets(m_device, 1, &write, 0, NULL);
        }

        void reset_fence(VkFence fence) {
            if (VK_SUCCESS != vkResetFences(m_device, 1, &fence)) {
                throw std::runtime_error{ "failed to reset fence" };
            }
        }

        void wait_for_fence(VkFence fence) {
            auto res = vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{ "wait fence fail" };
            }
        }

        void invalidate_mapped_memory_ranges(VkDeviceMemory memory, uint32_t offset, uint32_t size) {
            VkMappedMemoryRange memory_range{};
            memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memory_range.memory = memory;
            memory_range.offset = offset;
            memory_range.size = size;
            auto res = vkInvalidateMappedMemoryRanges(m_device, 1, &memory_range);
            if (res != VK_SUCCESS) {
                throw std::runtime_error{ "failed to invalidate mapped memory" };
            }
        }

    private:
        VkDevice m_device;
    };

    template<class D>
    class command_pool : public D{
    public:
        command_pool(std::invocable<D&> auto&& gen_command_pool) : m_command_pool { gen_command_pool(*this) }
        {}
        command_pool() = delete;
        command_pool(const command_pool&) = delete;
        command_pool(command_pool&&) = delete;
        ~command_pool() {
            D::destroy_command_pool(m_command_pool);
        }
        command_pool& operator=(const command_pool&) = delete;
        command_pool& operator=(command_pool&&) = delete;

        VkCommandPool get_command_pool() {
            return m_command_pool;
        }

    private:
        VkCommandPool m_command_pool;
    };

    template<class D>
    class fence : public D {
    public:
        fence() : m_fence{D::create_fence()}
        {}
        ~fence() {
            D::destroy_fence(m_fence);
        }
        VkFence get_fence() {
            return m_fence;
        }

        void reset() {
            D::reset_fence(m_fence);
        }
        void wait_for() {
            D::wait_for_fence(m_fence);
        }
    private:
        VkFence m_fence;
    };

    template<class D>
    class descriptor_set_layout : public D {
    public:
        descriptor_set_layout() : m_descriptor_set_layout{
            D::create_descriptor_set_layout()
        }
        {}
        ~descriptor_set_layout() {
            D::destroy_descriptor_set_layout(m_descriptor_set_layout);
        }

        const auto get_descriptor_set_layout() const {
            return m_descriptor_set_layout;
        }
    private:
        VkDescriptorSetLayout m_descriptor_set_layout;
    };

    template<class D>
    class descriptor_pool : public D {
    public:
        descriptor_pool() : m_descriptor_pool{D::create_descriptor_pool()}
        {}
        ~descriptor_pool() {
            D::destroy_descriptor_pool(m_descriptor_pool);
        }
        auto allocate_descriptor_set(VkDescriptorSetLayout layout) {
            return D::allocate_descriptor_set(m_descriptor_pool, layout);
        }

        auto get_descriptor_pool() {
            return m_descriptor_pool;
        }
    private:
        VkDescriptorPool m_descriptor_pool;
    };

    template<class D>
    class descriptor_set : public D {
    public:
    public:
        descriptor_set() : m_descriptor_set{ D::allocate_descriptor_set(D::get_descriptor_set_layout())}
        {}

        auto get_descriptor_set() {
            return m_descriptor_set;
        }
    private:
        VkDescriptorSet m_descriptor_set;
    };

    template<class D>
    class pipeline_layout : public D {
    public:
        pipeline_layout() : m_pipeline_layout{D::create_pipeline_layout(D::get_descriptor_set_layout())}
        {}
        ~pipeline_layout() {
            D::destroy_pipeline_layout(m_pipeline_layout);
        }
        auto get_pipeline_layout() const{
            return m_pipeline_layout;
        }
    private:
        VkPipelineLayout m_pipeline_layout;
    };

};