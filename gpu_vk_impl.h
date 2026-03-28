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

const char* GPU_VK_DEVICE_EXTENSIONS[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

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
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkSurfaceKHR surface;

	VkSwapchainKHR swapchain;
	VkImage* swapchain_images;
	VkImageView* swapchain_image_views;
	u32 swapchain_image_count;
	Gpu_Vk_Swapchain_Desc swapchain_desc;

	VkQueue queue[Gpu_Vk_Queue_Count];
} Gpu_Vk;

static Gpu_Vk gpu_vk;

void gpu_vk_init_instance(const Gpu_Vk_Desc* desc) {
	gpu_vk_log("Creating instance.");
	
	Allocator* allocator = desc->allocator;
	
	const char* internal_required_extensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
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

	u32 available_extension_count = 0;
	vkEnumerateDeviceExtensionProperties(device, NULL, &available_extension_count, NULL);

	VkExtensionProperties* available_extensions;
	allocator_new(gpu_vk.allocator, available_extensions, available_extension_count);

	for (u32 required_extension_idx = 0; required_extension_idx < sl_array_count(GPU_VK_DEVICE_EXTENSIONS); required_extension_idx++) {
		const char* required_extension = GPU_VK_DEVICE_EXTENSIONS[required_extension_idx];

		bool found_extension = false;
		for (u32 available_extension_idx = 0; available_extension_idx < available_extension_count; available_extension_idx++) {
			if (strcmp(required_extension, available_extensions[available_extension_idx].extensionName) == 0) {
				found_extension = true;
				break;
			}
		}

		if (!found_extension) {
			allocator_free(gpu_vk.allocator, available_extensions, available_extension_count);
			return false;
		}
	}

	allocator_free(gpu_vk.allocator, available_extensions, available_extension_count);
	
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
	gpu_vk.physical_device = physical_device;

#if GPU_VK_LOGGING
{
	VkPhysicalDeviceProperties physical_device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
	gpu_vk_log("Creating logical device for \"%s\".", physical_device_properties.deviceName);
}
#endif
	
	Gpu_Vk_Queue_Family_Indices queue_indices = gpu_vk_get_physical_device_queue_families(physical_device, allocator);
	
	const f32 queue_priority = 1.0f;

	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

	u32 queue_create_info_count = 0;
	VkDeviceQueueCreateInfo* queue_create_infos;
	allocator_new(gpu_vk.allocator, queue_create_infos, queue_family_count);

	// Deduplicate required queues
	for (u32 queue_family_idx = 0; queue_family_idx < queue_family_count; queue_family_idx++) {
		bool create_queue = false;
		for (Gpu_Vk_Queue queue = 0; queue < Gpu_Vk_Queue_Count; queue++) {
			if (queue_indices.index[queue] == queue_family_idx) {
				create_queue = true;
				break;
			}
		}
		if (create_queue) {
			queue_create_infos[queue_create_info_count++] = (VkDeviceQueueCreateInfo) {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queue_family_idx,
				.queueCount = 1,
				.pQueuePriorities = &queue_priority,
			};
		}
	}
	
	VkPhysicalDeviceFeatures device_features = {0};
	
	VkDeviceCreateInfo device_create_info = {0};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pQueueCreateInfos = queue_create_infos;
	device_create_info.queueCreateInfoCount = queue_create_info_count;
	device_create_info.pEnabledFeatures = &device_features;
	device_create_info.ppEnabledExtensionNames = GPU_VK_DEVICE_EXTENSIONS;
	device_create_info.enabledExtensionCount = sl_array_count(GPU_VK_DEVICE_EXTENSIONS);
	
	VkResult create_device_result = vkCreateDevice(physical_device, &device_create_info, NULL, &gpu_vk.device);
	sl_assert(create_device_result == VK_SUCCESS, "Failed to create logical device.");

	allocator_free(gpu_vk.allocator, queue_create_infos, queue_family_count);

	for (Gpu_Vk_Queue queue = 0; queue < Gpu_Vk_Queue_Count; queue++) {
		vkGetDeviceQueue(gpu_vk.device, queue_indices.index[queue], 0, &gpu_vk.queue[queue]);
	}
}

