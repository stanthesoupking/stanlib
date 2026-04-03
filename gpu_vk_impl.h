#pragma once

#define GPU_VK_LOGGING 1
#define GPU_VK_VALIDATION 1
#define GPU_VK_MAX_INFLIGHT_FRAMES 2

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

typedef SL_Handle Gpu_Vk_Framebuffer;

typedef struct Gpu_Vk_Framebuffer_Key {
	Gpu_Vk_Texture attachments[GPU_VK_MAX_ATTACHMENTS];
	u8 attachment_count;
	vec2_u16 size;
} Gpu_Vk_Framebuffer_Key;

typedef struct Gpu_Vk_Framebuffer_Data {
	u32 generation;

	Gpu_Vk_Framebuffer_Key key;
	u32 rc;
	VkFramebuffer framebuffer;

	// todo
	Gpu_Vk_Framebuffer older;
	Gpu_Vk_Framebuffer newer;
} Gpu_Vk_Framebuffer_Data;

typedef struct Gpu_Vk_Framebuffer_LRU {
	Gpu_Vk_Framebuffer oldest;
	Gpu_Vk_Framebuffer newest;
} Gpu_Vk_Framebuffer_LRU;

u64 gpu_vk_framebuffer_key_hash(Gpu_Vk_Framebuffer_Key key) {
	SL_Hasher hasher;
	sl_hasher_init(&hasher);
	sl_hasher_push(&hasher, immutable_buffer_for(key.attachment_count));
	for (u8 attachment_idx = 0; attachment_idx < key.attachment_count; attachment_idx++) {
		sl_hasher_push(&hasher, immutable_buffer_for(key.attachments[attachment_idx]));
	}
	sl_hasher_push(&hasher, immutable_buffer_for(key.size.x));
	sl_hasher_push(&hasher, immutable_buffer_for(key.size.y));
	return sl_hasher_finalise(&hasher);
}
bool gpu_vk_framebuffer_key_equals(Gpu_Vk_Framebuffer_Key a, Gpu_Vk_Framebuffer_Key b) {
	if (a.attachment_count != b.attachment_count) {
		return false;
	}
	if ((a.size.x != b.size.x) || (a.size.y != b.size.y)) {
		return false;
	}
	for (u8 attachment_idx = 0; attachment_idx < a.attachment_count; attachment_idx++) {
		if ((a.attachments[attachment_idx].index != b.attachments[attachment_idx].index) || (a.attachments[attachment_idx].generation != b.attachments[attachment_idx].generation)) {
			return false;
		}
	}
	return true;
}

sl_pool(Gpu_Vk_Framebuffer_Data, Gpu_Vk_Framebuffer_Pool, gpu_vk_framebuffer_pool);
sl_hashmap(Gpu_Vk_Framebuffer_Key, Gpu_Vk_Framebuffer, Gpu_Vk_Framebuffer_Map, gpu_vk_framebuffer_map, gpu_vk_framebuffer_key_hash, gpu_vk_framebuffer_key_equals);

// typedef struct Gpu_Vk_Transient_Key {
// 	// Gpu_Vk_Transient_Kind kind;
// } Gpu_Vk_Transient_Key;

// typedef struct Gpu_Vk_Transient {
// 	u32 rc;

// 	VkFramebuffer framebuffer;
// } Gpu_Vk_Transient;

// u64 gpu_vk_transient_key_hash(Gpu_Vk_Transient_Key key) {
// 	return 0;
// }
// bool gpu_vk_transient_key_equals(Gpu_Vk_Transient_Key a, Gpu_Vk_Transient_Key b) {
// 	return a.kind == b.kind;
// }

// sl_hashmap(Gpu_Vk_Transient_Key, Gpu_Vk_Transient*, Gpu_Vk_Transient_Map, gpu_vk_transient_map, gpu_vk_transient_key_hash, gpu_vk_transient_key_equals);
// sl_seq(Gpu_Vk_Transient, Seq_Gpu_Vk_Transient, seq_gpu_vk_transient);
// sl_seq(Gpu_Vk_Transient*, Seq_Gpu_Vk_Transient_Ptr, seq_gpu_vk_transient_ptr);

