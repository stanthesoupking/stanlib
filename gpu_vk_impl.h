#pragma once

#include "core.h"
#include "vulkan/vulkan_core.h"
#include <stdatomic.h>
#define GPU_VK_LOGGING 1
#define GPU_VK_VALIDATION 0

#include "gpu_vk.h"
#include <string.h>

#if GPU_VK_LOGGING
	#define gpu_vk_log(fmt, ...) \
		printf("[Gpu_Vk]: " fmt "\n", ##__VA_ARGS__)
#else
	#define gpu_vk_log(fmt, ...) ((void)0)
#endif

#if GPU_VK_VALIDATION
	#define gpu_vk_validate(condition, message) sl_assert(condition, message)
#else
	#define gpu_vk_validate(condition, message) ((void)0)
#endif

const char* GPU_VK_DEVICE_EXTENSIONS[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
	VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
};

typedef SL_Handle Gpu_Vk_Framebuffer;

typedef struct Gpu_Vk_Framebuffer_Key_Attachment {
	Gpu_Vk_Texture texture;
	Gpu_Vk_Load_Op load_op;
	Gpu_Vk_Store_Op store_op;
} Gpu_Vk_Framebuffer_Key_Attachment;

typedef struct Gpu_Vk_Framebuffer_Key {
	Gpu_Vk_Framebuffer_Key_Attachment attachments[GPU_VK_MAX_ATTACHMENTS];
	u8 attachment_count;
} Gpu_Vk_Framebuffer_Key;

sl_inline Gpu_Vk_Framebuffer_Key gpu_vk_framebuffer_key_for_render_pass(const Gpu_Vk_Render_Pass* render_pass) {
	Gpu_Vk_Framebuffer_Key result = {
		.attachment_count = render_pass->attachment_count,
	};
	for (u8 attachment_idx = 0; attachment_idx < render_pass->attachment_count; attachment_idx++) {
		const Gpu_Vk_Render_Attachment* render_attachment = &render_pass->attachments[attachment_idx];
		result.attachments[attachment_idx] = (Gpu_Vk_Framebuffer_Key_Attachment) {
			.texture = render_attachment->texture,
			.load_op = render_attachment->load_op,
			.store_op = render_attachment->store_op,
		};
	}
	return result;
}

typedef struct Gpu_Vk_Framebuffer_Data {
	u32 generation;

	Gpu_Vk_Framebuffer_Key key;
	u32 rc;

	VkFramebuffer framebuffer;
	VkRenderPass render_pass;

	// todo
	Gpu_Vk_Framebuffer older;
	Gpu_Vk_Framebuffer newer;
} Gpu_Vk_Framebuffer_Data;

typedef struct Gpu_Vk_Framebuffer_LRU {
	Gpu_Vk_Framebuffer oldest;
	Gpu_Vk_Framebuffer newest;
} Gpu_Vk_Framebuffer_LRU;

sl_inline u64 gpu_vk_framebuffer_key_hash(Gpu_Vk_Framebuffer_Key key) {
	SL_Hasher hasher;
	sl_hasher_init(&hasher);
	sl_hasher_push(&hasher, immutable_buffer_for(key.attachment_count));
	for (u8 attachment_idx = 0; attachment_idx < key.attachment_count; attachment_idx++) {
		sl_hasher_push(&hasher, immutable_buffer_for(key.attachments[attachment_idx]));
	}
	return sl_hasher_finalise(&hasher);
}
sl_inline bool gpu_vk_framebuffer_key_equals(Gpu_Vk_Framebuffer_Key a, Gpu_Vk_Framebuffer_Key b) {
	if (a.attachment_count != b.attachment_count) {
		return false;
	}
	for (u8 attachment_idx = 0; attachment_idx < a.attachment_count; attachment_idx++) {
		if ((a.attachments[attachment_idx].texture.index != b.attachments[attachment_idx].texture.index) || (a.attachments[attachment_idx].texture.generation != b.attachments[attachment_idx].texture.generation) || (a.attachments[attachment_idx].load_op != b.attachments[attachment_idx].load_op) || (a.attachments[attachment_idx].store_op != b.attachments[attachment_idx].store_op)) {
			return false;
		}
	}
	return true;
}

sl_pool(Gpu_Vk_Framebuffer_Data, Gpu_Vk_Framebuffer_Pool, gpu_vk_framebuffer_pool);
sl_hashmap(Gpu_Vk_Framebuffer_Key, Gpu_Vk_Framebuffer, Gpu_Vk_Framebuffer_Map, gpu_vk_framebuffer_map, gpu_vk_framebuffer_key_hash, gpu_vk_framebuffer_key_equals);

typedef enum Gpu_Vk_Queue {
	// Graphics + Compute
	Gpu_Vk_Queue_Primary,

	// Swapchain Present
	Gpu_Vk_Queue_Present,

	Gpu_Vk_Queue_Count
} Gpu_Vk_Queue;

typedef struct Gpu_Vk_Queue_Family_Indices {
	u32 index[Gpu_Vk_Queue_Count];
} Gpu_Vk_Queue_Family_Indices;

typedef struct Gpu_Vk_Memory_Type_Indices {
	u32 index[Gpu_Vk_Memory_Type_Count];
} Gpu_Vk_Memory_Type_Indices;

typedef struct Gpu_Vk_Heap_Data {
	u32 generation;
	VkDeviceMemory device_memory;
	VkBuffer buffer;
	Gpu_Vk_Memory_Type memory_type;
	u64 size;
	void* host_ptr;
} Gpu_Vk_Heap_Data;
sl_pool(Gpu_Vk_Heap_Data, Gpu_Vk_Heap_Pool, gpu_vk_heap_pool);

typedef enum Gpu_Vk_Texture_Data_Kind {
	Gpu_Vk_Texture_Data_Kind_Immediate,
} Gpu_Vk_Texture_Data_Kind;

typedef struct Gpu_Vk_Texture_Data {
	u32 generation;

	Gpu_Vk_Texture_Data_Kind data_kind;

	union {
		// Gpu_Vk_Texture_Kind_Immediate
		struct {
			bool owned_image;
			VkImage image;
			VkImageView image_view;
			Gpu_Vk_Texture_Layout layout;
			vec3_u32 size;
			VkFormat format;
		} imm;
	};
} Gpu_Vk_Texture_Data;
sl_pool(Gpu_Vk_Texture_Data, Gpu_Vk_Texture_Pool, gpu_vk_texture_pool);

typedef struct Gpu_Vk_Compute_Pipeline_Data {
	u32 generation;

	VkShaderModule shader_module;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
} Gpu_Vk_Compute_Pipeline_Data;
sl_pool(Gpu_Vk_Compute_Pipeline_Data, Gpu_Vk_Compute_Pipeline_Pool, gpu_vk_compute_pipeline_pool);

typedef struct Gpu_Vk_Semaphore_On_Notify_Callback {
	u64 value;
	void* ctx;
	Gpu_Vk_On_Notify_Fn fn;
} Gpu_Vk_Semaphore_On_Notify_Callback;
sl_seq(Gpu_Vk_Semaphore_On_Notify_Callback, Gpu_Vk_Semaphore_On_Notify_Callback_Seq, gpu_vk_semaphore_on_notify_callback_seq);

typedef struct Gpu_Vk_Semaphore_Data {
	u32 generation;

	bool destroying;

	// timeline semaphore
	VkSemaphore vk_semaphore;

	Gpu_Vk_Semaphore_On_Notify_Callback_Seq pending_callbacks;

	u64 current_value;
	u64 next_signal_value;

	SL_Mutex mutex;
	SL_Thread thread;
} Gpu_Vk_Semaphore_Data;
sl_pool(Gpu_Vk_Semaphore_Data, Gpu_Vk_Semaphore_Pool, gpu_vk_semaphore_pool);

typedef struct Gpu_Vk_Swapchain_Instance {
	atomic_u32 rc;
	Gpu_Vk_Swapchain_Desc desc;
	VkSwapchainKHR swapchain;

	VkSemaphore* present_semaphores;
	Gpu_Vk_Texture* textures;
	u32 texture_count;
} Gpu_Vk_Swapchain_Instance;

typedef struct Gpu_Vk_Swapchain_Data {
	u32 generation;
	VkSurfaceKHR surface;
	Gpu_Vk_Swapchain_Instance* current_instance;
} Gpu_Vk_Swapchain_Data;
sl_pool(Gpu_Vk_Swapchain_Data, Gpu_Vk_Swapchain_Pool, gpu_vk_swapchain_pool);

typedef enum Gpu_Vk_Command_Buffer_State {
	Gpu_Vk_Command_Buffer_State_Idle,
	Gpu_Vk_Command_Buffer_State_Recording,
	Gpu_Vk_Command_Buffer_State_Enqueued
} Gpu_Vk_Command_Buffer_State;

typedef enum Gpu_Vk_Command_Kind {
	Gpu_Vk_Command_Kind_Transition_Texture_Layouts,
	Gpu_Vk_Command_Kind_Begin_Render,
	Gpu_Vk_Command_Kind_End_Render,
	Gpu_Vk_Command_Kind_Dispatch,
	Gpu_Vk_Command_Kind_Blit,
	Gpu_Vk_Command_Kind_Barrier,
	Gpu_Vk_Command_Kind_Wait,
	Gpu_Vk_Command_Kind_Signal,
} Gpu_Vk_Command_Kind;

typedef struct Gpu_Vk_Command_Begin_Render {
	Gpu_Vk_Render_Pass render_pass;
} Gpu_Vk_Command_Begin_Render;

typedef struct Gpu_Vk_Command_Dispatch {
	Gpu_Vk_Compute_Pipeline pipeline;
	const Gpu_Vk_Binding* bindings;
	u32 binding_count;
	vec3_u32 group_count;
} Gpu_Vk_Command_Dispatch;

typedef struct Gpu_Vk_Command_Transition_Texture_Layouts {
	const Gpu_Vk_Texture* textures;
	const Gpu_Vk_Texture_Layout* layouts;
	u32 count;
} Gpu_Vk_Command_Transition_Texture_Layouts;

typedef struct Gpu_Vk_Command_Blit {
	Gpu_Vk_Blit_Desc desc;
} Gpu_Vk_Command_Blit;

typedef struct Gpu_Vk_Command_Wait {
	VkSemaphore semaphore;
	u64 value;
} Gpu_Vk_Command_Wait;

typedef struct Gpu_Vk_Command_Signal {
	VkSemaphore semaphore;
	u64 value;
} Gpu_Vk_Command_Signal;

typedef struct Gpu_Vk_Command {
	Gpu_Vk_Command_Kind kind;

	union {
		Gpu_Vk_Command_Transition_Texture_Layouts* transition_texture_layouts;
		Gpu_Vk_Command_Begin_Render* begin_render;
		Gpu_Vk_Command_Dispatch* dispatch;
		Gpu_Vk_Command_Blit* blit;
		Gpu_Vk_Command_Wait* wait;
		Gpu_Vk_Command_Signal* signal;
	} data;
} Gpu_Vk_Command;
sl_seq(Gpu_Vk_Command, Gpu_Vk_Command_Seq, gpu_vk_command_seq);

sl_seq(VkCommandBuffer, Gpu_Vk_Command_Buffer_Seq, gpu_vk_command_buffer_seq);
sl_seq(VkSemaphore, Gpu_Vk_Semaphore_Seq, gpu_vk_semaphore_seq);

typedef struct Gpu_Vk_Swapchain_Present {
	Gpu_Vk_Swapchain_Instance* swapchain_instance; // retained
	Gpu_Vk_Texture texture;
	u32 image_index;
	VkSemaphore image_available_semaphore;
	VkSemaphore present_semaphore;
} Gpu_Vk_Swapchain_Present;
sl_seq(Gpu_Vk_Swapchain_Present, Gpu_Vk_Swapchain_Present_Seq, gpu_vk_swapchain_present_seq);

typedef struct Gpu_Vk_Command_Buffer_Data {
	Gpu_Vk_Command_Buffer_State state;
	VkCommandPool command_pool;

	// On the global semaphore, the value that indicates that GPU work for the command buffer has completed.
	u64 gpu_complete_semaphore_value;

	Gpu_Vk_Semaphore_Seq semaphores;
	u32 next_free_semaphore;

	Gpu_Vk_Command_Buffer_Seq command_buffers;
	u32 next_free_command_buffer;

	Gpu_Vk_Swapchain_Present_Seq swapchain_presents;

	SL_Arena_Allocator* arena;
	Gpu_Vk_Command_Seq commands;
} Gpu_Vk_Command_Buffer_Data;

typedef struct Gpu_Vk_Command_Buffer_Pool_Data {
	u32 generation;

	Gpu_Vk_Command_Buffer_Data* command_buffers;
	u32 command_buffer_count;
	u32 next_command_buffer;
} Gpu_Vk_Command_Buffer_Pool_Data;
sl_pool(Gpu_Vk_Command_Buffer_Pool_Data, Gpu_Vk_Command_Buffer_Pool_Pool, gpu_vk_command_buffer_pool_pool);

typedef struct Gpu_Vk_Device_Function_Table {
	PFN_vkCmdPipelineBarrier2KHR vkCmdPipelineBarrier2KHR;
	PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR;
} Gpu_Vk_Device_Function_Table;

typedef struct Gpu_Vk {
	Allocator* allocator;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	Gpu_Vk_Device_Function_Table device_function_table;
	VkPhysicalDeviceLimits device_limits;

	Gpu_Vk_Semaphore global_semaphore;
	u64 global_semaphore_value;

	Gpu_Vk_Swapchain_Init_Desc swapchain_init_desc;

	Gpu_Vk_Memory_Type_Indices memory_type_indices;
	Gpu_Vk_Queue_Family_Indices queue_family_indices;
	VkQueue queue[Gpu_Vk_Queue_Count];

	Gpu_Vk_Heap_Pool heap_pool;
	Gpu_Vk_Texture_Pool texture_pool;
	Gpu_Vk_Command_Buffer_Pool_Pool command_pool_pool;
	Gpu_Vk_Compute_Pipeline_Pool compute_pipeline_pool;
	Gpu_Vk_Swapchain_Pool swapchain_pool;
	Gpu_Vk_Semaphore_Pool semaphore_pool;

	Gpu_Vk_Framebuffer_Pool framebuffer_pool;
	Gpu_Vk_Framebuffer_Map framebuffer_map;
	Gpu_Vk_Framebuffer_LRU framebuffer_lru;
} Gpu_Vk;

static Gpu_Vk gpu_vk;

void gpu_vk_init_instance(const Gpu_Vk_Desc* desc) {
	gpu_vk_log("Creating instance.");

	Allocator* allocator = desc->allocator;

	const char* internal_required_extensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		//VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, // this breaks render doc
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME, // renderdoc
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

	//create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

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
			u32_max
		},
	};
	for (u32 queue_idx = 0; queue_idx < queue_family_count; queue_idx++) {
		VkQueueFamilyProperties queue_family = queue_families[queue_idx];

		// TODO
		// VkBool32 present_support = false;
		// vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_idx, gpu_vk.surface, &present_support);
		bool present_support = true;

		const VkQueueFlagBits primary_queue_bits = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

		if (indices.index[Gpu_Vk_Queue_Primary] == u32_max && (queue_family.queueFlags & primary_queue_bits) == primary_queue_bits) {
			indices.index[Gpu_Vk_Queue_Primary] = queue_idx;
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

	if (!device_features.shaderInt16) {
		return false;
	}
	if (!device_features.shaderInt64) {
		return false;
	}

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

void gpu_vk_init_device(void) {
	Allocator* allocator = gpu_vk.allocator;

	VkPhysicalDevice physical_device = gpu_vk_find_physical_device();
	gpu_vk.physical_device = physical_device;

	VkPhysicalDeviceProperties physical_device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
	gpu_vk_log("Creating logical device for \"%s\".", physical_device_properties.deviceName);

	gpu_vk.device_limits = physical_device_properties.limits;

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

	VkPhysicalDeviceFeatures device_features = {
		.shaderInt16 = true,
		.shaderInt64 = true,
	};

	VkDeviceCreateInfo device_create_info = {0};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pQueueCreateInfos = queue_create_infos;
	device_create_info.queueCreateInfoCount = queue_create_info_count;
	device_create_info.pEnabledFeatures = &device_features;
	device_create_info.ppEnabledExtensionNames = GPU_VK_DEVICE_EXTENSIONS;
	device_create_info.enabledExtensionCount = sl_array_count(GPU_VK_DEVICE_EXTENSIONS);

	VkPhysicalDeviceSynchronization2FeaturesKHR sync2_features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
		.synchronization2 = VK_TRUE,
	};
	device_create_info.pNext = &sync2_features;

	VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
		.timelineSemaphore = VK_TRUE,
	};
	sync2_features.pNext = &timeline_semaphore_features;

	VkResult create_device_result = vkCreateDevice(physical_device, &device_create_info, NULL, &gpu_vk.device);
	sl_assert(create_device_result == VK_SUCCESS, "Failed to create logical device.");

	allocator_free(gpu_vk.allocator, queue_create_infos, queue_family_count);

	for (Gpu_Vk_Queue queue = 0; queue < Gpu_Vk_Queue_Count; queue++) {
		vkGetDeviceQueue(gpu_vk.device, queue_indices.index[queue], 0, &gpu_vk.queue[queue]);
	}

	gpu_vk.device_function_table = (Gpu_Vk_Device_Function_Table) {
		.vkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)vkGetDeviceProcAddr(gpu_vk.device, "vkCmdPipelineBarrier2KHR"),
		.vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(gpu_vk.device, "vkCmdPushDescriptorSetKHR"),
	};
}

