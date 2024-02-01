#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_shared.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cassert>
#include <array>
#include <span>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <filesystem>

#define max max
#include "spirv_reader.hpp"

template<class T>
class from_0_count_n {
public:
	struct T_ite {
		using difference_type = int64_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::random_access_iterator_tag;
		using iterator_concept = std::contiguous_iterator_tag;
		constexpr T operator*() const noexcept { return value; }
		constexpr T_ite& operator++() noexcept { ++value; return *this; }
		constexpr T_ite operator++(int) noexcept { return T_ite{ value++ }; }
		constexpr T_ite& operator--() noexcept { --value; return *this; }
		constexpr T_ite operator--(int) noexcept { return T_ite{ value-- }; }
		constexpr bool operator==(T_ite rhs) const noexcept{
			return value == rhs.value;
		}
		constexpr int64_t operator-(T_ite rhs) const noexcept {
			return static_cast<int64_t>(value) - rhs.value;
		}
		T value;
	};
	constexpr from_0_count_n(T count) : m_count{ count } {}
	constexpr T size() noexcept { return m_count; }
	constexpr T_ite begin() noexcept { return T_ite{ 0 }; }
	constexpr T_ite end() noexcept { return T_ite{ m_count }; }
private:
	T m_count;
};

template<class T>
inline constexpr bool std::ranges::enable_borrowed_range<from_0_count_n<T>> = true;