// typedef struct Gpu_Vk_Cleanup_Set {
// 	Seq_Gpu_Vk_Transient_Ptr resources;
// 	VkFence fence;
// } Gpu_Vk_Cleanup_Set;

typedef enum Gpu_Vk_Queue {
	Gpu_Vk_Queue_Graphics,
	Gpu_Vk_Queue_Compute,
	Gpu_Vk_Queue_Present,
	Gpu_Vk_Queue_Count
} Gpu_Vk_Queue;

typedef struct Gpu_Vk_Queue_Family_Indices {
	u32 index[Gpu_Vk_Queue_Count];
} Gpu_Vk_Queue_Family_Indices;

typedef struct Gpu_Vk_Memory_Type_Indices {
	u32 index[Gpu_Vk_Memory_Type_Count];
} Gpu_Vk_Memory_Type_Indices;

typedef struct Gpu_Vk_Frame_Recorder {
	VkCommandPool command_pool;

	VkSemaphore image_available_semaphore;
	VkSemaphore gpu_finished_semaphore;
	VkFence gpu_fence;
} Gpu_Vk_Frame_Recorder;

typedef struct Gpu_Vk_Heap_Data {
	u32 generation;
	VkDeviceMemory device_memory;
	Gpu_Vk_Memory_Type memory_type;
	u64 size;
	void* host_ptr;
} Gpu_Vk_Heap_Data;
sl_pool(Gpu_Vk_Heap_Data, Gpu_Vk_Heap_Pool, gpu_vk_heap_pool);

typedef struct Gpu_Vk_Texture_Data {
	u32 generation;
	VkImage image;
	VkImageView image_view;
} Gpu_Vk_Texture_Data;
sl_pool(Gpu_Vk_Texture_Data, Gpu_Vk_Texture_Pool, gpu_vk_texture_pool);

typedef enum Gpu_Vk_Command_Buffer_State {
	Gpu_Vk_Command_Buffer_State_Idle,
	Gpu_Vk_Command_Buffer_State_Recording,
	Gpu_Vk_Command_Buffer_State_Enqueued
} Gpu_Vk_Command_Buffer_State;

typedef struct Gpu_Vk_Command_Buffer_Data {
	Gpu_Vk_Command_Buffer_State state;
	VkCommandPool command_pool[Gpu_Vk_Queue_Count];
	VkFence fence;
} Gpu_Vk_Command_Buffer_Data;

typedef struct Gpu_Vk_Command_Buffer_Pool_Data {
	u32 generation;

	Gpu_Vk_Command_Buffer_Data* command_buffers;
	u32 command_buffer_count;
	u32 next_command_buffer;
} Gpu_Vk_Command_Buffer_Pool_Data;
sl_pool(Gpu_Vk_Command_Buffer_Pool_Data, Gpu_Vk_Command_Buffer_Pool_Pool, gpu_vk_command_buffer_pool_pool);