// Texture
VkImageView gpu_vk_texture_get_image_view(Gpu_Vk_Texture texture) {
	Gpu_Vk_Texture_Data* data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, texture);
	sl_assert(data != NULL, "Texture is invalid.");

	switch (data->data_kind) {
		case Gpu_Vk_Texture_Data_Kind_Immediate: {
			return data->imm.image_view;
		} break;
	}
}
Gpu_Vk_Texture gpu_vk_texture_get_root(Gpu_Vk_Texture texture) {
	Gpu_Vk_Texture_Data* data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, texture);
	sl_assert(data != NULL, "Texture is invalid.");

	switch (data->data_kind) {
		case Gpu_Vk_Texture_Data_Kind_Immediate: {
			return texture;
		} break;
	}
}
Gpu_Vk_Texture_Data* gpu_vk_texture_get_root_data(Gpu_Vk_Texture texture) {
	return gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, gpu_vk_texture_get_root(texture));
}


sl_inline VkImageType gpu_vk_texture_kind_to_vk_image_type(Gpu_Vk_Texture_Kind kind) {
	switch (kind) {
		case Gpu_Vk_Texture_Kind_1D: return VK_IMAGE_TYPE_1D;
		case Gpu_Vk_Texture_Kind_2D: return VK_IMAGE_TYPE_2D;
		case Gpu_Vk_Texture_Kind_3D: return VK_IMAGE_TYPE_3D;
	}
}
sl_inline VkImageViewType gpu_vk_texture_kind_to_vk_image_view_type(Gpu_Vk_Texture_Kind kind) {
	switch (kind) {
		case Gpu_Vk_Texture_Kind_1D: return VK_IMAGE_VIEW_TYPE_1D;
		case Gpu_Vk_Texture_Kind_2D: return VK_IMAGE_VIEW_TYPE_2D;
		case Gpu_Vk_Texture_Kind_3D: return VK_IMAGE_VIEW_TYPE_3D;
	}
}
sl_inline VkImageUsageFlags gpu_vk_texture_usage_to_vk_image_usage_flags(Gpu_Vk_Texture_Usage usage) {
	VkImageUsageFlags flags = 0;

	// For blit
	flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if ((usage & Gpu_Vk_Texture_Usage_Shader_Read) > 0) {
		flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	if ((usage & Gpu_Vk_Texture_Usage_Shader_Write) > 0) {
		flags |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if ((usage & Gpu_Vk_Texture_Usage_Render_Attachment) > 0) {
		flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	return flags;
}

sl_inline VkFormat gpu_vk_format_to_vk_format(Gpu_Vk_Format format) {
	switch (format) {
		case Gpu_Vk_Format_RGBA8_Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
		case Gpu_Vk_Format_RGBA8_sRGB: return VK_FORMAT_R8G8B8A8_SRGB;
		case Gpu_Vk_Format_BGRA8_Unorm: return VK_FORMAT_B8G8R8A8_UNORM;
		case Gpu_Vk_Format_BGRA8_sRGB: return VK_FORMAT_B8G8R8A8_SRGB;
		case Gpu_Vk_Format_RGBA16_Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Gpu_Vk_Format_RGBA32_Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
}

sl_inline VkColorSpaceKHR gpu_vk_colorspace_to_vk_colorspace(Gpu_Vk_Colorspace colorspace) {
	switch (colorspace) {
		case Gpu_Vk_Colorspace_sRGB: return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		case Gpu_Vk_Colorspace_Extended_sRGB_Linear: return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
	}
}

sl_inline VkImageLayout gpu_vk_texture_layout_to_vk_image_layout(Gpu_Vk_Texture_Layout layout) {
	switch (layout) {
		case Gpu_Vk_Texture_Layout_Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
		case Gpu_Vk_Texture_Layout_General: return VK_IMAGE_LAYOUT_GENERAL;
		case Gpu_Vk_Texture_Layout_Shader_Read: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case Gpu_Vk_Texture_Layout_Shader_Write: return VK_IMAGE_LAYOUT_GENERAL;
		case Gpu_Vk_Texture_Layout_Color_Attachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case Gpu_Vk_Texture_Layout_Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
}

bool gpu_vk_new_image_for_texture_desc(const Gpu_Vk_Texture_Desc* desc, VkImage* out_image) {
	VkImageCreateInfo image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = gpu_vk_texture_kind_to_vk_image_type(desc->kind),
		.usage = gpu_vk_texture_usage_to_vk_image_usage_flags(desc->usage),
		.format = gpu_vk_format_to_vk_format(desc->format),
		.extent = {
			.width = desc->size.x,
			.height = desc->size.y,
			.depth = desc->size.z,
		},
		.mipLevels = desc->mip_levels,
		.arrayLayers = desc->array_layers,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.pQueueFamilyIndices = &gpu_vk.queue_family_indices.index[Gpu_Vk_Queue_Primary],
		.queueFamilyIndexCount = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
	};
	if (vkCreateImage(gpu_vk.device, &image_create_info, NULL, out_image) == VK_SUCCESS) {
		return true;
	} else {
		return false;
	}
}

Gpu_Vk_Size_And_Align gpu_vk_size_and_align_for_texture(const Gpu_Vk_Texture_Desc* desc) {
	VkImage image;
	if (!gpu_vk_new_image_for_texture_desc(desc, &image)) {
		return (Gpu_Vk_Size_And_Align) {0};
	}

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(gpu_vk.device, image, &mem_reqs);

	vkDestroyImage(gpu_vk.device, image, NULL);

	return (Gpu_Vk_Size_And_Align) {
		.size = mem_reqs.size,

		// On older hardware tiled textures and buffers can't overlap the same page(s).
		// So we bump the alignment to prevent overlap.
		.align = sl_max(mem_reqs.alignment, gpu_vk.device_limits.bufferImageGranularity),
	};
}

Gpu_Vk_Texture gpu_vk_new_texture(const Gpu_Vk_Texture_Desc* desc, Gpu_Vk_Slice slice) {
	Gpu_Vk_Texture texture = gpu_vk_texture_pool_acquire(&gpu_vk.texture_pool);
	Gpu_Vk_Texture_Data* texture_data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, texture);
	texture_data->data_kind = Gpu_Vk_Texture_Data_Kind_Immediate;
	texture_data->imm.layout = Gpu_Vk_Texture_Layout_Undefined;
	texture_data->imm.size = desc->size;
	texture_data->imm.owned_image = true;

	if (!gpu_vk_new_image_for_texture_desc(desc, &texture_data->imm.image)) {
		gpu_vk_texture_pool_release(&gpu_vk.texture_pool, texture);
		return SL_HANDLE_NULL;
	}

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(gpu_vk.device, texture_data->imm.image, &mem_reqs);
	gpu_vk_validate(slice.offset % mem_reqs.alignment == 0, "Slice has insufficient alignment for texture.");
	gpu_vk_validate(slice.size >= mem_reqs.size, "Slice has insufficient size for texture.");

	Gpu_Vk_Heap_Data* heap_data = gpu_vk_heap_pool_resolve(&gpu_vk.heap_pool, slice.heap);
	sl_assert(heap_data != NULL, "Invalid slice.");

	VkResult bind_result = vkBindImageMemory(gpu_vk.device, texture_data->imm.image, heap_data->device_memory, slice.offset);
	if (bind_result != VK_SUCCESS) {
		vkDestroyImage(gpu_vk.device, texture_data->imm.image, NULL);
		gpu_vk_texture_pool_release(&gpu_vk.texture_pool, texture);
		return SL_HANDLE_NULL;
	}

	VkImageViewCreateInfo view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = texture_data->imm.image,
		.format = gpu_vk_format_to_vk_format(desc->format),
		.viewType = gpu_vk_texture_kind_to_vk_image_view_type(desc->kind),
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = desc->mip_levels,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = desc->array_layers,
	};

	VkResult create_view_result = vkCreateImageView(gpu_vk.device, &view_create_info, NULL, &texture_data->imm.image_view);
	if (create_view_result != VK_SUCCESS) {
		vkDestroyImage(gpu_vk.device, texture_data->imm.image, NULL);
		gpu_vk_texture_pool_release(&gpu_vk.texture_pool, texture);
		return SL_HANDLE_NULL;
	}

	return texture;
}
vec3_u32 gpu_vk_get_texture_size(Gpu_Vk_Texture texture) {
	Gpu_Vk_Texture root_texture = gpu_vk_texture_get_root(texture);
	Gpu_Vk_Texture_Data* root_data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, root_texture);
	sl_assert(root_data != NULL, "Texture is invalid.");
	sl_assert(root_data->data_kind == Gpu_Vk_Texture_Data_Kind_Immediate, "Root texture must be immediate.");
	return root_data->imm.size;
}
VkFormat gpu_vk_get_texture_format(Gpu_Vk_Texture texture) {
	Gpu_Vk_Texture root_texture = gpu_vk_texture_get_root(texture);
	Gpu_Vk_Texture_Data* root_data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, root_texture);
	sl_assert(root_data != NULL, "Texture is invalid.");
	sl_assert(root_data->data_kind == Gpu_Vk_Texture_Data_Kind_Immediate, "Root texture must be immediate.");
	return root_data->imm.format;
}
void gpu_vk_destroy_texture(Gpu_Vk_Texture texture) {
	Gpu_Vk_Texture_Data* data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, texture);
	sl_assert(data != NULL, "Texture is invalid.");

	// TODO: Assert that it is not in-use

	// destroy all cached framebuffers that retain this texture (can assert that rc == 0).
	// ...

	switch (data->data_kind) {
		case Gpu_Vk_Texture_Data_Kind_Immediate: {
			vkDestroyImageView(gpu_vk.device, data->imm.image_view, NULL);

			if (data->imm.owned_image) {
				vkDestroyImage(gpu_vk.device, data->imm.image, NULL);
			}
		} break;
	}

	gpu_vk_texture_pool_release(&gpu_vk.texture_pool, texture);
}