void gpu_vk_init_swapchain() {
	VkSurfaceCapabilitiesKHR capabilities;
	VkResult get_capabilities_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu_vk.physical_device, gpu_vk.surface, &capabilities);
	sl_assert(get_capabilities_result == VK_SUCCESS, "Failed to get swapchain capabilities.");

	VkSurfaceFormatKHR surface_format = {
		.format = VK_FORMAT_B8G8R8A8_SRGB,
		.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
	};

	u32 available_present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu_vk.physical_device, gpu_vk.surface, &available_present_mode_count, NULL);
	
	VkPresentModeKHR* available_present_modes;
	allocator_new(gpu_vk.allocator, available_present_modes, available_present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu_vk.physical_device, gpu_vk.surface, &available_present_mode_count, available_present_modes);

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // default
	for (u32 available_present_mode_idx = 0; available_present_mode_idx < available_present_mode_count; available_present_mode_idx++) {
		if (available_present_modes[available_present_mode_idx] == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	allocator_free(gpu_vk.allocator, available_present_modes, available_present_mode_count);

	VkExtent2D extent = gpu_vk.swapchain_desc.get_swapchain_extent_fn(gpu_vk.swapchain_desc.ctx);

	const u32 min_image_count = sl_clamp(3, capabilities.minImageCount, capabilities.maxImageCount);

	gpu_vk_log("Creating swapchain with extent (%u, %u), %u min images.", extent.width, extent.height, min_image_count);

	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = gpu_vk.surface,
		.imageFormat = surface_format.format,
		.imageColorSpace = surface_format.colorSpace,
		.imageExtent = extent,
		.minImageCount = min_image_count,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageArrayLayers = 1,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	Gpu_Vk_Queue_Family_Indices queue_indices = gpu_vk_get_physical_device_queue_families(gpu_vk.physical_device, gpu_vk.allocator);
	if (queue_indices.index[Gpu_Vk_Queue_Graphics] == queue_indices.index[Gpu_Vk_Queue_Present]) {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    	create_info.queueFamilyIndexCount = 3;
    	create_info.pQueueFamilyIndices = queue_indices.index;
	}

	VkResult create_result = vkCreateSwapchainKHR(gpu_vk.device, &create_info, NULL, &gpu_vk.swapchain);
	sl_assert(create_result == VK_SUCCESS, "Failed to create swapchain.");

	u32 swapchain_image_count;
	vkGetSwapchainImagesKHR(gpu_vk.device, gpu_vk.swapchain, &swapchain_image_count, NULL);

	// Resize image arrays
	if (gpu_vk.swapchain_image_count != swapchain_image_count) {
		if (gpu_vk.swapchain_image_count > 0) {
			allocator_free(gpu_vk.allocator, gpu_vk.swapchain_images, gpu_vk.swapchain_image_count);
			allocator_free(gpu_vk.allocator, gpu_vk.swapchain_image_views, gpu_vk.swapchain_image_count);
		}
		gpu_vk.swapchain_image_count = swapchain_image_count;
		allocator_new(gpu_vk.allocator, gpu_vk.swapchain_images, swapchain_image_count);
		allocator_new(gpu_vk.allocator, gpu_vk.swapchain_image_views, swapchain_image_count);
	}

	// Get images
	vkGetSwapchainImagesKHR(gpu_vk.device, gpu_vk.swapchain, &swapchain_image_count, gpu_vk.swapchain_images);

	// Create image views
	for (u32 image_idx = 0; image_idx < swapchain_image_count; image_idx++) {
		const VkImageViewCreateInfo view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = gpu_vk.swapchain_images[image_idx],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = surface_format.format,
			.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1,
		};
		const VkResult view_create_result = vkCreateImageView(gpu_vk.device, &view_create_info, NULL, &gpu_vk.swapchain_image_views[image_idx]);
		sl_assert(view_create_result == VK_SUCCESS, "Failed to create image view for swapchain.");
	}
}

void gpu_vk_init(const Gpu_Vk_Desc* desc) {
	gpu_vk = (Gpu_Vk) {};
	gpu_vk.allocator = desc->allocator;
	gpu_vk.swapchain_desc = desc->swapchain_desc;
	gpu_vk_init_instance(desc);
	gpu_vk_init_surface();
	gpu_vk_init_device();
	gpu_vk_init_swapchain();
}
void gpu_vk_deinit() {
	vkDestroyInstance(gpu_vk.instance, NULL);
	gpu_vk = (Gpu_Vk) {};
}