typedef struct Gpu_Vk {
	Allocator* allocator;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkSurfaceKHR surface;

	VkSwapchainKHR swapchain;
	VkExtent2D swapchain_extent;
	VkFormat swapchain_format;
	VkImage* swapchain_images;
	Gpu_Vk_Texture* swapchain_textures;
	u32 swapchain_image_count;
	Gpu_Vk_Swapchain_Desc swapchain_desc;
	
	VkRenderPass render_pass;

	Gpu_Vk_Memory_Type_Indices memory_type_indices;
	Gpu_Vk_Queue_Family_Indices queue_family_indices;
	VkQueue queue[Gpu_Vk_Queue_Count];

	Gpu_Vk_Heap_Pool heap_pool;
	Gpu_Vk_Texture_Pool texture_pool;
	Gpu_Vk_Command_Buffer_Pool_Pool command_pool_pool;

	Gpu_Vk_Framebuffer_Pool framebuffer_pool;
	Gpu_Vk_Framebuffer_Map framebuffer_map;
	Gpu_Vk_Framebuffer_LRU framebuffer_lru;

	u32 next_frame_recorder_idx;
	Gpu_Vk_Frame_Recorder frame_recorders[GPU_VK_MAX_INFLIGHT_FRAMES];
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

Gpu_Vk_Memory_Type_Indices gpu_vk_get_physical_device_memory_type_indices(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);

	VkMemoryPropertyFlagBits required_flags[Gpu_Vk_Memory_Type_Count] = {
		[Gpu_Vk_Memory_Type_Host_Visible] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		[Gpu_Vk_Memory_Type_Device_Local] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	};

	VkMemoryPropertyFlagBits undesirable_flags[Gpu_Vk_Memory_Type_Count] = {
		[Gpu_Vk_Memory_Type_Host_Visible] = 0,
		[Gpu_Vk_Memory_Type_Device_Local] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	};

	Gpu_Vk_Memory_Type_Indices indices = {
		.index = {
			u32_max,
			u32_max,
		},
	};

	for (Gpu_Vk_Memory_Type type = 0; type < Gpu_Vk_Memory_Type_Count; type++) {
		// Find optimal memory type
		for (u32 device_type = 0; device_type < properties.memoryTypeCount; device_type++) {
			const VkMemoryPropertyFlagBits device_type_flags = properties.memoryTypes[device_type].propertyFlags;
			const bool is_usable = ((device_type_flags & required_flags[type]) == required_flags[type]);
			const bool is_optimal = ((device_type_flags & undesirable_flags[type]) == 0);
			if (is_usable && is_optimal) {
				indices.index[type] = device_type;
				break;
			}
		}

		// Found optimal type, continue to next type.
		if (indices.index[type] != u32_max) {
			continue;
		}

		// Find usable memory type
		for (u32 device_type = 0; device_type < properties.memoryTypeCount; device_type++) {
			const VkMemoryPropertyFlagBits device_type_flags = properties.memoryTypes[device_type].propertyFlags;
			const bool is_usable = ((device_type_flags & required_flags[type]) == required_flags[type]);
			if (is_usable) {
				indices.index[type] = device_type;
				break;
			}
		}
	}

	return indices;
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
	vkEnumerateDeviceExtensionProperties(device, NULL, &available_extension_count, available_extensions);

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
	gpu_vk.queue_family_indices = queue_indices;

	gpu_vk.memory_type_indices = gpu_vk_get_physical_device_memory_type_indices(physical_device);
	
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

// Texture
void gpu_vk_texture_destroy(Gpu_Vk_Texture texture) {
	Gpu_Vk_Texture_Data* data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, texture);
	sl_assert(data != NULL, "Texture is invalid.");

	// 1. flush any pending command buffer cleanups
	// ...

	// 2. destroy all cached framebuffers that retain this texture (can assert that rc == 0).
	// ...

	vkDestroyImageView(gpu_vk.device, data->image_view, NULL);

	if (data->image != VK_NULL_HANDLE) {
		vkDestroyImage(gpu_vk.device, data->image, NULL);
	}

	gpu_vk_texture_pool_release(&gpu_vk.texture_pool, texture);
}