VkAttachmentLoadOp gpu_vk_load_op_to_vk(Gpu_Vk_Load_Op op) {
	switch (op) {
		case Gpu_Vk_Load_Op_Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
		case Gpu_Vk_Load_Op_Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case Gpu_Vk_Load_Op_Dont_Care: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
}
VkAttachmentStoreOp gpu_vk_store_op_to_vk(Gpu_Vk_Store_Op op) {
	switch (op) {
		case Gpu_Vk_Store_Op_Store: return VK_ATTACHMENT_STORE_OP_STORE;
		case Gpu_Vk_Store_Op_Dont_Care: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
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
	sl_assert(key.attachment_count > 0, "Framebuffer must have at least one attachment.");

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

		// Create render pass
		{
			VkAttachmentDescription attachments[GPU_VK_MAX_ATTACHMENTS];
			VkAttachmentReference attachment_refs[GPU_VK_MAX_ATTACHMENTS];
			for (u8 attachment_idx = 0; attachment_idx < key.attachment_count; attachment_idx++) {
				const Gpu_Vk_Framebuffer_Key_Attachment* key_att = &key.attachments[attachment_idx];
				const Gpu_Vk_Texture_Data* texture_data = gpu_vk_texture_get_root_data(key_att->texture);
				const VkImageLayout image_layout = gpu_vk_texture_layout_to_vk_image_layout(texture_data->imm.layout);
				attachments[attachment_idx] = (VkAttachmentDescription) {
					.format = gpu_vk_get_texture_format(key_att->texture),
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = gpu_vk_load_op_to_vk(key_att->load_op),
					.storeOp = gpu_vk_store_op_to_vk(key_att->store_op),
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = image_layout,
					.finalLayout = image_layout
				};
				attachment_refs[attachment_idx] = (VkAttachmentReference) {
					.attachment = attachment_idx,
					.layout = image_layout,
				};
			}

			VkSubpassDescription subpass = {
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = key.attachment_count,
				.pColorAttachments = attachment_refs,
			};

			VkRenderPassCreateInfo render_pass_create_info = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.attachmentCount = key.attachment_count,
				.pAttachments = attachments,
				.subpassCount = 1,
				.pSubpasses = &subpass,
			};

			VkResult create_render_pass_result = vkCreateRenderPass(gpu_vk.device, &render_pass_create_info, NULL, &data->render_pass);
			sl_assert(create_render_pass_result == VK_SUCCESS, "Failed to create render pass.");
		}

		VkImageView attachments[GPU_VK_MAX_ATTACHMENTS];
		for (u8 attachment_idx = 0; attachment_idx < key.attachment_count; attachment_idx++) {
			attachments[attachment_idx] = gpu_vk_texture_get_image_view(key.attachments[attachment_idx].texture);
		}

		const vec3_u32 size = gpu_vk_get_texture_size(key.attachments[0].texture);

		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = data->render_pass,
			.attachmentCount = key.attachment_count,
			.pAttachments = attachments,
			.width = size.x,
			.height = size.y,
			.layers = 1,
		};

		VkResult create_result = vkCreateFramebuffer(gpu_vk.device, &framebuffer_info, NULL, &data->framebuffer);
		sl_assert(create_result == VK_SUCCESS, "Failed to create framebuffer.");

		gpu_vk_framebuffer_map_insert(&gpu_vk.framebuffer_map, key, result);

		return result;
	}
}

// Swapchain
Gpu_Vk_Swapchain gpu_vk_new_swapchain(const Gpu_Vk_Swapchain_Init_Desc* init_desc) {
	Gpu_Vk_Swapchain swapchain = gpu_vk_swapchain_pool_acquire(&gpu_vk.swapchain_pool);
	Gpu_Vk_Swapchain_Data* swapchain_data = gpu_vk_swapchain_pool_resolve(&gpu_vk.swapchain_pool, swapchain);

	// Create surface
	swapchain_data->surface = init_desc->get_surface_fn(init_desc->ctx, gpu_vk.instance);

	return swapchain;
}
void gpu_vk_destroy_swapchain(Gpu_Vk_Swapchain swapchain) {
	// todo
}

void gpu_vk_retain_swapchain_instance(Gpu_Vk_Swapchain_Instance* instance) {
	const u32 pre = atomic_fetch_add_explicit(&instance->rc, 1, memory_order_acq_rel);
	gpu_vk_validate(pre > 0, "Can't retain a released swapchain.");
}
void gpu_vk_release_swapchain_instance(Gpu_Vk_Swapchain_Instance* instance) {
	const u32 pre = atomic_fetch_sub_explicit(&instance->rc, 1, memory_order_acq_rel);
	gpu_vk_validate(pre > 0, "Over-released");

	if (pre == 1) {
		gpu_vk_log("Destroying swapchain instance %p\n", instance);
		for (u32 texture_idx = 0; texture_idx < instance->texture_count; texture_idx++) {
			gpu_vk_destroy_texture(instance->textures[texture_idx]);
			vkDestroySemaphore(gpu_vk.device, instance->present_semaphores[texture_idx], NULL);
		}
		vkDestroySwapchainKHR(gpu_vk.device, instance->swapchain, NULL);
		allocator_free(gpu_vk.allocator, instance->textures, instance->texture_count);
		allocator_free(gpu_vk.allocator, instance->present_semaphores, instance->texture_count);
		allocator_free(gpu_vk.allocator, instance, 1);
	}
}

