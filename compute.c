#include <assert.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

struct App {
  VkInstance instance;
  VkPhysicalDevice physical_device;
  uint32_t queue_family;
  VkDevice device;
  VkQueue queue;
  VkFence fence;
  VkPhysicalDeviceMemoryProperties memory_properties;

  VkPipeline pipeline;
  VkPipelineLayout pipeline_layout;
  VkDescriptorSetLayout descriptor_set_layout;

  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;

  VkBuffer storage_buffer;
  VkDeviceMemory storage_memory;
  void* storage_memory_ptr;

  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_set;
};

typedef struct App App;

void create_instance(App* app) {
  VkApplicationInfo application_info =
  {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .apiVersion = VK_API_VERSION_1_3,
  };

  VkInstanceCreateInfo create_info =
  {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &application_info,
  };

  VkResult res = vkCreateInstance(&create_info, NULL, &app->instance);
  assert(res == VK_SUCCESS);
}

void destroy_instance(App* app) {
  vkDestroyInstance(app->instance, NULL);
}

void select_physical_device(App* app) {
  VkPhysicalDevice physical_devices[8] = {};
  uint32_t count = 8;
  VkResult res = vkEnumeratePhysicalDevices(app->instance, &count,
					    physical_devices);
  assert(res == VK_SUCCESS);
  assert(count > 0);
  app->physical_device = physical_devices[0];
  
  vkGetPhysicalDeviceMemoryProperties(app->physical_device,
				      &app->memory_properties);
}

void select_queue_family(App* app) {
  VkQueueFamilyProperties properties[8];
  uint32_t count = 8;
  vkGetPhysicalDeviceQueueFamilyProperties(app->physical_device, &count,
					   properties);
  assert(count > 0);
  for (uint32_t i = 0; i < count; i++) {
    if (VK_QUEUE_COMPUTE_BIT & properties[i].queueFlags) {
      app->queue_family = i;
      return;
    }
  }
  assert(0);
}

void create_device(App* app) {
  float priority = 1.0;
  VkDeviceQueueCreateInfo queue_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = app->queue_family,
    .queueCount = 1,
    .pQueuePriorities = &priority,
  };

  VkPhysicalDeviceVulkan13Features vulkan_1_3_features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .synchronization2 = VK_TRUE,
    .maintenance4 = VK_TRUE,
  };

  VkPhysicalDeviceFeatures2 features2 = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &vulkan_1_3_features,
  };

  VkDeviceCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &features2,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &queue_create_info,
  };

  VkResult res = vkCreateDevice(app->physical_device, &create_info, NULL,
				&app->device);
  assert(res == VK_SUCCESS);

  vkGetDeviceQueue(app->device, app->queue_family, 0, &app->queue);

  VkFenceCreateInfo fence_create_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
  };
  res = vkCreateFence(app->device, &fence_create_info, NULL, &app->fence);
  assert(res == VK_SUCCESS);
}

void destroy_device(App* app) {
  vkDestroyFence(app->device, app->fence, NULL);
  vkDestroyDevice(app->device, NULL);
}

VkShaderModule create_shader_module(App* app, const char* file_path) {
  FILE* spirv_file = fopen(file_path, "rb");
  assert(spirv_file != NULL);
  uint32_t buffer_size=256, size=0;
  uint32_t* spirv_codes = (uint32_t*)malloc(4*buffer_size);
  assert(spirv_codes != NULL);
  while (!feof(spirv_file)) {
    if (1 != fread(spirv_codes+size, 4, 1, spirv_file)) {
      break;
    }
    size++;
    assert(size <= buffer_size);
    if (size == buffer_size) {
      buffer_size *= 2;
      spirv_codes = (uint32_t*)realloc(spirv_codes, 4*buffer_size);
      assert(spirv_codes != NULL);
    }
  }
  VkShaderModuleCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size*4,
    .pCode = spirv_codes
  };
  VkShaderModule shader_module = {};
  VkResult res = vkCreateShaderModule(app->device, &create_info, NULL,
				      &shader_module);
  assert(res == VK_SUCCESS);
  return shader_module;
}

void destroy_shader_module(App* app, VkShaderModule shader_module) {
  vkDestroyShaderModule(app->device, shader_module, NULL);
}

void create_descriptor_set_layout(App* app) {
  VkDescriptorSetLayoutBinding binding = {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
  };

  VkDescriptorSetLayoutCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = &binding,
  };

  VkResult res = vkCreateDescriptorSetLayout(app->device, &create_info, NULL,
					     &app->descriptor_set_layout);
  assert(res == VK_SUCCESS);
}