// Framebuffer
void gpu_vk_retain_framebuffer(Gpu_Vk_Framebuffer framebuffer) {
	Gpu_Vk_Framebuffer_Data* data = gpu_vk_framebuffer_pool_resolve(&gpu_vk.framebuffer_pool, framebuffer);
	data->rc++;
}
void gpu_vk_release_framebuffer(Gpu_Vk_Framebuffer framebuffer) {
	Gpu_Vk_Framebuffer_Data* data = gpu_vk_framebuffer_pool_resolve(&gpu_vk.framebuffer_pool, framebuffer);
	sl_assert(data->rc > 0, "Over-released framebuffer.");
	data->rc--;

	// TODO: Put on evictable LRU
}
Gpu_Vk_Framebuffer gpu_vk_acquire_framebuffer(Gpu_Vk_Framebuffer_Key key) {
	// Find existing entry
	{
		Gpu_Vk_Framebuffer result;
		if (gpu_vk_framebuffer_map_get(&gpu_vk.framebuffer_map, key, &result)) {
			gpu_vk_retain_framebuffer(result);
			return result;
		}
	}

	// Make new entry
	{
		gpu_vk_log("Creating new framebuffer.");
		Gpu_Vk_Framebuffer result = gpu_vk_framebuffer_pool_acquire(&gpu_vk.framebuffer_pool);
		Gpu_Vk_Framebuffer_Data* data = gpu_vk_framebuffer_pool_resolve(&gpu_vk.framebuffer_pool, result);
		data->rc = 1;
		data->key = key;

		VkImageView attachments[GPU_VK_MAX_ATTACHMENTS];
		for (u8 attachment_idx = 0; attachment_idx < key.attachment_count; attachment_idx++) {
			Gpu_Vk_Texture_Data* texture_data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, key.attachments[attachment_idx]);
			attachments[attachment_idx] = texture_data->image_view;
		}

		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = gpu_vk.render_pass,
			.attachmentCount = key.attachment_count,
			.pAttachments = attachments,
			.width = key.size.x,
			.height = key.size.y,
			.layers = 1,
		};
		
		VkResult create_result = vkCreateFramebuffer(gpu_vk.device, &framebuffer_info, NULL, &data->framebuffer);
		sl_assert(create_result == VK_SUCCESS, "Failed to create framebuffer.");

		gpu_vk_framebuffer_map_insert(&gpu_vk.framebuffer_map, key, result);

		return result;
	}
}

// Swapchain
void gpu_vk_ensure_valid_swapchain() {
	VkExtent2D extent = gpu_vk.swapchain_desc.get_swapchain_extent_fn(gpu_vk.swapchain_desc.ctx);
	
	const bool must_rebuild = (gpu_vk.swapchain == VK_NULL_HANDLE) || (extent.width != gpu_vk.swapchain_extent.width) || (extent.height != gpu_vk.swapchain_extent.height);
	if (!must_rebuild) {
		return;
	}

	vkDeviceWaitIdle(gpu_vk.device);

	// Free old image views/framebuffers
	for (u32 image_idx = 0; image_idx < gpu_vk.swapchain_image_count; image_idx++) {
		gpu_vk_texture_destroy(gpu_vk.swapchain_textures[image_idx]);
	}
	
	gpu_vk.swapchain_extent = extent;

	VkSurfaceCapabilitiesKHR capabilities;
	VkResult get_capabilities_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu_vk.physical_device, gpu_vk.surface, &capabilities);
	sl_assert(get_capabilities_result == VK_SUCCESS, "Failed to get swapchain capabilities.");

	VkColorSpaceKHR colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

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

	const u32 min_image_count = sl_clamp(3, capabilities.minImageCount, capabilities.maxImageCount);

	gpu_vk_log("Rebuilding swapchain with extent (%u, %u), %u min images.", extent.width, extent.height, min_image_count);

	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = gpu_vk.surface,
		.imageFormat = gpu_vk.swapchain_format,
		.imageColorSpace = colorspace,
		.imageExtent = extent,
		.minImageCount = min_image_count,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageArrayLayers = 1,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = gpu_vk.swapchain,
		.presentMode = present_mode,
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
			allocator_free(gpu_vk.allocator, gpu_vk.swapchain_textures, gpu_vk.swapchain_image_count);
		}
		gpu_vk.swapchain_image_count = swapchain_image_count;
		allocator_new(gpu_vk.allocator, gpu_vk.swapchain_images, swapchain_image_count);
		allocator_new(gpu_vk.allocator, gpu_vk.swapchain_textures, swapchain_image_count);
	}

	// Get images
	vkGetSwapchainImagesKHR(gpu_vk.device, gpu_vk.swapchain, &swapchain_image_count, gpu_vk.swapchain_images);

	// Create image views
	for (u32 image_idx = 0; image_idx < swapchain_image_count; image_idx++) {
		const VkImageViewCreateInfo view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = gpu_vk.swapchain_images[image_idx],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = gpu_vk.swapchain_format,
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
		
		VkImageView image_view;
		const VkResult view_create_result = vkCreateImageView(gpu_vk.device, &view_create_info, NULL, &image_view);
		sl_assert(view_create_result == VK_SUCCESS, "Failed to create image view for swapchain.");

		Gpu_Vk_Texture texture = gpu_vk_texture_pool_acquire(&gpu_vk.texture_pool);
		Gpu_Vk_Texture_Data* texture_data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, texture);
		texture_data->image_view = image_view;
		gpu_vk.swapchain_textures[image_idx] = texture;
	}
}

