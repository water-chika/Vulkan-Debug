#include <stdexcept>
#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>
#include <array>
#include <vulkan/vulkan.h>
#include "vulkan_helper.hpp"
#include "spirv_helper.hpp"

template<vulkan_helper::concept_helper::instance instance>
requires vulkan_helper::concept_helper::instance_help_functions<instance>
class first_physical_device : public instance {
public:
    first_physical_device() : instance{}
    {
        m_physical_device = instance::get_first_physical_device();
    }
    auto get_vulkan_physical_device() {
        return m_physical_device;
    }
private:
    VkPhysicalDevice m_physical_device;
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
template<class D>
class add_image : public D {
public:
    add_image() : D{} {
        VkImageCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        create_info.arrayLayers = 1;
        create_info.extent = VkExtent3D{ 151, 151, 1 };
        create_info.format = VK_FORMAT_R32G32B32A32_UINT;
        create_info.imageType = VK_IMAGE_TYPE_2D;
        create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        create_info.mipLevels = 1;
        create_info.samples = VK_SAMPLE_COUNT_8_BIT;
        create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        m_image = D::create_image(&create_info);
    }
    ~add_image() {
        D::destroy_image(m_image);
    }
protected:
    VkImage m_image;
};

template<class D>
class add_image_memory : public D {
public:
    add_image_memory() : D{} {
        m_image_memory = D::alloc_device_memory(D::get_memory_properties(), D::m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
    ~add_image_memory() {
        D::free_device_memory(m_image_memory);
    }
protected:
    VkDeviceMemory m_image_memory;
};

template<class D>
class add_image_view : public D {
public:
    add_image_view() : D{} {
        VkImageViewCreateInfo		imageViewCreateInfo =
        {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType			sType;
            NULL,									// const void*				pNext;
            (VkImageViewCreateFlags)0u,					// VkImageViewCreateFlags	flags;
            D::m_image,									// VkImage					image;
            VK_IMAGE_VIEW_TYPE_2D_ARRAY,				// VkImageViewType			viewType;
            VK_FORMAT_R32G32B32A32_UINT,									// VkFormat					format;
            {
                VK_COMPONENT_SWIZZLE_R,					// VkComponentSwizzle	r;
                VK_COMPONENT_SWIZZLE_G,					// VkComponentSwizzle	g;
                VK_COMPONENT_SWIZZLE_B,					// VkComponentSwizzle	b;
                VK_COMPONENT_SWIZZLE_A					// VkComponentSwizzle	a;
            },											// VkComponentMapping		 components;
            {
                VK_IMAGE_ASPECT_COLOR_BIT,				// VkImageAspectFlags	aspectMask;
                0u,										// deUint32				baseMipLevel;
                1u,										// deUint32				levelCount;
                0u,										// deUint32				baseArrayLayer;
                1					// deUint32				layerCount;
            }											// VkImageSubresourceRange	subresourceRange;
        };
        m_image_view = D::create_image_view(&imageViewCreateInfo);
    }
    ~add_image_view() {
        D::destroy_image_view(m_image_view);
    }
protected:
    VkImageView m_image_view;
};

template<class D>
class add_descriptor_set_layout : public D {
public:
    add_descriptor_set_layout() : D{} {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding binding2{};
        binding2.binding = 1;
        binding2.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        binding2.descriptorCount = 1;
        binding2.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        auto bindings = std::array{ binding, binding2 };

        VkDescriptorSetLayoutCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.bindingCount = bindings.size();
        create_info.pBindings = bindings.data();

        m_descriptor_set_layout = D::create_descriptor_set_layout(&create_info);
    }
    ~add_descriptor_set_layout() {
        D::destroy_descriptor_set_layout(m_descriptor_set_layout);
    }
    VkDescriptorSetLayout get_descriptor_set_layout() {
        return m_descriptor_set_layout;
    }
private:
    VkDescriptorSetLayout m_descriptor_set_layout;
};
template<class D>
class add_descriptor_pool : public D {
public:
    add_descriptor_pool() : D{} {
        VkDescriptorPoolSize pool_size{};
        pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_size.descriptorCount = 1;

        VkDescriptorPoolSize pool_size2{};
        pool_size2.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        pool_size2.descriptorCount = 1;

        auto pool_sizes = std::array{ pool_size, pool_size2 };

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.maxSets = 1;
        info.poolSizeCount = pool_sizes.size();
        info.pPoolSizes = pool_sizes.data();
        m_descriptor_pool = D::create_descriptor_pool(&info);
    }
    ~add_descriptor_pool() {
        D::destroy_descriptor_pool(m_descriptor_pool);
    }
    auto get_descriptor_pool() {
        return m_descriptor_pool;
    }
    auto allocate_descriptor_set(VkDescriptorSetLayout layout) {
        return D::allocate_descriptor_set(m_descriptor_pool, layout);
    }
private:
    VkDescriptorPool m_descriptor_pool;
};

template<class D>
class update_descriptor_set : public D {
public:
    update_descriptor_set() : D{} {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = D::get_storage_buffer();
        buffer_info.offset = 0;
        buffer_info.range = VK_WHOLE_SIZE;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = D::get_descriptor_set();
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &buffer_info;
        D::update_descriptor_set(write);
        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = D::m_image_view;
        image_info.sampler = NULL;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageInfo = &image_info;
        write.dstBinding = 1;
        D::update_descriptor_set(write);
    }
private:
};

using app_parent = 
    update_descriptor_set<
    add_image_view<
    add_image_memory<
    add_image<
    vulkan_helper::add_storage_memory_ptr<
    vulkan_helper::add_storage_memory<
    vulkan_helper::add_storage_buffer< 151 * 151 * 8 * 4 * sizeof(uint32_t),
    vulkan_helper::command_buffer<
    add_compute_command_pool<
    vulkan_helper::fence<
    physical_device_cached_memory_properties<
    app_pipeline<
    vulkan_helper::pipeline_layout<
    vulkan_helper::descriptor_set<
    add_descriptor_pool<
    add_descriptor_set_layout<
    compute_queue
    >>>>>>>>>>>>>>>>;


class App : public app_parent{
public:
    App()
    {
        record_command_buffer();
    }

    void record_command_buffer() {
        command_buffer::begin();
        VkImageMemoryBarrier imageBarrier
        {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType		sType
            NULL,									// const void*			pNext
            0u,											// VkAccessFlags		srcAccessMask
            VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags		dstAccessMask
            VK_IMAGE_LAYOUT_UNDEFINED,					// VkImageLayout		oldLayout
            VK_IMAGE_LAYOUT_GENERAL,					// VkImageLayout		newLayout
            VK_QUEUE_FAMILY_IGNORED,					// uint32_t				srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,					// uint32_t				dstQueueFamilyIndex
            app_parent::m_image,								// VkImage				image
            {
                VK_IMAGE_ASPECT_COLOR_BIT,				// VkImageAspectFlags	aspectMask
                0u,										// uint32_t				baseMipLevel
                VK_REMAINING_MIP_LEVELS,				// uint32_t				mipLevels,
                0u,										// uint32_t				baseArray
                VK_REMAINING_ARRAY_LAYERS,				// uint32_t				arraySize
            }
        };

        command_buffer::pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            (VkDependencyFlags)0,
            0, (const VkMemoryBarrier*)NULL,
            0, (const VkBufferMemoryBarrier*)NULL,
            1, &imageBarrier);


        VkClearValue				clearColor{ .color = {.uint32 = {0, 0, 0, 0}} };
        // Clear color buffer to transparent black
        {
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0u;
            range.levelCount = 1u;
            range.baseArrayLayer = 0u;
            range.layerCount = 1;
            command_buffer::clear_color_image(app_parent::m_image, VK_IMAGE_LAYOUT_GENERAL, &clearColor.color, 1, &range);
        }


        VkMemoryBarrier memBarrier
        {
            VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            NULL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
        };
        command_buffer::pipeline_barrier(VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memBarrier, 0, NULL, 0, NULL);

        command_buffer::bind_descriptor_sets(VK_PIPELINE_BIND_POINT_COMPUTE, app_parent::get_pipeline_layout(), app_parent::get_descriptor_set() );
        command_buffer::bind_pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, app_parent::get_pipeline());

        // Copy color/depth/stencil buffers to buffer memory
        command_buffer::dispatch(151, 151, 1);

        memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        command_buffer::pipeline_barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
            0, 1, &memBarrier, 0, NULL, 0, NULL);

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
            app_parent::invalidate_mapped_memory_ranges(app_parent::get_storage_memory(), 0, VK_WHOLE_SIZE);
            uint32_t* data = reinterpret_cast<uint32_t*>(app_parent::get_storage_memory_ptr());
            for (int y = 0; y < 151; y++) {
                for (int x = 0; x < 151; x++) {
                    for (int s = 0; s < 8; s++) {
                        uint32_t* sample = &data[4*((y * 151 + x) * 8 + s)];
                        assert(sample[0] == 0);
                    }
                }
            }
            std::cout << data[(0x80*151 + 0x86)*4*8] << std::endl;
        }
        
    }
    void run() {
      for (int i = 0; i < 1; i++)
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