void destroy_descriptor_set_layout(App* app) {
  vkDestroyDescriptorSetLayout(app->device, app->descriptor_set_layout, NULL);
}

void create_pipeline_layout(App* app) {
  VkPipelineLayoutCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .flags = 0,
    .setLayoutCount = 1,
    .pSetLayouts = &app->descriptor_set_layout,
  };

  VkResult res = vkCreatePipelineLayout(app->device, &create_info, NULL,
					&app->pipeline_layout);
  assert(res == VK_SUCCESS);
}

void destroy_pipeline_layout(App* app) {
  vkDestroyPipelineLayout(app->device, app->pipeline_layout, NULL);
}

void create_pipeline(App* app) {
  VkComputePipelineCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .flags = 0,
    .stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage.stage = VK_SHADER_STAGE_COMPUTE_BIT,
    .stage.module = create_shader_module(app, "comp.spv"),
    .stage.pName = "main",
    .layout = app->pipeline_layout
  };
  VkResult res = vkCreateComputePipelines(app->device, VK_NULL_HANDLE, 1,
					  &create_info, NULL,
					  &app->pipeline);
  assert(res == VK_SUCCESS);
  destroy_shader_module(app, create_info.stage.module);
}

void destroy_pipeline(App* app) {
  vkDestroyPipeline(app->device, app->pipeline, NULL);
}

void create_command_pool(App* app) {
  VkCommandPoolCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .queueFamilyIndex = app->queue_family,
  };

  VkResult res = vkCreateCommandPool(app->device, &create_info, NULL,
				     &app->command_pool);
  assert(res == VK_SUCCESS);
}

void destroy_command_pool(App* app) {
  vkDestroyCommandPool(app->device, app->command_pool, NULL);
}

void allocate_command_buffer(App* app) {
  VkCommandBufferAllocateInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = app->command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };
  VkResult res = vkAllocateCommandBuffers(app->device, &info,
					  &app->command_buffer);
  assert(res == VK_SUCCESS);
}

void record_command_buffer(App* app) {
  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };

  VkResult res = vkBeginCommandBuffer(app->command_buffer, &begin_info);
  assert(res == VK_SUCCESS);
  vkCmdBindPipeline(app->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
		    app->pipeline);
  vkCmdBindDescriptorSets(app->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
			  app->pipeline_layout, 0, 1, &app->descriptor_set,
			  0, NULL);
  vkCmdDispatch(app->command_buffer, 1, 1, 1);

  res = vkEndCommandBuffer(app->command_buffer);
  assert(res == VK_SUCCESS);
}

VkBuffer create_buffer(App* app, VkDeviceSize size, VkBufferUsageFlags usage) {
  VkBufferCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = &app->queue_family,
  };

  VkBuffer buffer = {};
  VkResult res = vkCreateBuffer(app->device, &create_info, NULL, &buffer);
  assert(res == VK_SUCCESS);
  return buffer;
}
void destroy_buffer(App* app, VkBuffer buffer) {
  vkDestroyBuffer(app->device, buffer, NULL);
}

uint32_t findProperties(App* app, uint32_t memoryTypeBitsRequirements, VkMemoryPropertyFlags requiredProperty) {
  const uint32_t memoryCount = app->memory_properties.memoryTypeCount;
  for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; memoryIndex++) {
    const uint32_t memoryTypeBits = (1 << memoryIndex);
    const bool isRequiredMemoryType = memoryTypeBitsRequirements &
      memoryTypeBits;
    const VkMemoryPropertyFlags properties =
      app->memory_properties.memoryTypes[memoryIndex].propertyFlags;
    const bool hasRequiredProperties =
      (properties & requiredProperty) == requiredProperty;
    if (isRequiredMemoryType && hasRequiredProperties)
      return memoryIndex;
  }
  assert(0);
  return 0;
}

VkDeviceMemory alloc_device_memory(App* app, VkBuffer buffer,
				   VkMemoryPropertyFlags property) {
  VkMemoryRequirements requirements = {};
  vkGetBufferMemoryRequirements(app->device, buffer, &requirements);
  uint32_t memoryType = findProperties(app, requirements.memoryTypeBits, property);

  VkMemoryAllocateInfo allocate_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = requirements.size,
    .memoryTypeIndex = memoryType,
  };

  VkDeviceMemory device_memory = {};
  VkResult res = vkAllocateMemory(app->device, &allocate_info, NULL,
				  &device_memory);
  assert(res == VK_SUCCESS);

  vkBindBufferMemory(app->device, buffer, device_memory, 0);
  return device_memory;
}