void gpu_vk_init_render_pass() {
	gpu_vk_log("Creating render pass.");

	VkAttachmentDescription color_att = {
		.format = gpu_vk.swapchain_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference color_att_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_att_ref,
	};

	VkRenderPassCreateInfo render_pass_create_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &color_att,
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};

	VkResult create_render_pass_result = vkCreateRenderPass(gpu_vk.device, &render_pass_create_info, NULL, &gpu_vk.render_pass);
	sl_assert(create_render_pass_result == VK_SUCCESS, "Failed to create render pass.");
}

void gpu_vk_init_frame_recorder(Gpu_Vk_Frame_Recorder* recorder) {
	*recorder = (Gpu_Vk_Frame_Recorder) { 0 };

	VkCommandPoolCreateInfo command_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = gpu_vk.queue_family_indices.index[Gpu_Vk_Queue_Graphics],
	};

	VkResult create_command_pool_result = vkCreateCommandPool(gpu_vk.device, &command_pool_create_info, NULL, &recorder->command_pool);
	sl_assert(create_command_pool_result == VK_SUCCESS, "Failed to create command pool.");

	VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	vkCreateSemaphore(gpu_vk.device, &semaphore_create_info, NULL, &recorder->image_available_semaphore);
	vkCreateSemaphore(gpu_vk.device, &semaphore_create_info, NULL, &recorder->gpu_finished_semaphore);

	VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};
	vkCreateFence(gpu_vk.device, &fence_create_info, NULL, &recorder->gpu_fence);
}

void gpu_vk_frame_recorder_begin(Gpu_Vk_Frame_Recorder* recorder) {
	// Wait for command buffers associated with the recorder to be completed.
	vkWaitForFences(gpu_vk.device, 1, &recorder->gpu_fence, true, u64_max);
	vkResetFences(gpu_vk.device, 1, &recorder->gpu_fence);

	vkResetCommandPool(gpu_vk.device, recorder->command_pool, 0);

	gpu_vk_ensure_valid_swapchain();

	u32 image_index;
	VkResult acquire_image_result = vkAcquireNextImageKHR(gpu_vk.device, gpu_vk.swapchain, u64_max, recorder->image_available_semaphore, VK_NULL_HANDLE, &image_index);
	sl_assert(acquire_image_result == VK_SUCCESS, "Failed to acquire next swapchain image.");

	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = recorder->command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer command_buffer;
	VkResult alloc_result = vkAllocateCommandBuffers(gpu_vk.device, &alloc_info, &command_buffer);
	sl_assert(alloc_result == VK_SUCCESS, "Failed to allocate command buffer.");

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};
	VkResult begin_result = vkBeginCommandBuffer(command_buffer, &begin_info);
	sl_assert(begin_result == VK_SUCCESS, "Failed to begin command buffer.");

	VkClearValue clear_values[] = {
		{
			.color = {
				.float32 = { 0.2f, 0.3f, 0.4f, 1.0f },
			},
		}
	};

	// Acquire cached framebuffer
	Gpu_Vk_Framebuffer_Key fb_key = {
		.attachments = {
			[0] = gpu_vk.swapchain_textures[image_index],
		},
		.attachment_count = 1,
		.size = { gpu_vk.swapchain_extent.width, gpu_vk.swapchain_extent.height },
	};
	Gpu_Vk_Framebuffer fb = gpu_vk_acquire_framebuffer(fb_key);
	Gpu_Vk_Framebuffer_Data* fb_data = gpu_vk_framebuffer_pool_resolve(&gpu_vk.framebuffer_pool, fb);

	// Render pass
	VkRenderPassBeginInfo render_pass_begin_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = gpu_vk.render_pass,
		.framebuffer = fb_data->framebuffer,
		.renderArea = {
			.offset = { 0, 0 },
			.extent = gpu_vk.swapchain_extent,
		},
		.clearValueCount = sl_array_count(clear_values),
		.pClearValues = clear_values,
	};

	vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(command_buffer);

	VkResult end_result = vkEndCommandBuffer(command_buffer);
	sl_assert(begin_result == VK_SUCCESS, "Failed to end command buffer.");

	// graphics queue submit
    {
		VkSemaphore wait_semaphores[] = {
			recorder->image_available_semaphore,
		};
		VkPipelineStageFlags wait_dst_stage_mask[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		};
		VkSemaphore signal_semaphores[] = {
			recorder->gpu_finished_semaphore,
		};

		VkSubmitInfo submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = sl_array_count(wait_semaphores),
			.pWaitSemaphores = wait_semaphores,
			.pWaitDstStageMask = wait_dst_stage_mask,
			.signalSemaphoreCount = sl_array_count(signal_semaphores),
			.pSignalSemaphores = signal_semaphores,
			.pCommandBuffers = &command_buffer,
			.commandBufferCount = 1,
		};

		VkResult submit_result = vkQueueSubmit(gpu_vk.queue[Gpu_Vk_Queue_Graphics], 1, &submit_info, recorder->gpu_fence);
		sl_assert(submit_result == VK_SUCCESS, "Failed to submit command buffer.");
    }

	// Present
	{
        VkSemaphore wait_semaphores[] = {
            recorder->gpu_finished_semaphore,
        };
		VkPresentInfoKHR present_info = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = sl_array_count(wait_semaphores),
			.pWaitSemaphores = wait_semaphores,
			.pSwapchains = &gpu_vk.swapchain,
			.swapchainCount = 1,
			.pImageIndices = &image_index,
		};
		
		VkResult present_result = vkQueuePresentKHR(gpu_vk.queue[Gpu_Vk_Queue_Present], &present_info);
		sl_assert((present_result == VK_SUCCESS) || (present_result == VK_ERROR_OUT_OF_DATE_KHR) || (present_result == VK_SUBOPTIMAL_KHR), "Failed to present command buffer.");
    }
}

