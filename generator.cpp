#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

struct variable_depends {
	std::string name;
	std::vector<std::string> depends;
};

class generator {
public:
	generator(std::vector<variable_depends> depends, std::map<std::string, std::string> var_constructor_map, std::map<std::string, std::string> var_type_map,
		std::map<std::string, std::string> var_destructor_map) : 
		m_depends{ std::move(depends) }, m_var_constructor_map{ std::move(var_constructor_map) }, m_var_type_map{ std::move(var_type_map) },
		m_var_destructor_map{ var_destructor_map }
	{
		for (auto& depend : m_depends) {
			m_var_depends_map.emplace(depend.name, depend.depends);
			m_var_is_constructed.emplace(depend.name, false);
			m_var_is_destructed.emplace(depend.name, false);

			for (auto& be_depend : depend.depends) {
				m_var_be_depends_map[be_depend].emplace_back(depend.name);
			}
		}
	}
	void print_variable_constructor(std::ostream& out, std::string name) {
		if (false == m_var_is_constructed[name]) {
			for (auto& dep : m_var_depends_map[name]) {
				print_variable_constructor(out, dep);
			}
			//std::cout << name << "_constructor();" << std::endl;
			std::cout << m_var_type_map[name] << " " << name << ";" << std::endl;
			out << "{" << std::endl;
			out << m_var_constructor_map[name] << std::endl;
			out << "}" << std::endl;
			m_var_is_constructed[name] = true;
			if (m_var_be_depends_map[name].empty()) {
				print_variable_destructors(out, name);
			}
			m_constructed_var.emplace_back(name);
		}
	}
	void print_variable_destructors(std::ostream& out, std::string name) {
		if (false == m_var_is_destructed[name]) {
			if (std::all_of(std::begin(m_var_be_depends_map[name]), std::end(m_var_be_depends_map[name]), [&m_var_is_destructed = m_var_is_destructed](std::string name) {
				return m_var_is_destructed[name];
				})) {
				std::cout << m_var_destructor_map[name] << std::endl;
				m_var_is_destructed[name] = true;
				for (auto& dep : m_var_depends_map[name]) {
					print_variable_destructors(out, dep);
				}
			}
		}
	}
	void print_sequence(std::ostream& out) {
		for (auto& var_dep : m_depends) {
			print_variable_constructor(out, var_dep.name);
		}
	}
private:
	std::vector<variable_depends> m_depends;
	std::map<std::string, std::vector<std::string>> m_var_depends_map;
	std::map<std::string, bool> m_var_is_constructed;
	std::map<std::string, std::vector<std::string>> m_var_be_depends_map;
	std::map<std::string, bool> m_var_is_destructed;
	std::vector<std::string> m_constructed_var;

	std::map<std::string, std::string> m_var_constructor_map;
	std::map<std::string, std::string> m_var_type_map;
	std::map<std::string, std::string> m_var_destructor_map;
};

#define TO_STRING(code) #code

int main() {
	std::vector<variable_depends> depends{
		{"m_physical_device", {"m_instance"}},
		{"m_queue_family_index", {"m_physical_device"}},
		{"m_device", {"m_physical_device"}},
		{"m_command_pool", {"m_device"}},
		{"m_descriptor_set_layout", {"m_device"}},
		{"m_shader_module", {"m_device", "m_spirv_file_size", "m_spirv_file_data"}},
	};
	std::map<std::string, std::string> var_constructor_map{
		{"m_instance", 
		TO_STRING(VkApplicationInfo application_info{};
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
			})},
		{"m_physical_device", TO_STRING(uint32_t count = 1;
			VkPhysicalDevice physical_device;
			auto res = vkEnumeratePhysicalDevices(m_instance, &count, &physical_device);
			return physical_device;)},
		{"m_queue_family_index", "constexpr uint32_t COUNT = 8;\n"
"                       std::array<VkQueueFamilyProperties, COUNT> properties_array{};\n"
"                       uint32_t count = COUNT;\n"
"                       vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &count, properties_array.data());\n"
"                       assert(count > 0);\n"
"                       for (uint32_t i = 0; i < count; i++) {\n"
"                               auto& properties = properties_array[i];\n"
"                               if (properties.queueFlags & VK_QUEUE_COMPUTE_BIT) {\n"
"                                       m_queue_family_index = i;\n"
"                                       break;\n"
"                               }\n"
"                       }\n"
"                       throw std::runtime_error{ \"failed to find queue family\" };\n"},
		{"m_device", TO_STRING(VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = m_queue_family_index;
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
			m_device = device;)},
		{"m_command_pool", "            VkCommandPoolCreateInfo create_info{};\n"
"            create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;\n"
"            create_info.queueFamilyIndex = m_queue_family_index;\n"
"            VkCommandPool command_pool;\n"
"            auto res = vkCreateCommandPool(m_device, &create_info, NULL, &command_pool);\n"
"            if (res != VK_SUCCESS) {\n"
"                throw std::runtime_error{ \"failed to create command pool\" };\n"
"            }\n"
"            m_command_pool = command_pool;\n"},
		{"m_descriptor_set_layout", "            VkDescriptorSetLayoutBinding binding{};\n"
"            binding.binding = 0;\n"
"            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;\n"
"            binding.descriptorCount = 1;\n"
"            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;\n"
"\n"
"            VkDescriptorSetLayoutCreateInfo create_info{};\n"
"            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;\n"
"            create_info.bindingCount = 1;\n"
"            create_info.pBindings = &binding;\n"
"\n"
"            VkDescriptorSetLayout descriptor_set_layout;\n"
"            auto res = vkCreateDescriptorSetLayout(m_device, &create_info, NULL, &descriptor_set_layout);\n"
"            if (res != VK_SUCCESS) {\n"
"                throw std::runtime_error{ \"failed to create descriptor set layout\" };\n"
"            }\n"
"            m_descriptor_set_layout = descriptor_set_layout;\n"},
		{"m_shader_module","VkShaderModuleCreateInfo create_info{};\n"
"create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;\n"
"create_info.codeSize = m_spirv_file_size;\n"
"create_info.pCode = m_spirv_file_data;\n"
"VkShaderModule shader_module;\n"
"auto res = vkCreateShaderModule(m_device, &create_info, NULL, &shader_module);\n"
"if (res != VK_SUCCESS) {\n"
"    throw std::runtime_error(\"failed to create shader module\");\n"
"}\n"
"m_shader_module = shader_module;\n"},

	};
	std::map<std::string, std::string> var_destructor_map{
		{"m_instance",
		"vkDestroyInstance(m_instance, NULL);"},
		{"m_device","vkDestroyDevice(m_device, NULL);"},
		{"m_shader_module", "vkDestroyShaderModule(m_device, m_shader_module, NULL);"}
	};
	std::map<std::string, std::string> var_type_map{
		{"m_instance", "VkInstance"},
		{"m_physical_device", "VkPhysicalDevice"},
		{"m_device", "VkDevice"},
		{"m_shader_module", "VkShaderModule"},
		{"m_command_pool", "VkCommandPool"},
		{"m_descriptor_set_layout", "VkDescriptorSetLayout"},
	};
	generator generator{ std::move(depends), std::move(var_constructor_map), std::move(var_type_map), std::move(var_destructor_map)};
	generator.print_sequence(std::cout);
	return 0;
}