Gpu_Vk_Swapchain_Instance* gpu_vk_get_instance(Gpu_Vk_Swapchain swapchain, Gpu_Vk_Swapchain_Desc desc) {
	Gpu_Vk_Swapchain_Data* swapchain_data = gpu_vk_swapchain_pool_resolve(&gpu_vk.swapchain_pool, swapchain);

	VkSurfaceCapabilitiesKHR capabilities;
	VkResult get_capabilities_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu_vk.physical_device, swapchain_data->surface, &capabilities);
	sl_assert(get_capabilities_result == VK_SUCCESS, "Failed to get swapchain capabilities.");

	// use the surface's size, if it has one.
	desc.size = (vec2_u32) { capabilities.currentExtent.width == u32_max ? desc.size.x : capabilities.currentExtent.width, capabilities.currentExtent.height == u32_max ? desc.size.y : capabilities.currentExtent.height };

	Gpu_Vk_Swapchain_Instance* current_instance = swapchain_data->current_instance;

	const bool must_rebuild = (current_instance == NULL) || (current_instance->desc.size.x != desc.size.x) || (current_instance->desc.size.y != desc.size.y) || (current_instance->desc.colorspace != desc.colorspace);
	if (!must_rebuild) {
		return current_instance;
	}

	Gpu_Vk_Swapchain_Instance* new_instance;
	allocator_new(gpu_vk.allocator, new_instance, 1);
	*new_instance = (Gpu_Vk_Swapchain_Instance) {
		.rc = 1,
		.desc = desc,
	};

	const VkColorSpaceKHR colorspace = gpu_vk_colorspace_to_vk_colorspace(desc.colorspace);

	u32 available_present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu_vk.physical_device, swapchain_data->surface, &available_present_mode_count, NULL);

	VkPresentModeKHR* available_present_modes;
	allocator_new(gpu_vk.allocator, available_present_modes, available_present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu_vk.physical_device, swapchain_data->surface, &available_present_mode_count, available_present_modes);

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // default
	for (u32 available_present_mode_idx = 0; available_present_mode_idx < available_present_mode_count; available_present_mode_idx++) {
		if (available_present_modes[available_present_mode_idx] == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	allocator_free(gpu_vk.allocator, available_present_modes, available_present_mode_count);

	const u32 min_image_count = sl_clamp(3, capabilities.minImageCount, (capabilities.maxImageCount == 0) ? u32_max : capabilities.maxImageCount);

	gpu_vk_log("Rebuilding swapchain with extent (%u, %u), %u min images, instance %p.", desc.size.x, desc.size.y, min_image_count, new_instance);

	const VkExtent2D extent = {
		.width = desc.size.x,
		.height = desc.size.y,
	};

	VkFormat vk_format = gpu_vk_format_to_vk_format(desc.format);

	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = swapchain_data->surface,
		.imageFormat = vk_format,
		.imageColorSpace = colorspace,
		.imageExtent = extent,
		.minImageCount = min_image_count,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageArrayLayers = 1,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = current_instance ? current_instance->swapchain : NULL,
		.presentMode = present_mode,
	};

	Gpu_Vk_Queue_Family_Indices queue_indices = gpu_vk_get_physical_device_queue_families(gpu_vk.physical_device, gpu_vk.allocator);
	if (queue_indices.index[Gpu_Vk_Queue_Primary] == queue_indices.index[Gpu_Vk_Queue_Present]) {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 3;
		create_info.pQueueFamilyIndices = queue_indices.index;
	}

	VkResult create_result = vkCreateSwapchainKHR(gpu_vk.device, &create_info, NULL, &new_instance->swapchain);
	sl_assert(create_result == VK_SUCCESS, "Failed to create swapchain.");

	u32 swapchain_image_count;
	vkGetSwapchainImagesKHR(gpu_vk.device, new_instance->swapchain, &swapchain_image_count, NULL);

	new_instance->texture_count = swapchain_image_count;
	allocator_new(gpu_vk.allocator, new_instance->textures, swapchain_image_count);
	allocator_new(gpu_vk.allocator, new_instance->present_semaphores, swapchain_image_count);

	// Get images
	VkImage* images;
	allocator_new(gpu_vk.allocator, images, swapchain_image_count);
	vkGetSwapchainImagesKHR(gpu_vk.device, new_instance->swapchain, &swapchain_image_count, images);

	// Create image views
	for (u32 image_idx = 0; image_idx < swapchain_image_count; image_idx++) {
		const VkImageViewCreateInfo view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = images[image_idx],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = vk_format,
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

		Gpu_Vk_Texture texture = gpu_vk_texture_pool_acquire(&gpu_vk.texture_pool);
		Gpu_Vk_Texture_Data* texture_data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, texture);
		texture_data->data_kind = Gpu_Vk_Texture_Data_Kind_Immediate;
		texture_data->imm.size = (vec3_u32) { extent.width, extent.height, 1 };
		texture_data->imm.format = vk_format;
		texture_data->imm.layout = Gpu_Vk_Texture_Layout_Undefined;
		texture_data->imm.image = images[image_idx];
		texture_data->imm.owned_image = false;
		const VkResult view_create_result = vkCreateImageView(gpu_vk.device, &view_create_info, NULL, &texture_data->imm.image_view);
		sl_assert(view_create_result == VK_SUCCESS, "Failed to create image view for swapchain.");

		new_instance->textures[image_idx] = texture;
	}

	// Create semaphores
	for (u32 image_idx = 0; image_idx < swapchain_image_count; image_idx++) {
		VkSemaphoreCreateInfo semaphore_create_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
		VkResult create_semaphore_result = vkCreateSemaphore(gpu_vk.device, &semaphore_create_info, NULL, &new_instance->present_semaphores[image_idx]);
		sl_assert(create_semaphore_result == VK_SUCCESS, "Failed to create semaphore.");
	}

	allocator_free(gpu_vk.allocator, images, swapchain_image_count);

	swapchain_data->current_instance = new_instance;

	if (current_instance != NULL) {
		gpu_vk_release_swapchain_instance(current_instance);
	}

	return new_instance;
}

void gpu_vk_init_resource_pools() {
	gpu_vk.heap_pool = gpu_vk_heap_pool_new(gpu_vk.allocator, 0);
	gpu_vk.texture_pool = gpu_vk_texture_pool_new(gpu_vk.allocator, 0);
	gpu_vk.command_pool_pool = gpu_vk_command_buffer_pool_pool_new(gpu_vk.allocator, 0);
	gpu_vk.compute_pipeline_pool = gpu_vk_compute_pipeline_pool_new(gpu_vk.allocator, 0);
	gpu_vk.swapchain_pool = gpu_vk_swapchain_pool_new(gpu_vk.allocator, 0);
	gpu_vk.semaphore_pool = gpu_vk_semaphore_pool_new(gpu_vk.allocator, 0);

	gpu_vk.framebuffer_pool = gpu_vk_framebuffer_pool_new(gpu_vk.allocator, 0);
	gpu_vk.framebuffer_map = gpu_vk_framebuffer_map_new(gpu_vk.allocator, 64);
	gpu_vk.framebuffer_lru = (Gpu_Vk_Framebuffer_LRU) {
		.oldest = SL_HANDLE_NULL,
		.newest = SL_HANDLE_NULL,
	};
}

void gpu_vk_init(const Gpu_Vk_Desc* desc) {
	gpu_vk = (Gpu_Vk) {};
	gpu_vk.allocator = desc->allocator;
	gpu_vk.swapchain_init_desc = desc->swapchain_desc;
	gpu_vk_init_instance(desc);
	gpu_vk_init_device();
	gpu_vk_init_resource_pools();
	gpu_vk.global_semaphore = gpu_vk_new_semaphore();
}
void gpu_vk_deinit() {
	gpu_vk_log("Deinit");
	gpu_vk_wait_cpu(gpu_vk.global_semaphore, gpu_vk.global_semaphore_value);
	gpu_vk_destroy_semaphore(gpu_vk.global_semaphore);
	vkDestroyInstance(gpu_vk.instance, NULL);
	gpu_vk = (Gpu_Vk) {};
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

	VkBufferCreateInfo buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = bytes,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.pQueueFamilyIndices = &gpu_vk.queue_family_indices.index[Gpu_Vk_Queue_Primary],
		.queueFamilyIndexCount = 1,
		.flags = 0,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	};
	VkResult create_buffer_result = vkCreateBuffer(gpu_vk.device, &buffer_create_info, NULL, &heap_data->buffer);
	sl_assert(create_buffer_result == VK_SUCCESS, "Failed to create buffer.");

	VkResult bind_buffer_result = vkBindBufferMemory(gpu_vk.device, heap_data->buffer, heap_data->device_memory, 0);
	sl_assert(bind_buffer_result == VK_SUCCESS, "Failed to bind buffer to heap allocation.");

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
Gpu_Vk_Slice gpu_vk_get_heap_slice(Gpu_Vk_Heap heap) {
	return gpu_vk_slice(heap, 0, gpu_vk_get_heap_size(heap));
}

// Slice
void* gpu_vk_get_slice_host_ptr(Gpu_Vk_Slice slice) {
	Gpu_Vk_Heap_Data* heap_data = gpu_vk_heap_pool_resolve(&gpu_vk.heap_pool, slice.heap);
	sl_assert(heap_data->memory_type == Gpu_Vk_Memory_Type_Host_Visible, "Can only map host-visible memory.");
	return heap_data->host_ptr + slice.offset;
}
void gpu_vk_flush_slice_to_gpu(Gpu_Vk_Slice slice) {
	Gpu_Vk_Heap_Data* heap_data = gpu_vk_heap_pool_resolve(&gpu_vk.heap_pool, slice.heap);
	sl_assert(heap_data->memory_type == Gpu_Vk_Memory_Type_Host_Visible, "Flush only makes sense for host-visible memory.");

	const u64 aligned_offset = sl_round_down_u64(slice.offset, gpu_vk.device_limits.nonCoherentAtomSize);
	const u64 aligned_size = sl_round_up_u64(slice.size + (slice.offset - aligned_offset), gpu_vk.device_limits.nonCoherentAtomSize);

	VkMappedMemoryRange memory_range = {
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.offset = aligned_offset,
		.size = aligned_size,
		.memory = heap_data->device_memory
	};

	// To simplify the API, we wait for upload to complete before returning.
	// This is sub-optimal.
	VkResult flush_result = vkFlushMappedMemoryRanges(gpu_vk.device, 1, &memory_range);
	sl_assert(flush_result == VK_SUCCESS, "Failed to flush slice upload.");
}
void gpu_vk_flush_slice_from_gpu(Gpu_Vk_Slice slice) {
	Gpu_Vk_Heap_Data* heap_data = gpu_vk_heap_pool_resolve(&gpu_vk.heap_pool, slice.heap);
	sl_assert(heap_data->memory_type == Gpu_Vk_Memory_Type_Host_Visible, "Flush only makes sense for host-visible memory.");

	const u64 aligned_offset = sl_round_down_u64(slice.offset, gpu_vk.device_limits.nonCoherentAtomSize);
	const u64 aligned_size = sl_round_up_u64(slice.size + (slice.offset - aligned_offset), gpu_vk.device_limits.nonCoherentAtomSize);

	VkMappedMemoryRange memory_range = {
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.offset = aligned_offset,
		.size = aligned_size,
		.memory = heap_data->device_memory
	};

	VkResult invalidate_result = vkInvalidateMappedMemoryRanges(gpu_vk.device, 1, &memory_range);
	sl_assert(invalidate_result == VK_SUCCESS, "Failed to invalidate slice.");
}

// Command Buffer
void gpu_init_command_buffer(Gpu_Vk_Command_Buffer_Data* command_buffer) {
	*command_buffer = (Gpu_Vk_Command_Buffer_Data) {
		.arena = sl_arena_allocator_new(gpu_vk.allocator, 256 << 10),
		.commands = gpu_vk_command_seq_new(gpu_vk.allocator, 8),
		.semaphores = gpu_vk_semaphore_seq_new(gpu_vk.allocator, 1),
		.command_buffers = gpu_vk_command_buffer_seq_new(gpu_vk.allocator, 1),
		.swapchain_presents = gpu_vk_swapchain_present_seq_new(gpu_vk.allocator, 1),
	};

	VkCommandPoolCreateInfo command_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = gpu_vk.queue_family_indices.index[Gpu_Vk_Queue_Primary],
	};
	VkResult command_pool_result = vkCreateCommandPool(gpu_vk.device, &command_pool_create_info, NULL, &command_buffer->command_pool);
	sl_assert(command_pool_result == VK_SUCCESS, "Failed to create command pool.");
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

	// Wait for inflight work associated with the recorder to be completed.
	// Note: it is guaranteed that the on completion callback is processed prior to this function returning.
	gpu_vk_wait_cpu(gpu_vk.global_semaphore, cb_data->gpu_complete_semaphore_value);

	vkResetCommandPool(gpu_vk.device, cb_data->command_pool, 0);

	cb_data->state = Gpu_Vk_Command_Buffer_State_Recording;
	sl_arena_allocator_reset(cb_data->arena, 0);
	cb_data->next_free_semaphore = 0;
	cb_data->next_free_command_buffer = 0;
	gpu_vk_command_seq_clear(&cb_data->commands);
	gpu_vk_swapchain_present_seq_clear(&cb_data->swapchain_presents);

	pool_data->next_command_buffer = (pool_data->next_command_buffer + 1) % pool_data->command_buffer_count;

	*out_cb = cb_handle;
	return true;
}