void gpu_vk_init_frame_recorders() {
	gpu_vk_log("Creating frame recorders.");
	for (u32 recorder_idx = 0; recorder_idx < GPU_VK_MAX_INFLIGHT_FRAMES; recorder_idx++) {
		gpu_vk_init_frame_recorder(&gpu_vk.frame_recorders[recorder_idx]);
	}
}

Gpu_Vk_Frame_Recorder* gpu_vk_acquire_frame_recorder() {
	const u32 recorder_idx = gpu_vk.next_frame_recorder_idx;
	gpu_vk.next_frame_recorder_idx = (recorder_idx + 1) % GPU_VK_MAX_INFLIGHT_FRAMES;
	return &gpu_vk.frame_recorders[recorder_idx];
}

void gpu_vk_init_resource_pools() {
	gpu_vk.heap_pool = gpu_vk_heap_pool_new(gpu_vk.allocator, 8);
	gpu_vk.texture_pool = gpu_vk_texture_pool_new(gpu_vk.allocator, 8);
	gpu_vk.command_pool_pool = gpu_vk_command_buffer_pool_pool_new(gpu_vk.allocator, 8);

	gpu_vk.framebuffer_pool = gpu_vk_framebuffer_pool_new(gpu_vk.allocator, 8);
	gpu_vk.framebuffer_map = gpu_vk_framebuffer_map_new(gpu_vk.allocator, 64);
	gpu_vk.framebuffer_lru = (Gpu_Vk_Framebuffer_LRU) {
		.oldest = SL_HANDLE_NULL,
		.newest = SL_HANDLE_NULL,
	};
}

void gpu_vk_init(const Gpu_Vk_Desc* desc) {
	gpu_vk = (Gpu_Vk) {};
	gpu_vk.allocator = desc->allocator;
	gpu_vk.swapchain_desc = desc->swapchain_desc;
	gpu_vk.swapchain_format = VK_FORMAT_B8G8R8A8_SRGB;
	gpu_vk_init_instance(desc);
	gpu_vk_init_surface();
	gpu_vk_init_device();
	gpu_vk_init_render_pass();
	gpu_vk_init_frame_recorders();
	gpu_vk_init_resource_pools();
	gpu_vk_ensure_valid_swapchain();
}
void gpu_vk_deinit() {
	vkDestroyInstance(gpu_vk.instance, NULL);
	gpu_vk = (Gpu_Vk) {};
}

