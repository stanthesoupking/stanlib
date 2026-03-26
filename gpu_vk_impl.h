#pragma once

#define GPU_VK_LOGGING 1
#define GPU_VK_VALIDATION 1

#include "gpu_vk.h"

#if GPU_VK_LOGGING
	#define gpu_vk_log(fmt, ...) \
		printf("[Gpu_Vk]: " fmt "\n", ##__VA_ARGS__)
#else
	#define gpu_vk_log(fmt, ...) ((void)0)
#endif

typedef enum Gpu_Vk_Queue {
	Gpu_Vk_Queue_Graphics,
	Gpu_Vk_Queue_Compute,
	Gpu_Vk_Queue_Present,
	Gpu_Vk_Queue_Count
} Gpu_Vk_Queue;

typedef struct Gpu_Vk_Queue_Family_Indices {
	u32 index[Gpu_Vk_Queue_Count];
} Gpu_Vk_Queue_Family_Indices;

typedef struct Gpu_Vk {
	Allocator* allocator;
	VkInstance instance;
	VkDevice device;
	VkSurfaceKHR surface;

	Gpu_Vk_Swapchain_Desc swapchain_desc;

	VkQueue queue[Gpu_Vk_Queue_Count];
} Gpu_Vk;

static Gpu_Vk gpu_vk;

void gpu_vk_init_instance(const Gpu_Vk_Desc* desc) {
	gpu_vk_log("Creating instance.");
	
	Allocator* allocator = desc->allocator;
	
	const char* internal_required_extensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	};
	
	const u32 combined_extension_count = sl_array_count(internal_required_extensions) + desc->required_extension_count;
	const char** combined_extensions;
	allocator_new(allocator, combined_extensions, combined_extension_count);
	
	u32 next_combined_extension_idx = 0;
	for (u32 i = 0; i < sl_array_count(internal_required_extensions); i++) {
		combined_extensions[next_combined_extension_idx++] = internal_required_extensions[i];
	}
	for (u32 i = 0; i < desc->required_extension_count; i++) {
		combined_extensions[next_combined_extension_idx++] = desc->required_extensions[i];
	}
	
	VkApplicationInfo app_info = {0};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

	create_info.enabledExtensionCount = combined_extension_count;
	create_info.ppEnabledExtensionNames = combined_extensions;
	
#if GPU_VK_VALIDATION
	{
		const char* req_validation_layers[] = {
			"VK_LAYER_KHRONOS_validation"
		};
		
		u32 supported_layer_count;
		vkEnumerateInstanceLayerProperties(&supported_layer_count, NULL);
		
		VkLayerProperties* supported_layers;
		allocator_new(desc->allocator, supported_layers, supported_layer_count);
		vkEnumerateInstanceLayerProperties(&supported_layer_count, supported_layers);
		
		u32 found_validation_layer_count = 0;
		const char* found_validation_layers[sl_array_count(req_validation_layers)];
		
		for (u32 req_validation_layer_idx = 0; req_validation_layer_idx < sl_array_count(req_validation_layers); req_validation_layer_idx++) {
			bool found_layer = false;
			for (u32 supported_layer_idx = 0; supported_layer_idx < supported_layer_count; supported_layer_idx++) {
				if (strcmp(supported_layers[supported_layer_idx].layerName, req_validation_layers[req_validation_layer_idx]) == 0) {
					found_layer = true;
					break;
				}
			}
			
			if (found_layer) {
				gpu_vk_log("Enabling layer: %s", req_validation_layers[req_validation_layer_idx]);
				found_validation_layers[found_validation_layer_count++] = req_validation_layers[req_validation_layer_idx];
			} else {
				gpu_vk_log("Could not find layer: %s", req_validation_layers[req_validation_layer_idx]);
			}
		}
		
		create_info.ppEnabledLayerNames = found_validation_layers;
		create_info.enabledLayerCount = found_validation_layer_count;
	}
#endif

	VkResult res = vkCreateInstance(&create_info, NULL, &gpu_vk.instance);
	sl_assert(res == VK_SUCCESS, "Failed to create Vulkan instance.");
	
	allocator_free(allocator, combined_extensions, combined_extension_count);
}

Gpu_Vk_Queue_Family_Indices gpu_vk_get_physical_device_queue_families(VkPhysicalDevice device, Allocator* scratch_allocator) {
	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
	
	VkQueueFamilyProperties* queue_families;
	allocator_new(scratch_allocator, queue_families, queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
	
	Gpu_Vk_Queue_Family_Indices indices = {
		.index = {
			u32_max,
			u32_max,
			u32_max
		},
	};
	for (u32 queue_idx = 0; queue_idx < queue_family_count; queue_idx++) {
		VkQueueFamilyProperties queue_family = queue_families[queue_idx];

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_idx, gpu_vk.surface, &present_support);

		if (indices.index[Gpu_Vk_Queue_Graphics] == u32_max && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.index[Gpu_Vk_Queue_Graphics] = queue_idx;
		}
		if (indices.index[Gpu_Vk_Queue_Compute] == u32_max && queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
			indices.index[Gpu_Vk_Queue_Compute] = queue_idx;
		}
		if (indices.index[Gpu_Vk_Queue_Present] == u32_max && present_support) {
			indices.index[Gpu_Vk_Queue_Present] = queue_idx;
		}
	}
	
	allocator_free(scratch_allocator, queue_families, queue_family_count);
	
	return indices;
}