void gpu_vk_on_complete_command_buffer_callback(void* ctx) {
	Gpu_Vk_Command_Buffer_Data* cb_data = ctx;

	const u32 present_count = gpu_vk_swapchain_present_seq_get_count(&cb_data->swapchain_presents);
	for (u32 present_idx = 0; present_idx < present_count; present_idx++) {
		const Gpu_Vk_Swapchain_Present present_info = gpu_vk_swapchain_present_seq_get(&cb_data->swapchain_presents, present_idx);
		gpu_vk_release_swapchain_instance(present_info.swapchain_instance);
	}
}

VkSemaphore gpu_vk_get_next_command_buffer_semaphore(Gpu_Vk_Command_Buffer cb) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	if (cb_data->next_free_semaphore < gpu_vk_semaphore_seq_get_count(&cb_data->semaphores)) {
		return gpu_vk_semaphore_seq_get(&cb_data->semaphores, cb_data->next_free_semaphore++);
	} else {
		VkSemaphore* semaphore_ptr = gpu_vk_semaphore_seq_push_reserve(&cb_data->semaphores);

		VkSemaphoreCreateInfo semaphore_create_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
		VkResult create_semaphore_result = vkCreateSemaphore(gpu_vk.device, &semaphore_create_info, NULL, semaphore_ptr);
		sl_assert(create_semaphore_result == VK_SUCCESS, "Failed to create semaphore.");

		cb_data->next_free_semaphore++;
		return *semaphore_ptr;
	}
}

VkCommandBuffer gpu_vk_get_next_vk_command_buffer(Gpu_Vk_Command_Buffer_Data* cb_data) {
	if (cb_data->next_free_command_buffer < gpu_vk_command_buffer_seq_get_count(&cb_data->command_buffers)) {
		return gpu_vk_command_buffer_seq_get(&cb_data->command_buffers, cb_data->next_free_command_buffer++);
	} else {
		VkCommandBuffer* command_buffer_ptr = gpu_vk_command_buffer_seq_push_reserve(&cb_data->command_buffers);

		const VkCommandBufferAllocateInfo allocate_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = cb_data->command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		VkResult allocate_result = vkAllocateCommandBuffers(gpu_vk.device, &allocate_info, command_buffer_ptr);
		sl_assert(allocate_result == VK_SUCCESS, "Failed to create command buffer.");

		cb_data->next_free_command_buffer++;
		return *command_buffer_ptr;
	}
}

sl_inline VkDescriptorType gpu_vk_binding_kind_to_vk_descriptor_type(Gpu_Vk_Binding_Kind binding_kind) {
	switch (binding_kind) {
		case Gpu_Vk_Binding_Kind_Storage_Texture: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case Gpu_Vk_Binding_Kind_Sampled_Texture: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case Gpu_Vk_Binding_Kind_Slice: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	}
}

sl_inline void gpu_vk_execute_transition_textures_to_layout(SL_Arena_Allocator* arena, VkCommandBuffer vk_cb, const Gpu_Vk_Texture* textures, const Gpu_Vk_Texture_Layout* layouts, u32 count) {
	if (count == 0) {
		return;
	}
	const u64 reset_position = sl_arena_allocator_get_position(arena);

	u32 next_barrier = 0;
	VkImageMemoryBarrier2* barriers;
	allocator_new(&arena->allocator, barriers, count);

	for (u32 texture_idx = 0; texture_idx < count; texture_idx++) {
		Gpu_Vk_Texture_Data* texture_data = gpu_vk_texture_get_root_data(textures[texture_idx]);
		gpu_vk_validate(texture_data != NULL, "Invalid texture.");

		Gpu_Vk_Texture_Layout old_layout = texture_data->imm.layout;
		Gpu_Vk_Texture_Layout new_layout = layouts[texture_idx];
		if (old_layout == new_layout) {
			continue;
		}
		texture_data->imm.layout = new_layout;

		barriers[next_barrier++] = (VkImageMemoryBarrier2) {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,

			.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,

			.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,

			.oldLayout = gpu_vk_texture_layout_to_vk_image_layout(old_layout),
			.newLayout = gpu_vk_texture_layout_to_vk_image_layout(new_layout),

			.image = texture_data->imm.image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount = VK_REMAINING_ARRAY_LAYERS,
			},
		};
	}

	if (next_barrier > 0) {
		VkDependencyInfo dep = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = next_barrier,
			.pImageMemoryBarriers = barriers,
		};

		gpu_vk.device_function_table.vkCmdPipelineBarrier2KHR(vk_cb, &dep);
	}

	sl_arena_allocator_reset(arena, reset_position);
}

sl_inline void gpu_vk_write_bindings(SL_Arena_Allocator* arena, VkCommandBuffer vk_cb, VkPipelineLayout pipeline_layout, const Gpu_Vk_Binding* bindings, u32 binding_count) {
	if (binding_count == 0) {
		return;
	}
	const u64 reset_position = sl_arena_allocator_get_position(arena);

	VkWriteDescriptorSet* writes;
	allocator_new(&arena->allocator, writes, binding_count);

	for (u32 binding_idx = 0; binding_idx < binding_count; binding_idx++) {
		VkWriteDescriptorSet* write = &writes[binding_idx];
		Gpu_Vk_Binding binding = bindings[binding_idx];
		switch (binding.kind) {
			case Gpu_Vk_Binding_Kind_Storage_Texture: {
				VkDescriptorImageInfo* image_info;
				allocator_new(&arena->allocator, image_info, 1);
				*image_info = (VkDescriptorImageInfo) {
					.imageLayout = VK_IMAGE_LAYOUT_GENERAL, // todo
					.imageView = gpu_vk_texture_get_image_view(binding.storage_texture.texture),
					.sampler = VK_NULL_HANDLE,
				};

				*write = (VkWriteDescriptorSet) {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = VK_NULL_HANDLE,
					.dstBinding = binding.index,
					.descriptorType = gpu_vk_binding_kind_to_vk_descriptor_type(binding.kind),
					.pImageInfo = image_info,
					.dstArrayElement = 0,
					.descriptorCount = 1,
				};
			} break;

			case Gpu_Vk_Binding_Kind_Sampled_Texture: {
				// todo
			} break;

			case Gpu_Vk_Binding_Kind_Slice: {
				Gpu_Vk_Heap_Data* heap_data = gpu_vk_heap_pool_resolve(&gpu_vk.heap_pool, binding.slice.heap);

				VkDescriptorBufferInfo* buffer_info;
				allocator_new(&arena->allocator, buffer_info, 1);
				*buffer_info = (VkDescriptorBufferInfo) {
					.buffer = heap_data->buffer,
					.offset = binding.slice.offset,
					.range = binding.slice.size,
				};

				*write = (VkWriteDescriptorSet) {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = VK_NULL_HANDLE,
					.dstBinding = binding.index,
					.descriptorType = gpu_vk_binding_kind_to_vk_descriptor_type(binding.kind),
					.pBufferInfo = buffer_info,
					.dstArrayElement = 0,
					.descriptorCount = 1,
				};
			} break;
		}
	}

	gpu_vk.device_function_table.vkCmdPushDescriptorSetKHR(vk_cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, binding_count, writes);

	sl_arena_allocator_reset(arena, reset_position);
}

typedef struct Gpu_Vk_Command_Buffer_Emitter_Semaphore {
	VkSemaphore semaphore;
	u64 value;
} Gpu_Vk_Command_Buffer_Emitter_Semaphore;
sl_seq(Gpu_Vk_Command_Buffer_Emitter_Semaphore, Gpu_Vk_Command_Buffer_Emitter_Semaphore_Seq, gpu_vk_command_buffer_emitter_semaphore_seq);

typedef struct Gpu_Vk_Command_Buffer_Emitter {
	Gpu_Vk_Command_Buffer_Data* cb_data;
	Gpu_Vk_Command_Buffer_Emitter_Semaphore_Seq wait;
	Gpu_Vk_Command_Buffer_Emitter_Semaphore_Seq signal;
	VkCommandBuffer cb;
} Gpu_Vk_Command_Buffer_Emitter;