void gpu_vk_dummy_render() {
	Gpu_Vk_Frame_Recorder* recorder = gpu_vk_acquire_frame_recorder();
	gpu_vk_frame_recorder_begin(recorder);
}

// Heap
Gpu_Vk_Heap gpu_vk_new_heap(u64 bytes, Gpu_Vk_Memory_Type memory_type) {
	gpu_vk_log("Allocating heap of size %lu.", bytes);

	Gpu_Vk_Heap heap_handle = gpu_vk_heap_pool_acquire(&gpu_vk.heap_pool);
	Gpu_Vk_Heap_Data* heap_data = gpu_vk_heap_pool_resolve(&gpu_vk.heap_pool, heap_handle);

	VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = bytes,
		.memoryTypeIndex = gpu_vk.memory_type_indices.index[memory_type],
	};

	VkResult allocate_result = vkAllocateMemory(gpu_vk.device, &allocate_info, NULL, &heap_data->device_memory);
	sl_assert(allocate_result == VK_SUCCESS, "Failed to allocate memory.");

	heap_data->memory_type = memory_type;
	heap_data->size = bytes;

	switch (memory_type) {
		case Gpu_Vk_Memory_Type_Host_Visible: {
			VkResult map_result = vkMapMemory(gpu_vk.device, heap_data->device_memory, 0, VK_WHOLE_SIZE, 0, &heap_data->host_ptr);
			sl_assert(map_result == VK_SUCCESS, "Failed to map slice.");
		} break;

		case Gpu_Vk_Memory_Type_Device_Local: {
			heap_data->host_ptr = NULL;
		} break;

		case Gpu_Vk_Memory_Type_Count: {
			sl_abort("Invalid memory type.");
		} break;
	}

	return heap_handle;
}
void gpu_vk_destroy_heap(Gpu_Vk_Heap heap) {
	Gpu_Vk_Heap_Data* heap_data = gpu_vk_heap_pool_resolve(&gpu_vk.heap_pool, heap);
	
	switch (heap_data->memory_type) {
		case Gpu_Vk_Memory_Type_Host_Visible: {
			vkUnmapMemory(gpu_vk.device, heap_data->device_memory);
		} break;

		case Gpu_Vk_Memory_Type_Device_Local: {
			// Do nothing
		} break;

		case Gpu_Vk_Memory_Type_Count: {
			sl_abort("Invalid memory type.");
		} break;
	}

	vkFreeMemory(gpu_vk.device, heap_data->device_memory, NULL);
	gpu_vk_heap_pool_release(&gpu_vk.heap_pool, heap);
}
u64 gpu_vk_get_heap_size(Gpu_Vk_Heap heap) {
	Gpu_Vk_Heap_Data* heap_data = gpu_vk_heap_pool_resolve(&gpu_vk.heap_pool, heap);
	return heap_data->size;
}

// Slice
void* gpu_vk_get_slice_host_ptr(Gpu_Vk_Slice slice) {
	Gpu_Vk_Heap_Data* heap_data = gpu_vk_heap_pool_resolve(&gpu_vk.heap_pool, slice.heap);
	sl_assert(heap_data->memory_type == Gpu_Vk_Memory_Type_Host_Visible, "Can only map host-visible memory.");
	return heap_data->host_ptr + slice.offset;
}
void gpu_vk_flush_slice(Gpu_Vk_Slice slice) {
	Gpu_Vk_Heap_Data* heap_data = gpu_vk_heap_pool_resolve(&gpu_vk.heap_pool, slice.heap);
	sl_assert(heap_data->memory_type == Gpu_Vk_Memory_Type_Host_Visible, "Flush only makes sense for host-visible memory.");
	
	VkMappedMemoryRange memory_range = {
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.offset = slice.offset,
		.size = slice.size,
		.memory = heap_data->device_memory
	};

	VkResult invalidate_result = vkInvalidateMappedMemoryRanges(gpu_vk.device, 1, &memory_range);
	sl_assert(invalidate_result == VK_SUCCESS, "Failed to invalidate slice.");

	// To simplify the API, we wait for upload to complete before returning.
	// This is sub-optimal.
	VkResult flush_result = vkFlushMappedMemoryRanges(gpu_vk.device, 1, &memory_range);
	sl_assert(invalidate_result == VK_SUCCESS, "Failed to flush slice upload.");
}