bool gpu_vk_queue_family_indices_is_complete(Gpu_Vk_Queue_Family_Indices indices) {
	for (Gpu_Vk_Queue queue = 0; queue < Gpu_Vk_Queue_Count; queue++) {
		if (indices.index[queue] == u32_max) {
			return false;
		}
	}
	return true;
}

bool gpu_vk_is_device_suitable(VkPhysicalDevice device, Allocator* scratch_allocator) {
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);
	
	if ((device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) && (device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) {
		return false;
	}
	
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(device, &device_features);
	
	Gpu_Vk_Queue_Family_Indices queue_indices = gpu_vk_get_physical_device_queue_families(device, scratch_allocator);
	if (!gpu_vk_queue_family_indices_is_complete(queue_indices)) {
		return false;
	}
	
	return true;
}

bool gpu_vk_is_device_discrete(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);
	return (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
}

VkPhysicalDevice gpu_vk_find_physical_device(void) {
	gpu_vk_log("Finding physical device.");
	
	Allocator* allocator = gpu_vk.allocator;

	u32 device_count = 0;
	vkEnumeratePhysicalDevices(gpu_vk.instance, &device_count, NULL);
	gpu_vk_log("%u available device(s).", device_count);
	sl_assert(device_count > 0, "Failed to find GPU with Vulkan support.");
	
	VkPhysicalDevice* devices;
	allocator_new(allocator, devices, device_count);
	vkEnumeratePhysicalDevices(gpu_vk.instance, &device_count, devices);
	
	u32 suitable_device_count = 0;
	VkPhysicalDevice* suitable_devices;
	allocator_new(allocator, suitable_devices, device_count);
	for (u32 device_idx = 0; device_idx < device_count; device_idx++) {
		if (gpu_vk_is_device_suitable(devices[device_idx], allocator)) {
			suitable_devices[suitable_device_count++] = devices[device_idx];
		}
	}
	gpu_vk_log("%u suitable device(s).", suitable_device_count);
	sl_assert(suitable_device_count > 0, "No suitable GPU found.");
	
	for (u32 suitable_device_idx = 0; suitable_device_idx < suitable_device_count; suitable_device_idx++) {
		VkPhysicalDevice device = suitable_devices[suitable_device_idx];
		if (gpu_vk_is_device_discrete(device)) {
			// Use the first discrete GPU we find.
			return device;
		}
	}
	
	// Fallback
	return suitable_devices[0];
}

void gpu_vk_init_surface(void) {
	gpu_vk_log("Creating surface.");
	gpu_vk.surface = gpu_vk.swapchain_desc.get_surface_fn(gpu_vk.swapchain_desc.ctx, gpu_vk.instance);
}

void gpu_vk_init_device(void) {
	Allocator* allocator = gpu_vk.allocator;
	
	VkPhysicalDevice physical_device = gpu_vk_find_physical_device();

#if GPU_VK_LOGGING
{
	VkPhysicalDeviceProperties physical_device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
	gpu_vk_log("Creating logical device for \"%s\".", physical_device_properties.deviceName);
}
#endif
	
	Gpu_Vk_Queue_Family_Indices queue_indices = gpu_vk_get_physical_device_queue_families(physical_device, allocator);
	
	const f32 queue_priority = 1.0f;
		
	VkDeviceQueueCreateInfo queue_create_infos[Gpu_Vk_Queue_Count];
	for (Gpu_Vk_Queue queue = 0; queue < Gpu_Vk_Queue_Count; queue++) {
		queue_create_infos[queue] = (VkDeviceQueueCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queue_indices.index[queue],
			.queueCount = 1,
			.pQueuePriorities = &queue_priority,
		};
	}
	
	VkPhysicalDeviceFeatures device_features = {0};
	
	VkDeviceCreateInfo device_create_info = {0};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pQueueCreateInfos = queue_create_infos;
	device_create_info.queueCreateInfoCount = Gpu_Vk_Queue_Count;
	device_create_info.pEnabledFeatures = &device_features;
	
	VkResult create_device_result = vkCreateDevice(physical_device, &device_create_info, NULL, &gpu_vk.device);
	sl_assert(create_device_result == VK_SUCCESS, "Failed to create logical device.");

	for (Gpu_Vk_Queue queue = 0; queue < Gpu_Vk_Queue_Count; queue++) {
		vkGetDeviceQueue(gpu_vk.device, queue_indices.index[queue], 0, &gpu_vk.queue[queue]);
	}
}

void gpu_vk_init(const Gpu_Vk_Desc* desc) {
	gpu_vk = (Gpu_Vk) {};
	gpu_vk.allocator = desc->allocator;
	gpu_vk.swapchain_desc = desc->swapchain_desc;
	gpu_vk_init_instance(desc);
	gpu_vk_init_surface();
	gpu_vk_init_device();
}
void gpu_vk_deinit() {
	vkDestroyInstance(gpu_vk.instance, NULL);
	gpu_vk = (Gpu_Vk) {};
}