void gpu_vk_init_command_buffer_emitter(Gpu_Vk_Command_Buffer_Data* cb_data, Gpu_Vk_Command_Buffer_Emitter* emitter) {
	*emitter = (Gpu_Vk_Command_Buffer_Emitter) {
		.cb_data = cb_data,
		.wait = gpu_vk_command_buffer_emitter_semaphore_seq_new(&cb_data->arena->allocator, 0),
		.signal = gpu_vk_command_buffer_emitter_semaphore_seq_new(&cb_data->arena->allocator, 0),
		.cb = VK_NULL_HANDLE,
	};
}
void gpu_vk_flush_command_buffer_emitter(Gpu_Vk_Command_Buffer_Emitter* emitter) {
	const u32 wait_count = gpu_vk_command_buffer_emitter_semaphore_seq_get_count(&emitter->wait);
	const u32 signal_count = gpu_vk_command_buffer_emitter_semaphore_seq_get_count(&emitter->signal);

	if ((emitter->cb == VK_NULL_HANDLE) && (wait_count == 0) && (signal_count == 0)) {
		return;
	}

	SL_Arena_Allocator* arena = emitter->cb_data->arena;
	const u64 arena_reset_position = sl_arena_allocator_get_position(arena);

	VkCommandBuffer cb;
	if (emitter->cb == VK_NULL_HANDLE) {
		cb = gpu_vk_get_next_vk_command_buffer(emitter->cb_data);
		const VkCommandBufferBeginInfo vk_cb_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		vkBeginCommandBuffer(cb, &vk_cb_begin_info);
	} else {
		cb = emitter->cb;
		emitter->cb = VK_NULL_HANDLE;
	}

	vkEndCommandBuffer(cb);

	VkSemaphore* wait_semaphores;
	VkPipelineStageFlags* wait_stage_flags;
	u64* wait_values;
	VkSemaphore* signal_semaphores;
	u64* signal_values;
	allocator_new(&arena->allocator, wait_values, wait_count);
	allocator_new(&arena->allocator, wait_semaphores, wait_count);
	allocator_new(&arena->allocator, wait_stage_flags, wait_count);
	allocator_new(&arena->allocator, signal_semaphores, signal_count);
	allocator_new(&arena->allocator, signal_values, signal_count);

	for (u32 wait_idx = 0; wait_idx < wait_count; wait_idx++) {
		const Gpu_Vk_Command_Buffer_Emitter_Semaphore* semaphore = gpu_vk_command_buffer_emitter_semaphore_seq_get_ptr(&emitter->wait, wait_idx);
		wait_semaphores[wait_idx] = semaphore->semaphore;
		wait_values[wait_idx] = semaphore->value;
		wait_stage_flags[wait_idx] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}

	for (u32 signal_idx = 0; signal_idx < signal_count; signal_idx++) {
		const Gpu_Vk_Command_Buffer_Emitter_Semaphore* semaphore = gpu_vk_command_buffer_emitter_semaphore_seq_get_ptr(&emitter->signal, signal_idx);
		signal_semaphores[signal_idx] = semaphore->semaphore;
		signal_values[signal_idx] = semaphore->value;
	}

	const VkTimelineSemaphoreSubmitInfo timeline_info = {
		.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
		.pWaitSemaphoreValues = wait_values,
		.waitSemaphoreValueCount = wait_count,
		.signalSemaphoreValueCount = signal_count,
		.pSignalSemaphoreValues = signal_values
	};

	const VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = &timeline_info,
		.pCommandBuffers = &cb,
		.commandBufferCount = 1,
		.waitSemaphoreCount = wait_count,
		.pWaitSemaphores = wait_semaphores,
		.pWaitDstStageMask = wait_stage_flags,
		.signalSemaphoreCount = signal_count,
		.pSignalSemaphores = signal_semaphores,
	};
	const VkResult submit_result = vkQueueSubmit(gpu_vk.queue[Gpu_Vk_Queue_Primary], 1, &submit_info, VK_NULL_HANDLE);
	sl_assert(submit_result == VK_SUCCESS, "Failed to submit command buffer.");

	gpu_vk_command_buffer_emitter_semaphore_seq_clear(&emitter->wait);
	gpu_vk_command_buffer_emitter_semaphore_seq_clear(&emitter->signal);

	sl_arena_allocator_reset(arena, arena_reset_position);
}
VkCommandBuffer gpu_vk_fetch_command_buffer_emitter(Gpu_Vk_Command_Buffer_Emitter* emitter) {
	const u32 signal_count = gpu_vk_command_buffer_emitter_semaphore_seq_get_count(&emitter->signal);
	if ((emitter->cb != VK_NULL_HANDLE) && (signal_count > 0)) {
		gpu_vk_flush_command_buffer_emitter(emitter);
	}

	if (emitter->cb == VK_NULL_HANDLE) {
		emitter->cb = gpu_vk_get_next_vk_command_buffer(emitter->cb_data);
		const VkCommandBufferBeginInfo vk_cb_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		vkBeginCommandBuffer(emitter->cb, &vk_cb_begin_info);
	}

	return emitter->cb;
}
void gpu_vk_add_wait_to_command_buffer_emitter(Gpu_Vk_Command_Buffer_Emitter* emitter, VkSemaphore semaphore, u64 value) {
	if (emitter->cb != VK_NULL_HANDLE) {
		// Flush, so all prior commands can execute before waiting on semaphore
		gpu_vk_flush_command_buffer_emitter(emitter);
	}

	Gpu_Vk_Command_Buffer_Emitter_Semaphore entry = {
		.semaphore = semaphore,
		.value = value,
	};
	gpu_vk_command_buffer_emitter_semaphore_seq_push(&emitter->wait, entry);
}
void gpu_vk_add_signal_to_command_buffer_emitter(Gpu_Vk_Command_Buffer_Emitter* emitter, VkSemaphore semaphore, u64 value) {
	Gpu_Vk_Command_Buffer_Emitter_Semaphore entry = {
		.semaphore = semaphore,
		.value = value,
	};
	gpu_vk_command_buffer_emitter_semaphore_seq_push(&emitter->signal, entry);
}

void gpu_vk_enqueue(Gpu_Vk_Command_Buffer cb, bool wait_until_completed) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");
	cb_data->state = Gpu_Vk_Command_Buffer_State_Enqueued;

	Gpu_Vk_Command_Buffer_Emitter cb_emitter;
	gpu_vk_init_command_buffer_emitter(cb_data, &cb_emitter);

	// Wait on global semaphore
	{
		Gpu_Vk_Semaphore_Data* global_semaphore_data = gpu_vk_semaphore_pool_resolve(&gpu_vk.semaphore_pool, gpu_vk.global_semaphore);
		gpu_vk_add_signal_to_command_buffer_emitter(&cb_emitter, global_semaphore_data->vk_semaphore, gpu_vk.global_semaphore_value);
	}

	const u64 command_count = gpu_vk_command_seq_get_count(&cb_data->commands);
	for (u64 command_idx = 0; command_idx < command_count; command_idx++) {
		const Gpu_Vk_Command command = gpu_vk_command_seq_get(&cb_data->commands, command_idx);

		switch (command.kind) {
			case Gpu_Vk_Command_Kind_Transition_Texture_Layouts: {
				VkCommandBuffer vk_cb = gpu_vk_fetch_command_buffer_emitter(&cb_emitter);

				Gpu_Vk_Command_Transition_Texture_Layouts* transition_texture_layouts = command.data.transition_texture_layouts;
				gpu_vk_execute_transition_textures_to_layout(cb_data->arena, vk_cb, transition_texture_layouts->textures, transition_texture_layouts->layouts, transition_texture_layouts->count);
			} break;

			case Gpu_Vk_Command_Kind_Begin_Render: {
				VkCommandBuffer vk_cb = gpu_vk_fetch_command_buffer_emitter(&cb_emitter);

				Gpu_Vk_Command_Begin_Render* begin_render = command.data.begin_render;
				sl_assert(begin_render->render_pass.attachment_count > 0, "Must have at least one attachment in render pass.");

				// Acquire cached framebuffer
				Gpu_Vk_Framebuffer_Key fb_key = {
					.attachment_count = begin_render->render_pass.attachment_count,
				};
				for (u8 attachment_idx = 0; attachment_idx < begin_render->render_pass.attachment_count; attachment_idx++) {
					fb_key.attachments[attachment_idx] = (Gpu_Vk_Framebuffer_Key_Attachment) {
						.texture = gpu_vk_texture_get_root(begin_render->render_pass.attachments[attachment_idx].texture),
						.load_op = begin_render->render_pass.attachments[attachment_idx].load_op,
						.store_op = begin_render->render_pass.attachments[attachment_idx].store_op,
					};
				}
				Gpu_Vk_Framebuffer fb = gpu_vk_acquire_framebuffer(fb_key);
				Gpu_Vk_Framebuffer_Data* fb_data = gpu_vk_framebuffer_pool_resolve(&gpu_vk.framebuffer_pool, fb);

				// Render pass
				VkClearValue clear_values[GPU_VK_MAX_ATTACHMENTS];
				for (u8 attachment_idx = 0; attachment_idx < begin_render->render_pass.attachment_count; attachment_idx++) {
					const Gpu_Vk_Render_Attachment* att = &begin_render->render_pass.attachments[attachment_idx];
					clear_values[attachment_idx].color = (VkClearColorValue) {
						.float32 = {
							att->clear_value.x,
							att->clear_value.y,
							att->clear_value.z,
							att->clear_value.w
						},
					};
				}

				const vec3_u32 render_size = gpu_vk_get_texture_size(begin_render->render_pass.attachments[0].texture);

				VkRenderPassBeginInfo render_pass_begin_info = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = fb_data->render_pass,
					.framebuffer = fb_data->framebuffer,
					.renderArea = {
						.offset = { 0, 0 },
						.extent = { render_size.x, render_size.y },
					},
					.clearValueCount = sl_array_count(clear_values),
					.pClearValues = clear_values,
				};
				vkCmdBeginRenderPass(vk_cb, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			} break;

			case Gpu_Vk_Command_Kind_End_Render: {
				VkCommandBuffer vk_cb = gpu_vk_fetch_command_buffer_emitter(&cb_emitter);

				vkCmdEndRenderPass(vk_cb);
			} break;

			case Gpu_Vk_Command_Kind_Dispatch: {
				VkCommandBuffer vk_cb = gpu_vk_fetch_command_buffer_emitter(&cb_emitter);

				Gpu_Vk_Command_Dispatch* dispatch = command.data.dispatch;
				Gpu_Vk_Compute_Pipeline_Data* pipeline_data = gpu_vk_compute_pipeline_pool_resolve(&gpu_vk.compute_pipeline_pool, dispatch->pipeline);
				gpu_vk_write_bindings(cb_data->arena, vk_cb, pipeline_data->pipeline_layout, dispatch->bindings, dispatch->binding_count);
				vkCmdBindPipeline(vk_cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_data->pipeline);
				vkCmdDispatch(vk_cb, dispatch->group_count.x, dispatch->group_count.y, dispatch->group_count.z);
			} break;

			case Gpu_Vk_Command_Kind_Blit: {
				VkCommandBuffer vk_cb = gpu_vk_fetch_command_buffer_emitter(&cb_emitter);

				Gpu_Vk_Command_Blit* blit = command.data.blit;
				const Gpu_Vk_Blit_Desc* blit_desc = &blit->desc;
				Gpu_Vk_Texture_Data* src_data = gpu_vk_texture_get_root_data(blit_desc->src);
				Gpu_Vk_Texture_Data* dst_data = gpu_vk_texture_get_root_data(blit_desc->dst);
				VkImageBlit region = {
					.srcSubresource = {
						.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel       = blit_desc->src_mip_level,
						.baseArrayLayer = blit_desc->src_array_layer,
						.layerCount     = 1,
					},
					.srcOffsets = {
						{ blit_desc->src_start.x, blit_desc->src_start.y, blit_desc->src_start.z },
						{ blit_desc->src_end.x, blit_desc->src_end.y, blit_desc->src_end.z },
					},
					.dstSubresource = {
						.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel       = blit_desc->dst_mip_level,
						.baseArrayLayer = blit_desc->dst_array_layer,
						.layerCount     = 1,
					},
					.dstOffsets = {
					  { blit_desc->dst_start.x, blit_desc->dst_start.y, blit_desc->dst_start.z },
					  { blit_desc->dst_end.x, blit_desc->dst_end.y, blit_desc->dst_end.z },
					},
				};
				vkCmdBlitImage(vk_cb, src_data->imm.image, gpu_vk_texture_layout_to_vk_image_layout(src_data->imm.layout), dst_data->imm.image, gpu_vk_texture_layout_to_vk_image_layout(dst_data->imm.layout), 1, &region, VK_FILTER_NEAREST);
			} break;

			case Gpu_Vk_Command_Kind_Barrier: {
				VkCommandBuffer vk_cb = gpu_vk_fetch_command_buffer_emitter(&cb_emitter);

				VkMemoryBarrier2 barrier = {
					.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
					.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
					.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
					.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
				};

				VkDependencyInfo dep = {
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.memoryBarrierCount = 1,
					.pMemoryBarriers = &barrier,
				};

				gpu_vk.device_function_table.vkCmdPipelineBarrier2KHR(vk_cb, &dep);
			} break;

			case Gpu_Vk_Command_Kind_Wait: {
				Gpu_Vk_Command_Wait* wait = command.data.wait;
				gpu_vk_add_wait_to_command_buffer_emitter(&cb_emitter, wait->semaphore, wait->value);
			} break;

			case Gpu_Vk_Command_Kind_Signal: {
				Gpu_Vk_Command_Signal* signal = command.data.signal;
				gpu_vk_add_signal_to_command_buffer_emitter(&cb_emitter, signal->semaphore, signal->value);
			} break;
		}
	}

	const u32 present_count = gpu_vk_swapchain_present_seq_get_count(&cb_data->swapchain_presents);

	// Transition all swapchain images for present
	if (present_count > 0) {
		VkCommandBuffer vk_cb = gpu_vk_fetch_command_buffer_emitter(&cb_emitter);
		for (u32 present_idx = 0; present_idx < present_count; present_idx++) {
			const Gpu_Vk_Swapchain_Present present_info = gpu_vk_swapchain_present_seq_get(&cb_data->swapchain_presents, present_idx);
			Gpu_Vk_Texture_Layout present_layout = Gpu_Vk_Texture_Layout_Present;
			gpu_vk_execute_transition_textures_to_layout(cb_data->arena, vk_cb, &present_info.texture, &present_layout, 1);
		}
	}

	// Signal all swapchain semaphores
	for (u32 present_idx = 0; present_idx < present_count; present_idx++) {
		const Gpu_Vk_Swapchain_Present present_info = gpu_vk_swapchain_present_seq_get(&cb_data->swapchain_presents, present_idx);
		gpu_vk_add_signal_to_command_buffer_emitter(&cb_emitter, present_info.present_semaphore, 0);
	}

	// Signal global semaphore
	{
		Gpu_Vk_Semaphore_Data* global_semaphore_data = gpu_vk_semaphore_pool_resolve(&gpu_vk.semaphore_pool, gpu_vk.global_semaphore);

		const u64 signal_value = ++gpu_vk.global_semaphore_value;
		gpu_vk_add_signal_to_command_buffer_emitter(&cb_emitter, global_semaphore_data->vk_semaphore, signal_value);
		cb_data->gpu_complete_semaphore_value = signal_value;
	}

	gpu_vk_flush_command_buffer_emitter(&cb_emitter);

	// Presents
	if (present_count > 0) {
		VkSwapchainKHR* swapchains;
		VkSemaphore* wait_semaphores;
		u32* image_indices;
		allocator_new(&cb_data->arena->allocator, swapchains, present_count);
		allocator_new(&cb_data->arena->allocator, wait_semaphores, present_count);
		allocator_new(&cb_data->arena->allocator, image_indices, present_count);

		for (u32 present_idx = 0; present_idx < present_count; present_idx++) {
			const Gpu_Vk_Swapchain_Present present_info = gpu_vk_swapchain_present_seq_get(&cb_data->swapchain_presents, present_idx);
			swapchains[present_idx] = present_info.swapchain_instance->swapchain;
			wait_semaphores[present_idx] = present_info.present_semaphore;
			image_indices[present_idx] = present_info.image_index;
		}

		VkPresentInfoKHR present_info = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = sl_array_count(wait_semaphores),
			.pWaitSemaphores = wait_semaphores,
			.pSwapchains = swapchains,
			.swapchainCount = present_count,
			.pImageIndices = image_indices,
		};

		VkResult present_result = vkQueuePresentKHR(gpu_vk.queue[Gpu_Vk_Queue_Present], &present_info);
		sl_assert((present_result == VK_SUCCESS) || (present_result == VK_ERROR_OUT_OF_DATE_KHR) || (present_result == VK_SUBOPTIMAL_KHR), "Failed to present command buffer.");
	}

	// Clean up notification
	gpu_vk_notify(gpu_vk.global_semaphore, cb_data->gpu_complete_semaphore_value, cb_data, gpu_vk_on_complete_command_buffer_callback);

	if (wait_until_completed) {
		gpu_vk_wait_cpu(gpu_vk.global_semaphore, cb_data->gpu_complete_semaphore_value);
	}
}