namespace terminal {
	auto create_instance() {
		vk::ApplicationInfo applicationInfo("Terminal Emulator", 1, nullptr, 0, VK_API_VERSION_1_3);
		std::array<const char*, 0> instanceLayers{};
		std::array<const char*, 2> instanceExtensions{ "VK_KHR_surface", "VK_KHR_win32_surface" };
		vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo, instanceLayers, instanceExtensions);
		return vk::SharedInstance{ vk::createInstance(instanceCreateInfo) };
	}
	auto select_physical_device(vk::SharedInstance instance) {
		auto devices = instance->enumeratePhysicalDevices();
		return devices.front();
	}
	auto select_queue_family(vk::PhysicalDevice physical_device) {
		uint32_t graphicsQueueFamilyIndex = 0;
		auto queueFamilyProperties = physical_device.getQueueFamilyProperties();
		for (int i = 0; i < queueFamilyProperties.size(); i++) {
			if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
				graphicsQueueFamilyIndex = i;
			}
		}
		return graphicsQueueFamilyIndex;
	}
	auto create_window_and_get_surface(vk::Instance instance, uint32_t width, uint32_t height) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		assert(true == glfwVulkanSupported());
		GLFWwindow* window = glfwCreateWindow(width, height, "Terminal Emulator", nullptr, nullptr);

		VkSurfaceKHR surface;
		glfwCreateWindowSurface(instance, window, nullptr, &surface);
		return std::pair{ window, surface };
	}
	auto create_device(vk::PhysicalDevice physical_device, uint32_t graphicsQueueFamilyIndex) {
		float queuePriority = 0.0f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, graphicsQueueFamilyIndex, 1, &queuePriority);
		std::array<const char*, 0> deviceLayers{};
		std::array<const char*, 1> deviceExtensions{ "VK_KHR_swapchain" };
		vk::DeviceCreateInfo deviceCreateInfo({}, deviceQueueCreateInfo, deviceLayers, deviceExtensions);

		return physical_device.createDevice(deviceCreateInfo);
	}
	auto select_depth_image_tiling(vk::PhysicalDevice physical_device, vk::Format format) {
		vk::FormatProperties format_properties = physical_device.getFormatProperties(format);
		if (format_properties.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
			return vk::ImageTiling::eLinear;
		}
		else if (format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
			return vk::ImageTiling::eOptimal;
		}
		else {
			throw std::runtime_error{ "DepthStencilAttachment is not supported for this format" };
		}
	}
	auto create_image(vk::Device device, vk::ImageType type, vk::Format format, vk::Extent2D extent, vk::ImageTiling tiling, vk::ImageUsageFlags usages) {
		vk::ImageCreateInfo create_info{ {}, type, format, vk::Extent3D{extent, 1}, 1, 1, vk::SampleCountFlagBits::e1, tiling, usages };
		return device.createImage(create_info);
	}
	template<class T, class B>
	bool contain_bit(T bitfield, B bit) {
		return (bitfield & bit) == bit;
	}

	auto select_memory_type(vk::PhysicalDevice physical_device, vk::Device device, vk::MemoryRequirements requirements, vk::MemoryPropertyFlags properties) {
		auto memory_properties = physical_device.getMemoryProperties();
		auto type_bits = requirements.memoryTypeBits;
		return *std::ranges::find_if(from_0_count_n(memory_properties.memoryTypeCount), [&memory_properties, type_bits, properties](auto i) {
			if ((type_bits & (1 << i)) &&
				contain_bit(memory_properties.memoryTypes[i].propertyFlags, properties)) {
				return true;
			}
			return false;
			});
	}
	auto allocate_device_memory(vk::PhysicalDevice physical_device, vk::Device device, vk::Image image, vk::MemoryPropertyFlags properties) {

		auto memory_requirements = device.getImageMemoryRequirements(image);
		auto type_index = select_memory_type(physical_device, device, memory_requirements, properties);
		return std::tuple{ device.allocateMemory(vk::MemoryAllocateInfo{ memory_requirements.size, type_index }), memory_requirements.size };
	}
	auto allocate_device_memory(vk::PhysicalDevice physical_device, vk::Device device, vk::Buffer buffer, vk::MemoryPropertyFlags properties) {

		auto memory_requirements = device.getBufferMemoryRequirements(buffer);
		auto type_index = select_memory_type(physical_device, device, memory_requirements, properties);
		return std::tuple{ device.allocateMemory(vk::MemoryAllocateInfo{ memory_requirements.size, type_index }), memory_requirements.size };
	}
	auto create_image_view(vk::Device device, vk::Image image, vk::ImageViewType type, vk::Format format, vk::ImageAspectFlags aspect) {
		return device.createImageView(vk::ImageViewCreateInfo{ {}, image, type, format, {}, {aspect, 0, 1, 0, 1} });
	}
	auto create_depth_buffer(vk::PhysicalDevice physical_device, vk::Device device, uint32_t width, uint32_t height) {
		const vk::Format format = vk::Format::eD16Unorm;
		vk::ImageTiling tiling = select_depth_image_tiling(physical_device, format);
		auto image = create_image(device, vk::ImageType::e2D, format, vk::Extent2D{ width, height }, tiling, vk::ImageUsageFlagBits::eDepthStencilAttachment);
		auto [memory,memory_size] = allocate_device_memory(physical_device, device, image, vk::MemoryPropertyFlagBits::eDeviceLocal);
		device.bindImageMemory(image, memory, 0);
		auto image_view = create_image_view(device, image, vk::ImageViewType::e2D, format, vk::ImageAspectFlagBits::eDepth);
		return std::tuple{ image, memory, image_view };
	}
	auto create_buffer(vk::Device device, size_t size, vk::BufferUsageFlags usages) {
		return device.createBuffer(vk::BufferCreateInfo{ {}, size, usages });
	}
	void copy_to_buffer(vk::Device device, vk::Buffer buffer, vk::DeviceMemory memory, std::span<char> data) {
		auto memory_requirement = device.getBufferMemoryRequirements(buffer);
		auto* ptr = static_cast<uint8_t*>(device.mapMemory(memory, 0, memory_requirement.size));
		std::memcpy(ptr, data.data(), data.size());
		device.unmapMemory(memory);
	}
	auto create_uniform_buffer(vk::PhysicalDevice physical_device, vk::Device device, std::span<char> mem) {
		auto buffer = create_buffer(device, mem.size(), vk::BufferUsageFlagBits::eUniformBuffer);
		auto [memory,memory_size] = allocate_device_memory(physical_device, device, buffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		device.bindBufferMemory(buffer, memory, 0);
		copy_to_buffer(device, buffer, memory, mem);
		return std::tuple{ buffer, memory, memory_size };
	}
	auto create_vertex_buffer(vk::PhysicalDevice physical_device, vk::Device device, std::span<char> vertices) {
		auto buffer = create_buffer(device, vertices.size(), vk::BufferUsageFlagBits::eVertexBuffer);
		auto [memory, memory_size] = allocate_device_memory(physical_device, device, buffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		device.bindBufferMemory(buffer, memory, 0);
		copy_to_buffer(device, buffer, memory, vertices);
		return std::tuple{ buffer, memory, memory_size };
	}
	auto create_pipeline_layout(vk::Device device) {
		return device.createPipelineLayout(vk::PipelineLayoutCreateInfo{});
	}
	auto create_render_pass(vk::Device device, vk::Format colorFormat, vk::Format depthFormat) {
		std::array<vk::AttachmentDescription, 2> attachmentDescriptions;
		attachmentDescriptions[0] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
			colorFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR);
		attachmentDescriptions[1] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
			depthFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
		vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
		vk::SubpassDescription  subpass(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, {}, colorReference, {}, &depthReference);

		vk::RenderPass renderPass = device.createRenderPass(vk::RenderPassCreateInfo(vk::RenderPassCreateFlags(), attachmentDescriptions, subpass));
		return renderPass;
	}
	auto create_shader_module(vk::Device device, std::filesystem::path path) {
		spirv_file file{ path };
		std::span code{ file.data(), file.size() };
		return device.createShaderModule(vk::ShaderModuleCreateInfo{ {}, code });
	}
	auto create_framebuffer(vk::Device device, vk::RenderPass render_pass, std::vector<vk::ImageView> attachments, vk::Extent2D extent) {
		return device.createFramebuffer(vk::FramebufferCreateInfo{ {}, render_pass, attachments, extent.width, extent.height });
	}
	struct vertex_stage_info {
		std::filesystem::path shader_file_path;
		std::string entry_name;
		vk::VertexInputBindingDescription input_binding;
		std::vector<vk::VertexInputAttributeDescription> input_attributes;
	};
	auto create_pipeline(vk::Device device, vertex_stage_info vertex_stage, std::filesystem::path fragment_shader,
		vk::VertexInputBindingDescription, std::vector<vk::VertexInputAttributeDescription> vertex_input_attributeDescriptions) {
		auto vertex_shader_module = create_shader_module(device, vertex_stage.shader_file_path);
		auto fragment_shader_module = create_shader_module(device, fragment_shader);
		std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stage_create_infos = {
			vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eVertex, vertex_shader_module, vertex_stage.entry_name.c_str()},
			vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eFragment, fragment_shader_module, "main"},
		};
		vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{ {}, vertex_stage.input_binding, vertex_stage.input_attributes };
		vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{ {}, vk::PrimitiveTopology::eTriangleList };
		vk::PipelineViewportStateCreateInfo viewport_state_create_info{ {}, 1, nullptr, 1, nullptr };
		vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{ {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f };
		vk::PipelineMultisampleStateCreateInfo multisample_state_create_info{ {}, vk::SampleCountFlagBits::e1 };
		vk::StencilOpState stencil_op_state{ vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways };
		vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{ {}, true, true, vk::CompareOp::eLessOrEqual, false, false, stencil_op_state, stencil_op_state };
		vk::PipelineColorBlendAttachmentState color_blend_attachment_state{ false };
		vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info{ {}, false, vk::LogicOp::eNoOp, color_blend_attachment_state, {{1.0f, 1.0f, 1.0f, 1.0f}} };
		std::array<vk::DynamicState, 2> dynamic_states = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamic_state_create_info{ vk::PipelineDynamicStateCreateFlags{}, dynamic_states };
		return device.createGraphicsPipeline(nullptr, 
			vk::GraphicsPipelineCreateInfo{ {}, 
				shader_stage_create_infos ,&vertex_input_state_create_info, &input_assembly_state_create_info,
				nullptr, &viewport_state_create_info, &rasterization_state_create_info, &multisample_state_create_info, 
				&depth_stencil_state_create_info, &color_blend_state_create_info, &dynamic_state_create_info });
	}
}