void free_device_memory(App* app, VkDeviceMemory device_memory) {
  vkFreeMemory(app->device, device_memory, NULL);
}

void* map_device_memory(App* app, VkDeviceMemory device_memory,
			VkDeviceSize offset, VkDeviceSize size) {
  void* ptr = {};
  VkResult res = vkMapMemory(app->device,
			     device_memory, offset, size, 0, &ptr);
  assert(res == VK_SUCCESS);
  return ptr;
}

void unmap_device_memory(App* app, VkDeviceMemory device_memory) {
  vkUnmapMemory(app->device, device_memory);
}

void create_storage_buffer(App* app) {
  app->storage_buffer = create_buffer(app,
				      128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  app->storage_memory = alloc_device_memory(app, app->storage_buffer,
					    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
					    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  app->storage_memory_ptr = map_device_memory(app, app->storage_memory,
					      0, 128);
}

void destroy_storage_buffer(App* app) {
  destroy_buffer(app, app->storage_buffer);
  unmap_device_memory(app, app->storage_memory);
  free_device_memory(app, app->storage_memory);
}

void create_descriptor_pool(App* app) {
  VkDescriptorPoolSize pool_size = {
    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
  };

  VkDescriptorPoolCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = 1,
    .poolSizeCount = 1,
    .pPoolSizes = &pool_size,
  };

  vkCreateDescriptorPool(app->device, &info, NULL,
			 &app->descriptor_pool);
}

void destroy_descriptor_pool(App* app) {
  vkDestroyDescriptorPool(app->device, app->descriptor_pool, NULL);
}

void allocate_descriptor_set(App* app) {
  VkDescriptorSetAllocateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = app->descriptor_pool,
    .descriptorSetCount = 1,
    .pSetLayouts = &app->descriptor_set_layout,
  };

  VkResult res = vkAllocateDescriptorSets(app->device, &info,
					  &app->descriptor_set);
  assert(res == VK_SUCCESS);
}

void update_descriptor_set(App* app) {
  VkDescriptorBufferInfo buffer_info = {
    .buffer = app->storage_buffer,
    .offset = 0,
    .range = 128,
  };

  VkWriteDescriptorSet write = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = app->descriptor_set,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .pBufferInfo = &buffer_info,
  };

  vkUpdateDescriptorSets(app->device, 1, &write, 0, NULL);
}

void init(App* app) {
  create_instance(app);
  select_physical_device(app);
  select_queue_family(app);
  create_device(app);
  create_descriptor_set_layout(app);
  create_pipeline_layout(app);
  create_pipeline(app);
  create_descriptor_pool(app);
  allocate_descriptor_set(app);
  create_storage_buffer(app);
  update_descriptor_set(app);

  create_command_pool(app);
  allocate_command_buffer(app);
  record_command_buffer(app);
}

void deinit(App* app) {
  destroy_command_pool(app);

  destroy_storage_buffer(app);
  destroy_descriptor_pool(app);

  destroy_pipeline(app);
  destroy_pipeline_layout(app);
  destroy_descriptor_set_layout(app);
  destroy_device(app);
  destroy_instance(app);
}

void draw(App* app) {
  assert(VK_SUCCESS == vkResetFences(app->device, 1, &app->fence));
  VkCommandBufferSubmitInfo command_buffer_submit_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .commandBuffer = app->command_buffer,
  };

  VkSubmitInfo2 submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    .commandBufferInfoCount = 1,
    .pCommandBufferInfos = &command_buffer_submit_info,
  };
  VkResult res = vkQueueSubmit2(app->queue, 1, &submit_info, app->fence);
  assert(res == VK_SUCCESS);

  res = vkWaitForFences(app->device, 1, &app->fence, VK_TRUE,
			UINT64_MAX);
  assert(res == VK_SUCCESS);

  VkMappedMemoryRange memory_range = {
    .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
    .memory = app->storage_memory,
    .offset = 0,
    .size = 128,
  };
  res = vkInvalidateMappedMemoryRanges(app->device, 1,
						&memory_range);
  assert(res == VK_SUCCESS);

  uint32_t* data = (uint32_t*)app->storage_memory_ptr;

  printf("%" PRIu32 "\n", data[0]);
}

void run(App* app) {
  for (int i = 0; i < 8; i++) {
    draw(app);
  }
}

int main(void) {
  App app;
  init(&app);
  run(&app);
  deinit(&app);
  return 0;
}