void gpu_vk_transition_texture_layouts(Gpu_Vk_Command_Buffer cb, const Gpu_Vk_Texture* textures, const Gpu_Vk_Texture_Layout* layouts, u32 count) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Vk_Texture* textures_copy;
	Gpu_Vk_Texture_Layout* layouts_copy;
	allocator_new(&cb_data->arena->allocator, textures_copy, count);
	allocator_new(&cb_data->arena->allocator, layouts_copy, count);
	memcpy(textures_copy, textures, sizeof(Gpu_Vk_Texture) * count);
	memcpy(layouts_copy, layouts, sizeof(Gpu_Vk_Texture_Layout) * count);

	Gpu_Vk_Command_Transition_Texture_Layouts* transition_texture_layouts;
	allocator_new(&cb_data->arena->allocator, transition_texture_layouts, 1);
	*transition_texture_layouts = (Gpu_Vk_Command_Transition_Texture_Layouts) {
		.textures = textures_copy,
		.layouts = layouts_copy,
		.count = count,
	};

	gpu_vk_command_seq_push(&cb_data->commands, (Gpu_Vk_Command) {
		.kind = Gpu_Vk_Command_Kind_Transition_Texture_Layouts,
		.data = {
			.transition_texture_layouts = transition_texture_layouts,
		},
	});
}

bool gpu_vk_fetch_swapchain_texture(Gpu_Vk_Swapchain swapchain, Gpu_Vk_Command_Buffer cb, Gpu_Vk_Swapchain_Desc swapchain_desc, u64 timeout, Gpu_Vk_Texture* out_texture) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	VkSemaphore semaphore = gpu_vk_get_next_command_buffer_semaphore(cb);

	u32 image_index;
	Gpu_Vk_Swapchain_Instance* swapchain_instance;
	while (true) {
		swapchain_instance = gpu_vk_get_instance(swapchain, swapchain_desc);
		VkResult acquire_image_result = vkAcquireNextImageKHR(gpu_vk.device, swapchain_instance->swapchain, timeout, semaphore, VK_NULL_HANDLE, &image_index);

		if (acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
			continue;
		} else if (acquire_image_result == VK_NOT_READY) {
			// Return unused semaphore.
			cb_data->next_free_semaphore--;
			return false;
		} else if (acquire_image_result == VK_SUCCESS) {
			break;
		}
	}

	// Retain swapchain until command buffer completes.
	gpu_vk_retain_swapchain_instance(swapchain_instance);

	Gpu_Vk_Texture swapchain_texture = swapchain_instance->textures[image_index];
	Gpu_Vk_Texture_Data* swapchain_texture_data = gpu_vk_texture_pool_resolve(&gpu_vk.texture_pool, swapchain_texture);

	// Always discard the original contents of swapchain texture.
	swapchain_texture_data->imm.layout = Gpu_Vk_Texture_Layout_Undefined;

	// Wait for swapchain image to be available.
	{
		Gpu_Vk_Command_Wait* wait;
		allocator_new(&cb_data->arena->allocator, wait, 1);
		*wait = (Gpu_Vk_Command_Wait) {
			.semaphore = semaphore,
			.value = 0,
		};

		gpu_vk_command_seq_push(&cb_data->commands, (Gpu_Vk_Command) {
			.kind = Gpu_Vk_Command_Kind_Wait,
			.data = {
				.wait = wait,
			},
		});
	}

	// Automatically present image when command buffer completes
	gpu_vk_swapchain_present_seq_push(&cb_data->swapchain_presents, (Gpu_Vk_Swapchain_Present) {
		.swapchain_instance = swapchain_instance,
		.present_semaphore = swapchain_instance->present_semaphores[image_index],
		.texture = swapchain_texture,
		.image_index = image_index,
		.image_available_semaphore = semaphore,
	});

	*out_texture = swapchain_texture;
	return true;
}

void gpu_vk_begin_render(Gpu_Vk_Command_Buffer cb, const Gpu_Vk_Render_Pass* render_pass) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Vk_Command_Begin_Render* begin_render;
	allocator_new(&cb_data->arena->allocator, begin_render, 1);
	*begin_render = (Gpu_Vk_Command_Begin_Render) {
		.render_pass = *render_pass,
	};

	gpu_vk_command_seq_push(&cb_data->commands, (Gpu_Vk_Command) {
		.kind = Gpu_Vk_Command_Kind_Begin_Render,
		.data = {
			.begin_render = begin_render,
		},
	});
}
void gpu_vk_end_render(Gpu_Vk_Command_Buffer cb) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	gpu_vk_command_seq_push(&cb_data->commands, (Gpu_Vk_Command) {
		.kind = Gpu_Vk_Command_Kind_End_Render,
	});
}

bool gpu_vk_new_pipeline_layout(const Gpu_Vk_Layout_Binding* bindings, u32 binding_count, VkShaderStageFlagBits stage_flags, VkPipelineLayout* out_layout) {
	VkDescriptorSetLayoutBinding* vk_bindings;
	allocator_new(gpu_vk.allocator, vk_bindings, binding_count);

	for (u32 binding_idx = 0; binding_idx < binding_count; binding_idx++) {
		vk_bindings[binding_idx] = (VkDescriptorSetLayoutBinding) {
			.descriptorType = gpu_vk_binding_kind_to_vk_descriptor_type(bindings[binding_idx].kind),
			.binding = bindings[binding_idx].index,
			.descriptorCount = 1,
			.stageFlags = stage_flags,
		};
	}

	VkDescriptorSetLayoutCreateInfo set_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
		.pBindings = vk_bindings,
		.bindingCount = binding_count,
	};

	VkDescriptorSetLayout set_layout;
	VkResult create_set_layout_result = vkCreateDescriptorSetLayout(gpu_vk.device, &set_layout_create_info, NULL, &set_layout);
	allocator_free(gpu_vk.allocator, vk_bindings, binding_count);
	if (create_set_layout_result != VK_SUCCESS) {
		return false;
	}

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pSetLayouts = &set_layout,
		.setLayoutCount = 1,
	};

	VkPipelineLayout pipeline_layout;
	VkResult create_pipeline_layout_result = vkCreatePipelineLayout(gpu_vk.device, &pipeline_layout_create_info, NULL, &pipeline_layout);
	if (create_pipeline_layout_result != VK_SUCCESS) {
		return false;
	}

	*out_layout = pipeline_layout;
	return true;
}

Gpu_Vk_Compute_Pipeline gpu_vk_new_compute_pipeline(const Gpu_Vk_Compute_Pipeline_Desc* desc) {
	if (desc->code.size == 0) {
		return SL_HANDLE_NULL;
	}

	Gpu_Vk_Compute_Pipeline pipeline = gpu_vk_compute_pipeline_pool_acquire(&gpu_vk.compute_pipeline_pool);
	Gpu_Vk_Compute_Pipeline_Data* pipeline_data = gpu_vk_compute_pipeline_pool_resolve(&gpu_vk.compute_pipeline_pool, pipeline);

	VkShaderModuleCreateInfo module_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pCode = desc->code.code,
		.codeSize = desc->code.size,
	};
	VkResult create_module_result = vkCreateShaderModule(gpu_vk.device, &module_create_info, NULL, &pipeline_data->shader_module);
	if (create_module_result != VK_SUCCESS) {
		gpu_vk_log("Failed to create compute pipeline (VkShaderModule).");
		gpu_vk_compute_pipeline_pool_release(&gpu_vk.compute_pipeline_pool, pipeline);
		return SL_HANDLE_NULL;
	}

	bool create_layout_result = gpu_vk_new_pipeline_layout(desc->bindings, desc->binding_count, VK_SHADER_STAGE_COMPUTE_BIT, &pipeline_data->pipeline_layout);
	if (!create_layout_result) {
		gpu_vk_log("Failed to create compute pipeline (VkPipelineLayout).");
		vkDestroyShaderModule(gpu_vk.device, pipeline_data->shader_module, NULL);
		gpu_vk_compute_pipeline_pool_release(&gpu_vk.compute_pipeline_pool, pipeline);
		return SL_HANDLE_NULL;
	}

	VkComputePipelineCreateInfo pipeline_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = pipeline_data->shader_module,
			.pName = desc->entry_point,
		},
		.layout = pipeline_data->pipeline_layout,
	};
	VkResult create_pipeline_result = vkCreateComputePipelines(gpu_vk.device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline_data->pipeline);
	if (create_pipeline_result != VK_SUCCESS) {
		gpu_vk_log("Failed to create compute pipeline (VkPipeline).");
		vkDestroyShaderModule(gpu_vk.device, pipeline_data->shader_module, NULL);
		vkDestroyPipelineLayout(gpu_vk.device, pipeline_data->pipeline_layout, NULL);
		gpu_vk_compute_pipeline_pool_release(&gpu_vk.compute_pipeline_pool, pipeline);
		return SL_HANDLE_NULL;
	}

	gpu_vk_log("Created compute pipeline.");
	return pipeline;
}