int main() {
	struct glfwContext
	{
		glfwContext()
		{
			glfwInit();
			glfwSetErrorCallback(
				[](int error, const char* msg)
				{
					std::cerr << "glfw: " << "(" << error << ") " << msg << std::endl;
				}
			);
		}
		~glfwContext() {
			glfwTerminate();
		}
	};
	auto glfwCtx = glfwContext{};
	try {
		auto instance{ terminal::create_instance() };

		auto physical_device{ terminal::select_physical_device(instance) };
		uint32_t graphicsQueueFamilyIndex = terminal::select_queue_family(physical_device);

		uint32_t width = 512, height = 512;
		auto [window, vk_surface] = terminal::create_window_and_get_surface(*instance, width, height);
		vk::SharedSurfaceKHR surface(vk_surface, instance);

		vk::SharedDevice device{ terminal::create_device(physical_device, graphicsQueueFamilyIndex) };
		vk::SharedQueue queue{ device->getQueue(graphicsQueueFamilyIndex, 0), device };

		vk::CommandPoolCreateInfo commandPoolCreateInfo({}, graphicsQueueFamilyIndex);
		vk::SharedCommandPool commandPool(device->createCommandPool(commandPoolCreateInfo), device);



		std::vector<vk::SurfaceFormatKHR> formats = physical_device.getSurfaceFormatsKHR(*surface);
		vk::Format colorFormat = (formats[0].format == vk::Format::eUndefined) ? vk::Format::eR8G8B8A8Unorm : formats[0].format;
		vk::Format depthFormat = vk::Format::eD16Unorm;

		vk::SurfaceCapabilitiesKHR surfaceCapabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);
		vk::Extent2D swapchainExtent;
		if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
			swapchainExtent.width = std::clamp(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
			swapchainExtent.height = std::clamp(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
		}
		else {
			swapchainExtent = surfaceCapabilities.currentExtent;
		}

		vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

		vk::SurfaceTransformFlagBitsKHR preTransform = (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
			? vk::SurfaceTransformFlagBitsKHR::eIdentity
			: surfaceCapabilities.currentTransform;

		vk::CompositeAlphaFlagBitsKHR compositeAlpha =
			(surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
			: (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
			: (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit :
			vk::CompositeAlphaFlagBitsKHR::eOpaque;

		vk::SwapchainCreateInfoKHR swapchainCreateInfo(vk::SwapchainCreateFlagsKHR(),
			*surface,
			std::clamp(3u, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount),
			colorFormat,
			vk::ColorSpaceKHR::eSrgbNonlinear,
			swapchainExtent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
			vk::SharingMode::eExclusive,
			{},
			preTransform,
			compositeAlpha,
			swapchainPresentMode,
			true,
			nullptr);

		vk::SharedSwapchainKHR swapchain{ device->createSwapchainKHR(swapchainCreateInfo), device, surface };
		std::vector<vk::Image> swapchainImages = device->getSwapchainImagesKHR(*swapchain);

		std::vector<vk::SharedImageView> imageViews;
		std::vector<vk::UniqueSemaphore> acquire_image_semaphores;
		std::vector<vk::UniqueSemaphore> render_complete_semaphores;
		imageViews.reserve(swapchainImages.size());
		vk::ImageViewCreateInfo imageViewCreateInfo({}, {}, vk::ImageViewType::e2D, colorFormat, {}, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
		for (auto image : swapchainImages) {
			imageViewCreateInfo.image = image;
			imageViews.emplace_back(device->createImageView(imageViewCreateInfo), device);
			acquire_image_semaphores.push_back(device->createSemaphoreUnique(vk::SemaphoreCreateInfo{}));
			render_complete_semaphores.push_back(device->createSemaphoreUnique(vk::SemaphoreCreateInfo{}));
		}
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*commandPool, vk::CommandBufferLevel::ePrimary, swapchainImages.size());
		auto buffers = device->allocateCommandBuffers(commandBufferAllocateInfo);
		std::vector<vk::SharedCommandBuffer> command_buffers(swapchainImages.size());
		for (int i = 0; i < swapchainImages.size(); i++){
			command_buffers[i] = vk::SharedCommandBuffer{ buffers[i], device, commandPool };
			vk::CommandBufferBeginInfo begin_info{};
			command_buffers[i]->begin(begin_info);
			command_buffers[i]->end();
		}
		int index = 0;
		while (false == glfwWindowShouldClose(window)) {
			index++;
			if (index >= acquire_image_semaphores.size()) index = 0;
			auto& acquire_image_semaphore = acquire_image_semaphores[index];
			auto& render_complete_semaphore = render_complete_semaphores[index];
			auto& command_buffer = command_buffers[index];
			auto image_index = device->acquireNextImageKHR(*swapchain, 10000000000, *acquire_image_semaphore).value;

			{
				std::array<vk::Semaphore, 1> wait_semaphores{ *acquire_image_semaphore };
				std::array<vk::PipelineStageFlags, 1> stages{ vk::PipelineStageFlagBits::eBottomOfPipe };
				std::array<vk::CommandBuffer, 1> submit_command_buffers{ *command_buffer};
				std::array<vk::Semaphore, 1> signal_semaphores{ *render_complete_semaphore };
				queue->submit(vk::SubmitInfo{ wait_semaphores, stages, submit_command_buffers, signal_semaphores });
			}
			
			{
				std::array<vk::Semaphore, 1> wait_semaphores{ *render_complete_semaphore };
				std::array<vk::SwapchainKHR, 1> swapchains{ *swapchain };
				std::array<uint32_t, 1> indices{ image_index };
				vk::PresentInfoKHR present_info{wait_semaphores, swapchains, indices};
				queue->presentKHR(present_info);
			}
			glfwPollEvents();
		}

	}
	catch (vk::SystemError& err) {
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		return -1;
	}
	catch (std::exception& err) {
		std::cout << "std::exception: " << err.what() << std::endl;
	}
	catch (...) {
		std::cout << "unknown error\n";
		return -1;
	}
	return 0;
}