// Command Buffer
void gpu_init_command_buffer(Gpu_Vk_Command_Buffer_Data* command_buffer) {
	*command_buffer = (Gpu_Vk_Command_Buffer_Data) {0};

	for (Gpu_Vk_Queue queue = 0; queue < Gpu_Vk_Queue_Count; queue++) {
		VkCommandPoolCreateInfo command_pool_create_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = gpu_vk.queue_family_indices.index[queue],
		};
		VkResult command_pool_result = vkCreateCommandPool(gpu_vk.device, &command_pool_create_info, NULL, &command_buffer->command_pool[queue]);
		sl_assert(command_pool_result == VK_SUCCESS, "Failed to create command pool.");
	}

	VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	};
	VkResult fence_result = vkCreateFence(gpu_vk.device, &fence_create_info, NULL, &command_buffer->fence);
	sl_assert(fence_result == VK_SUCCESS, "Failed to create fence.");
}

// Command Buffer Pool
Gpu_Vk_Command_Buffer_Pool gpu_vk_new_command_buffer_pool(u32 size) {
	gpu_vk_log("Creating command buffer pool of size %u.", size);
	Gpu_Vk_Command_Buffer_Pool pool = gpu_vk_command_buffer_pool_pool_acquire(&gpu_vk.command_pool_pool);
	Gpu_Vk_Command_Buffer_Pool_Data* pool_data = gpu_vk_command_buffer_pool_pool_resolve(&gpu_vk.command_pool_pool, pool);

	allocator_new(gpu_vk.allocator, pool_data->command_buffers, size);
	pool_data->command_buffer_count = size;

	for (u32 i = 0; i < size; i++) {
		gpu_init_command_buffer(&pool_data->command_buffers[i]);
	}

	return pool;
}
void gpu_vk_destroy_command_buffer_pool(Gpu_Vk_Command_Buffer_Pool pool) {
	// todo
}
Gpu_Vk_Command_Buffer_Data* gpu_vk_resolve_command_buffer_data(Gpu_Vk_Command_Buffer cb) {
	Gpu_Vk_Command_Buffer_Pool_Data* pool_data = gpu_vk_command_buffer_pool_pool_resolve(&gpu_vk.command_pool_pool, cb.pool);
	if (pool_data == NULL) {
		return NULL;
	}
	if (cb.index > pool_data->command_buffer_count) {
		return NULL;
	}
	return &pool_data->command_buffers[cb.index];
}

bool gpu_vk_new_command_buffer(Gpu_Vk_Command_Buffer_Pool pool, Gpu_Vk_Command_Buffer* out_cb) {
	Gpu_Vk_Command_Buffer_Pool_Data* pool_data = gpu_vk_command_buffer_pool_pool_resolve(&gpu_vk.command_pool_pool, pool);
	sl_assert(pool_data != NULL, "Pool is invalid.");

	Gpu_Vk_Command_Buffer cb_handle = {
		.pool = pool,
		.index = pool_data->next_command_buffer,
	};

	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb_handle);
	if (cb_data->state == Gpu_Vk_Command_Buffer_State_Recording) {
		// Command pool exceeded.
		return false;
	}

	cb_data->state = Gpu_Vk_Command_Buffer_State_Recording;

	pool_data->next_command_buffer = (pool_data->next_command_buffer + 1) % pool_data->command_buffer_count;
	
	*out_cb = cb_handle;
	return true;
}
void gpu_vk_enqueue(Gpu_Vk_Command_Buffer cb, bool wait_until_completed) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");
	cb_data->state = Gpu_Vk_Command_Buffer_State_Enqueued;
}