void gpu_vk_dispatch(Gpu_Vk_Command_Buffer cb, Gpu_Vk_Compute_Pipeline pipeline, const Gpu_Vk_Binding* bindings, u32 binding_count, vec3_u32 group_count) {
	gpu_vk_validate(gpu_vk_compute_pipeline_pool_resolve(&gpu_vk.compute_pipeline_pool, pipeline), "Invalid pipeline.");

	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	gpu_vk_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Vk_Binding* bindings_copy;
	allocator_new(&cb_data->arena->allocator, bindings_copy, binding_count);
	memcpy(bindings_copy, bindings, sizeof(Gpu_Vk_Binding) * binding_count);

	Gpu_Vk_Command_Dispatch* dispatch;
	allocator_new(&cb_data->arena->allocator, dispatch, 1);
	*dispatch = (Gpu_Vk_Command_Dispatch) {
		.pipeline = pipeline,
		.bindings = bindings_copy,
		.binding_count = binding_count,
		.group_count = group_count,
	};

	gpu_vk_command_seq_push(&cb_data->commands, (Gpu_Vk_Command) {
		.kind = Gpu_Vk_Command_Kind_Dispatch,
		.data.dispatch = dispatch,
	});
}

void gpu_vk_blit(Gpu_Vk_Command_Buffer cb, const Gpu_Vk_Blit_Desc* desc) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	gpu_vk_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Vk_Command_Blit* blit;
	allocator_new(&cb_data->arena->allocator, blit, 1);
	*blit = (Gpu_Vk_Command_Blit) {
		.desc = *desc,
	};

	gpu_vk_command_seq_push(&cb_data->commands, (Gpu_Vk_Command) {
		.kind = Gpu_Vk_Command_Kind_Blit,
		.data.blit = blit,
	});
}

void gpu_vk_barrier(Gpu_Vk_Command_Buffer cb) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	gpu_vk_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	gpu_vk_command_seq_push(&cb_data->commands, (Gpu_Vk_Command) {
		.kind = Gpu_Vk_Command_Kind_Barrier,
	});
}

void gpu_vk_semaphore_flush_pending_callbacks(Gpu_Vk_Semaphore_Data* data) {
	for (u32 callback_idx = 0; callback_idx < gpu_vk_semaphore_on_notify_callback_seq_get_count(&data->pending_callbacks);) {
		Gpu_Vk_Semaphore_On_Notify_Callback callback = gpu_vk_semaphore_on_notify_callback_seq_get(&data->pending_callbacks, callback_idx);
		if (callback.value <= data->current_value) {
			callback.fn(callback.ctx);
			gpu_vk_semaphore_on_notify_callback_seq_remove(&data->pending_callbacks, callback_idx);
		} else {
			callback_idx++;
		}
	}
}

void* gpu_vk_semaphore_thread(void* ctx) {
	Gpu_Vk_Semaphore_Data* data = ctx;

	u64 wait_value = 0;
	for (;;) {
		++wait_value;
		const VkSemaphoreWaitInfo wait_info = {
		    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		    .pNext = NULL,
		    .flags = 0,
		    .semaphoreCount = 1,
		    .pSemaphores = &data->vk_semaphore,
		    .pValues = &wait_value,
		};
		VkResult wait_result = vkWaitSemaphores(gpu_vk.device, &wait_info, u64_max);
		sl_assert(wait_result == VK_SUCCESS, "Failed to wait on semaphore.");

		VkResult counter_result = vkGetSemaphoreCounterValue(gpu_vk.device, data->vk_semaphore, &wait_value);
		sl_assert(counter_result == VK_SUCCESS, "Failed to get current semaphore value.");

		sl_mutex_lock(&data->mutex);
		data->current_value = sl_max(data->current_value, wait_value);

		gpu_vk_semaphore_flush_pending_callbacks(data);

		if (data->destroying && (wait_value >= data->current_value)) {
			sl_mutex_unlock(&data->mutex);
			break;
		}
		sl_mutex_unlock(&data->mutex);
	}

	return NULL;
}

Gpu_Vk_Semaphore gpu_vk_new_semaphore(void) {
	Gpu_Vk_Semaphore result = gpu_vk_semaphore_pool_acquire(&gpu_vk.semaphore_pool);
	Gpu_Vk_Semaphore_Data* result_data = gpu_vk_semaphore_pool_resolve(&gpu_vk.semaphore_pool, result);

	VkSemaphoreTypeCreateInfo semaphore_type_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = 0,
	};
	VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &semaphore_type_create_info,
	};
	VkResult create_semaphore_result = vkCreateSemaphore(gpu_vk.device, &semaphore_create_info, NULL, &result_data->vk_semaphore);
	sl_assert(create_semaphore_result == VK_SUCCESS, "Failed to create timeline semaphore");

	result_data->current_value = 0;
	result_data->mutex = sl_mutex_new();
	result_data->thread = sl_thread_new(result_data, gpu_vk_semaphore_thread);
	result_data->pending_callbacks = gpu_vk_semaphore_on_notify_callback_seq_new(gpu_vk.allocator, 0);

	return result;
}

void gpu_vk_destroy_semaphore(Gpu_Vk_Semaphore semaphore) {
	Gpu_Vk_Semaphore_Data* data = gpu_vk_semaphore_pool_resolve(&gpu_vk.semaphore_pool, semaphore);

	sl_mutex_lock(&data->mutex);
	sl_assert(gpu_vk_semaphore_on_notify_callback_seq_get_count(&data->pending_callbacks) == 0, "Can't destroy a semaphore that has pending notifications (i.e. gpu_vk_notify()).");
	data->destroying = true;
	sl_mutex_unlock(&data->mutex);

	// in order to clean up the thread, we must signal the semaphore.
	gpu_vk_signal_cpu(semaphore, u64_max);

	sl_thread_join(&data->thread);
	sl_mutex_destroy(&data->mutex);

	gpu_vk_semaphore_on_notify_callback_seq_destroy(&data->pending_callbacks);
	gpu_vk_semaphore_pool_release(&gpu_vk.semaphore_pool, semaphore);
}

void gpu_vk_notify(Gpu_Vk_Semaphore semaphore, u64 value, void* ctx, Gpu_Vk_On_Notify_Fn fn) {
	Gpu_Vk_Semaphore_Data* data = gpu_vk_semaphore_pool_resolve(&gpu_vk.semaphore_pool, semaphore);
	sl_mutex_lock(&data->mutex);
	const bool pending_notify = (value > data->current_value);
	if (pending_notify) {
		Gpu_Vk_Semaphore_On_Notify_Callback callback = {
			.ctx = ctx,
			.value = value,
			.fn = fn,
		};
		gpu_vk_semaphore_on_notify_callback_seq_push(&data->pending_callbacks, callback);
	}
	sl_mutex_unlock(&data->mutex);

	if (!pending_notify) {
		fn(ctx);
	}
}

void gpu_vk_wait_gpu(Gpu_Vk_Command_Buffer cb, Gpu_Vk_Semaphore semaphore, u64 value) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Vk_Semaphore_Data* semaphore_data = gpu_vk_semaphore_pool_resolve(&gpu_vk.semaphore_pool, semaphore);
	sl_assert(semaphore_data != NULL, "Invalid semaphore");

	Gpu_Vk_Command_Wait* wait;
	allocator_new(&cb_data->arena->allocator, wait, 1);
	*wait = (Gpu_Vk_Command_Wait) {
		.semaphore = semaphore_data->vk_semaphore,
		.value = value,
	};

	gpu_vk_command_seq_push(&cb_data->commands, (Gpu_Vk_Command) {
		.kind = Gpu_Vk_Command_Kind_Wait,
		.data = {
			.wait = wait,
		},
	});
}
void gpu_vk_signal_gpu(Gpu_Vk_Command_Buffer cb, Gpu_Vk_Semaphore semaphore, u64 value) {
	Gpu_Vk_Command_Buffer_Data* cb_data = gpu_vk_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Vk_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Vk_Semaphore_Data* semaphore_data = gpu_vk_semaphore_pool_resolve(&gpu_vk.semaphore_pool, semaphore);
	sl_assert(semaphore_data != NULL, "Invalid semaphore");

	Gpu_Vk_Command_Signal* signal;
	allocator_new(&cb_data->arena->allocator, signal, 1);
	*signal = (Gpu_Vk_Command_Signal) {
		.semaphore = semaphore_data->vk_semaphore,
		.value = value,
	};

	gpu_vk_command_seq_push(&cb_data->commands, (Gpu_Vk_Command) {
		.kind = Gpu_Vk_Command_Kind_Signal,
		.data = {
			.signal = signal,
		},
	});
}

void gpu_vk_signal_cpu(Gpu_Vk_Semaphore semaphore, u64 value) {
	Gpu_Vk_Semaphore_Data* data = gpu_vk_semaphore_pool_resolve(&gpu_vk.semaphore_pool, semaphore);
	sl_assert(data, "Invalid semaphore.");

	VkSemaphoreSignalInfo signal_info = {
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
	    .pNext = NULL,
	    .semaphore = data->vk_semaphore,
	    .value = value,
	};
	VkResult result = vkSignalSemaphore(gpu_vk.device, &signal_info);
	sl_assert(result == VK_SUCCESS, "Failed to signal semaphore.");

	sl_mutex_lock(&data->mutex);
	data->current_value = sl_max(data->current_value, value);
	gpu_vk_semaphore_flush_pending_callbacks(data);
	sl_mutex_unlock(&data->mutex);
}
void gpu_vk_wait_cpu(Gpu_Vk_Semaphore semaphore, u64 value) {
	Gpu_Vk_Semaphore_Data* data = gpu_vk_semaphore_pool_resolve(&gpu_vk.semaphore_pool, semaphore);
	sl_assert(data, "Invalid semaphore.");

	sl_mutex_lock(&data->mutex);
	const bool already_signalled = (value <= data->current_value);
	sl_mutex_unlock(&data->mutex);

	if (already_signalled) {
		return;
	} else {
		const VkSemaphoreWaitInfo wait_info = {
		    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		    .pNext = NULL,
		    .flags = 0,
		    .semaphoreCount = 1,
		    .pSemaphores = &data->vk_semaphore,
		    .pValues = &value,
		};
		VkResult result = vkWaitSemaphores(gpu_vk.device, &wait_info, u64_max);
		sl_assert(result == VK_SUCCESS, "Failed to wait on semaphore.");

		sl_mutex_lock(&data->mutex);
		data->current_value = sl_max(data->current_value, value);
		gpu_vk_semaphore_flush_pending_callbacks(data);
		sl_mutex_unlock(&data->mutex);
	}
}
