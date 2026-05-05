#pragma once

#include "core.h"
#include "vulkan/vulkan_core.h"
#define gpu_LOGGING 1
#define gpu_VALIDATION 1

#include "gpu3.h"
#include <string.h>

#if gpu_LOGGING
	#define gpu_log(fmt, ...) \
		printf("[Gpu]: " fmt "\n", ##__VA_ARGS__)
#else
	#define gpu_log(fmt, ...) ((void)0)
#endif

#if gpu_VALIDATION
	#define gpu_validate(condition, message) sl_assert(condition, message)
#else
	#define gpu_validate(condition, message) ((void)0)
#endif

const char* gpu_DEVICE_EXTENSIONS[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
	VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
};

typedef SL_Handle Gpu_Render_Pass_Object;

typedef struct Gpu_Render_Pass_Object_Key {
	Gpu_Render_Pass_Layout layout;
} Gpu_Render_Pass_Object_Key;

typedef struct Gpu_Render_Pass_Object_Data {
	u32 generation;
	u32 rc;

	Gpu_Render_Pass_Object_Key key;
	VkRenderPass render_pass;

	// todo
	Gpu_Render_Pass_Object older;
	Gpu_Render_Pass_Object newer;
} Gpu_Render_Pass_Object_Data;

typedef struct Gpu_Render_Pass_Object_LRU {
	Gpu_Render_Pass_Object oldest;
	Gpu_Render_Pass_Object newest;
} Gpu_Render_Pass_Object_LRU;

sl_inline bool gpu_render_pass_layout_equals(const Gpu_Render_Pass_Layout* a, const Gpu_Render_Pass_Layout* b) {
	if (a->attachment_count != b->attachment_count) {
		return false;
	}
	for (u8 attachment_idx = 0; attachment_idx < a->attachment_count; attachment_idx++) {
		if ((a->attachments[attachment_idx].format != b->attachments[attachment_idx].format) || (a->attachments[attachment_idx].load_op != b->attachments[attachment_idx].load_op) || (a->attachments[attachment_idx].store_op != b->attachments[attachment_idx].store_op)) {
			return false;
		}
	}
	return true;
}

sl_inline u64 gpu_render_pass_object_key_hash(Gpu_Render_Pass_Object_Key key) {
	SL_Hasher hasher;
	sl_hasher_init(&hasher);
	sl_hasher_push(&hasher, immutable_buffer_for(key.layout.attachment_count));
	for (u8 attachment_idx = 0; attachment_idx < key.layout.attachment_count; attachment_idx++) {
		sl_hasher_push(&hasher, immutable_buffer_for(key.layout.attachments[attachment_idx]));
	}
	return sl_hasher_finalise(&hasher);
}
sl_inline bool gpu_render_pass_object_key_equals(Gpu_Render_Pass_Object_Key a, Gpu_Render_Pass_Object_Key b) {
	return gpu_render_pass_layout_equals(&a.layout, &b.layout);
}

sl_threadsafe_pool(Gpu_Render_Pass_Object_Data, Gpu_Render_Pass_Object_Pool, gpu_render_pass_object_pool);
sl_hashmap(Gpu_Render_Pass_Object_Key, Gpu_Render_Pass_Object, Gpu_Render_Pass_Object_Map, gpu_render_pass_object_map, gpu_render_pass_object_key_hash, gpu_render_pass_object_key_equals);

typedef SL_Handle Gpu_Framebuffer;

typedef struct Gpu_Framebuffer_Key_Attachment {
	Gpu_Texture texture;
} Gpu_Framebuffer_Key_Attachment;

typedef struct Gpu_Framebuffer_Key {
	Gpu_Render_Pass_Layout layout;
	Gpu_Texture textures[GPU_MAX_ATTACHMENTS];
} Gpu_Framebuffer_Key;

sl_inline Gpu_Framebuffer_Key gpu_new_framebuffer_key(const Gpu_Render_Pass_Layout* layout, const Gpu_Render_Pass_Values* values) {
	Gpu_Framebuffer_Key result = {
		.layout = *layout,
	};
	for (u8 attachment_idx = 0; attachment_idx < values->attachment_count; attachment_idx++) {
		const Gpu_Render_Pass_Values_Attachment* att = &values->attachments[attachment_idx];
		result.textures[attachment_idx] = att->texture;
	}
	return result;
}

typedef struct Gpu_Framebuffer_Data {
	u32 generation;

	Gpu_Framebuffer_Key key;
	u32 rc;

	VkFramebuffer framebuffer; // owned
	Gpu_Render_Pass_Object render_pass_object; // retained

	// todo
	Gpu_Framebuffer older;
	Gpu_Framebuffer newer;
} Gpu_Framebuffer_Data;

typedef struct Gpu_Framebuffer_LRU {
	Gpu_Framebuffer oldest;
	Gpu_Framebuffer newest;
} Gpu_Framebuffer_LRU;

sl_inline u64 gpu_framebuffer_key_hash(Gpu_Framebuffer_Key key) {
	SL_Hasher hasher;
	sl_hasher_init(&hasher);
	sl_hasher_push(&hasher, immutable_buffer_for(key.layout.attachment_count));
	for (u8 attachment_idx = 0; attachment_idx < key.layout.attachment_count; attachment_idx++) {
		sl_hasher_push(&hasher, immutable_buffer_for(key.layout.attachments[attachment_idx]));
		sl_hasher_push(&hasher, immutable_buffer_for(key.textures[attachment_idx]));
	}
	return sl_hasher_finalise(&hasher);
}
sl_inline bool gpu_framebuffer_key_equals(Gpu_Framebuffer_Key a, Gpu_Framebuffer_Key b) {
	if (!gpu_render_pass_layout_equals(&a.layout, &b.layout)) {
		return false;
	}
	for (u8 attachment_idx = 0; attachment_idx < a.layout.attachment_count; attachment_idx++) {
		if ((a.textures[attachment_idx].index != b.textures[attachment_idx].index) || (a.textures[attachment_idx].generation != b.textures[attachment_idx].generation)) {
			return false;
		}
	}
	return true;
}

sl_threadsafe_pool(Gpu_Framebuffer_Data, Gpu_Framebuffer_Pool, gpu_framebuffer_pool);
sl_hashmap(Gpu_Framebuffer_Key, Gpu_Framebuffer, Gpu_Framebuffer_Map, gpu_framebuffer_map, gpu_framebuffer_key_hash, gpu_framebuffer_key_equals);

typedef enum Gpu_Queue {
	// Graphics + Compute
	Gpu_Queue_Primary,

	// Swapchain Present
	Gpu_Queue_Present,

	Gpu_Queue_Count
} Gpu_Queue;

typedef struct Gpu_Queue_Family_Indices {
	u32 index[Gpu_Queue_Count];
} Gpu_Queue_Family_Indices;

typedef struct Gpu_Memory_Type_Indices {
	u32 index[Gpu_Memory_Type_Count];
} Gpu_Memory_Type_Indices;

typedef struct Gpu_Heap_Data {
	u32 generation;
	VkDeviceMemory device_memory;
	VkBuffer buffer;
	Gpu_Memory_Type memory_type;
	u64 size;
	void* host_ptr;
} Gpu_Heap_Data;
sl_threadsafe_pool(Gpu_Heap_Data, Gpu_Heap_Pool, gpu_heap_pool);

typedef enum Gpu_Texture_Data_Kind {
	Gpu_Texture_Data_Kind_Immediate,
} Gpu_Texture_Data_Kind;

typedef struct Gpu_Texture_Data {
	u32 generation;

	Gpu_Texture_Data_Kind data_kind;

	union {
		// Gpu_Texture_Kind_Immediate
		struct {
			bool owned_image;
			VkImage image;
			VkImageView image_view;
			Gpu_Texture_Layout layout;
			vec3_u32 size;
			VkFormat format;
		} imm;
	};
} Gpu_Texture_Data;
sl_threadsafe_pool(Gpu_Texture_Data, Gpu_Texture_Pool, gpu_texture_pool);

typedef struct Gpu_Compute_Pipeline_Data {
	u32 generation;

	VkShaderModule shader_module;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
} Gpu_Compute_Pipeline_Data;
sl_threadsafe_pool(Gpu_Compute_Pipeline_Data, Gpu_Compute_Pipeline_Pool, gpu_compute_pipeline_pool);

typedef struct Gpu_Render_Pipeline_Data {
	u32 generation;

	Gpu_Render_Pass_Object render_pass_object; // retained

	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
} Gpu_Render_Pipeline_Data;
sl_threadsafe_pool(Gpu_Render_Pipeline_Data, Gpu_Render_Pipeline_Pool, gpu_render_pipeline_pool);

typedef struct Gpu_Semaphore_On_Notify_Callback {
	u64 value;
	void* ctx;
	Gpu_On_Notify_Fn fn;
} Gpu_Semaphore_On_Notify_Callback;
sl_seq(Gpu_Semaphore_On_Notify_Callback, Gpu_Semaphore_On_Notify_Callback_Seq, gpu_semaphore_on_notify_callback_seq);

typedef struct Gpu_Semaphore_Data {
	u32 generation;

	bool destroying;

	// timeline semaphore
	VkSemaphore vk_semaphore;

	Gpu_Semaphore_On_Notify_Callback_Seq pending_callbacks;

	u64 current_value;
	u64 next_signal_value;

	SL_Mutex mutex;
	SL_Thread thread;
} Gpu_Semaphore_Data;
sl_threadsafe_pool(Gpu_Semaphore_Data, Gpu_Semaphore_Pool, gpu_semaphore_pool);

typedef struct Gpu_Swapchain_Instance {
	atomic_u32 rc;
	Gpu_Swapchain_Desc desc;
	VkSwapchainKHR swapchain;

	VkSemaphore* present_semaphores;
	Gpu_Texture* textures;
	u32 texture_count;
} Gpu_Swapchain_Instance;

typedef struct Gpu_Swapchain_Data {
	u32 generation;
	VkSurfaceKHR surface;
	Gpu_Swapchain_Instance* current_instance;
} Gpu_Swapchain_Data;
sl_threadsafe_pool(Gpu_Swapchain_Data, Gpu_Swapchain_Pool, gpu_swapchain_pool);

typedef struct Gpu_Sampler_Data {
	u32 generation;
	VkSampler sampler;
} Gpu_Sampler_Data;
sl_threadsafe_pool(Gpu_Sampler_Data, Gpu_Sampler_Pool, gpu_sampler_pool);

typedef enum Gpu_Command_Buffer_State {
	Gpu_Command_Buffer_State_Idle,
	Gpu_Command_Buffer_State_Recording,
	Gpu_Command_Buffer_State_Enqueued
} Gpu_Command_Buffer_State;

typedef enum Gpu_Command_Kind {
	Gpu_Command_Kind_Transition_Texture_Layouts,
	Gpu_Command_Kind_Begin_Render,
	Gpu_Command_Kind_End_Render,
	Gpu_Command_Kind_Draw,
	Gpu_Command_Kind_Dispatch,
	Gpu_Command_Kind_Blit,
	Gpu_Command_Kind_Barrier,
	Gpu_Command_Kind_Wait,
	Gpu_Command_Kind_Signal,
} Gpu_Command_Kind;

typedef struct Gpu_Command_Begin_Render {
	Gpu_Render_Pass_Layout layout;
	Gpu_Render_Pass_Values values;
} Gpu_Command_Begin_Render;

typedef struct Gpu_Command_Draw {
	Gpu_Draw_Desc desc;
} Gpu_Command_Draw;

typedef struct Gpu_Command_Dispatch {
	Gpu_Compute_Pipeline pipeline;
	const Gpu_Binding* bindings;
	u32 binding_count;
	vec3_u32 group_count;
} Gpu_Command_Dispatch;

typedef struct Gpu_Command_Transition_Texture_Layouts {
	const Gpu_Texture* textures;
	const Gpu_Texture_Layout* layouts;
	u32 count;
} Gpu_Command_Transition_Texture_Layouts;

typedef struct Gpu_Command_Blit {
	Gpu_Blit_Desc desc;
} Gpu_Command_Blit;

typedef struct Gpu_Command_Wait {
	VkSemaphore semaphore;
	u64 value;
} Gpu_Command_Wait;

typedef struct Gpu_Command_Signal {
	VkSemaphore semaphore;
	u64 value;
} Gpu_Command_Signal;

typedef struct Gpu_Command {
	Gpu_Command_Kind kind;

	union {
		Gpu_Command_Transition_Texture_Layouts* transition_texture_layouts;
		Gpu_Command_Begin_Render* begin_render;
		Gpu_Command_Draw* draw;
		Gpu_Command_Dispatch* dispatch;
		Gpu_Command_Blit* blit;
		Gpu_Command_Wait* wait;
		Gpu_Command_Signal* signal;
	} data;
} Gpu_Command;
sl_seq(Gpu_Command, Gpu_Command_Seq, gpu_command_seq);

sl_seq(VkCommandBuffer, Gpu_Command_Buffer_Seq, gpu_command_buffer_seq);
sl_seq(VkSemaphore, Gpu_Semaphore_Seq, gpu_semaphore_seq);

typedef struct Gpu_Swapchain_Present {
	Gpu_Swapchain_Instance* swapchain_instance; // retained
	Gpu_Texture texture;
	u32 image_index;
	VkSemaphore image_available_semaphore;
	VkSemaphore present_semaphore;
} Gpu_Swapchain_Present;
sl_seq(Gpu_Swapchain_Present, Gpu_Swapchain_Present_Seq, gpu_swapchain_present_seq);

typedef struct Gpu_Command_Buffer_Data {
	Gpu_Command_Buffer_State state;
	VkCommandPool command_pool;

	// On the global semaphore, the value that indicates that GPU work for the command buffer has completed.
	u64 gpu_complete_semaphore_value;

	Gpu_Semaphore_Seq semaphores;
	u32 next_free_semaphore;

	Gpu_Command_Buffer_Seq command_buffers;
	u32 next_free_command_buffer;

	Gpu_Swapchain_Present_Seq swapchain_presents;

	SL_Arena_Allocator* arena;
	Gpu_Command_Seq commands;
} Gpu_Command_Buffer_Data;

typedef struct Gpu_Command_Buffer_Pool_Data {
	u32 generation;

	Gpu_Command_Buffer_Data* command_buffers;
	u32 command_buffer_count;
	u32 next_command_buffer;
} Gpu_Command_Buffer_Pool_Data;
sl_threadsafe_pool(Gpu_Command_Buffer_Pool_Data, Gpu_Command_Buffer_Pool_Pool, gpu_command_buffer_pool_pool);

typedef struct Gpu_Device_Function_Table {
	PFN_vkCmdPipelineBarrier2KHR vkCmdPipelineBarrier2KHR;
	PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR;
} Gpu_Device_Function_Table;

typedef struct Gpu {
	Allocator* allocator;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	Gpu_Device_Function_Table device_function_table;
	VkPhysicalDeviceLimits device_limits;

	// Currently necessary due to how texture transitions are tracked.
	SL_Mutex enqueue_mutex;

	Gpu_Semaphore global_semaphore;
	u64 global_semaphore_value;

	Gpu_Swapchain_Init_Desc swapchain_init_desc;

	Gpu_Memory_Type_Indices memory_type_indices;
	Gpu_Queue_Family_Indices queue_family_indices;
	VkQueue queue[Gpu_Queue_Count];

	Gpu_Heap_Pool heap_pool;
	Gpu_Texture_Pool texture_pool;
	Gpu_Command_Buffer_Pool_Pool command_pool_pool;
	Gpu_Render_Pipeline_Pool render_pipeline_pool;
	Gpu_Compute_Pipeline_Pool compute_pipeline_pool;
	Gpu_Swapchain_Pool swapchain_pool;
	Gpu_Semaphore_Pool semaphore_pool;
	Gpu_Sampler_Pool sampler_pool;

	Gpu_Render_Pass_Object_Pool render_pass_object_pool;
	Gpu_Render_Pass_Object_Map render_pass_object_map;
	Gpu_Render_Pass_Object_LRU render_pass_object_lru;

	Gpu_Framebuffer_Pool framebuffer_pool;
	Gpu_Framebuffer_Map framebuffer_map;
	Gpu_Framebuffer_LRU framebuffer_lru;
} Gpu;

static Gpu gpu;

void gpu_init_instance(const Gpu_Desc* desc) {
	gpu_log("Creating instance.");

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

#if gpu_VALIDATION
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
				gpu_log("Enabling layer: %s", req_validation_layers[req_validation_layer_idx]);
				found_validation_layers[found_validation_layer_count++] = req_validation_layers[req_validation_layer_idx];
			} else {
				gpu_log("Could not find layer: %s", req_validation_layers[req_validation_layer_idx]);
			}
		}

		create_info.ppEnabledLayerNames = found_validation_layers;
		create_info.enabledLayerCount = found_validation_layer_count;
	}
#endif

	VkResult res = vkCreateInstance(&create_info, NULL, &gpu.instance);
	sl_assert(res == VK_SUCCESS, "Failed to create Vulkan instance.");

	allocator_free(allocator, combined_extensions, combined_extension_count);
}

Gpu_Queue_Family_Indices gpu_get_physical_device_queue_families(VkPhysicalDevice device, Allocator* scratch_allocator) {
	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

	VkQueueFamilyProperties* queue_families;
	allocator_new(scratch_allocator, queue_families, queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

	Gpu_Queue_Family_Indices indices = {
		.index = {
			u32_max,
			u32_max
		},
	};
	for (u32 queue_idx = 0; queue_idx < queue_family_count; queue_idx++) {
		VkQueueFamilyProperties queue_family = queue_families[queue_idx];

		// TODO
		// VkBool32 present_support = false;
		// vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_idx, gpu.surface, &present_support);
		bool present_support = true;

		const VkQueueFlagBits primary_queue_bits = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

		if (indices.index[Gpu_Queue_Primary] == u32_max && (queue_family.queueFlags & primary_queue_bits) == primary_queue_bits) {
			indices.index[Gpu_Queue_Primary] = queue_idx;
		}
		if (indices.index[Gpu_Queue_Present] == u32_max && present_support) {
			indices.index[Gpu_Queue_Present] = queue_idx;
		}
	}

	allocator_free(scratch_allocator, queue_families, queue_family_count);

	return indices;
}

bool gpu_queue_family_indices_is_complete(Gpu_Queue_Family_Indices indices) {
	for (Gpu_Queue queue = 0; queue < Gpu_Queue_Count; queue++) {
		if (indices.index[queue] == u32_max) {
			return false;
		}
	}
	return true;
}

Gpu_Memory_Type_Indices gpu_get_physical_device_memory_type_indices(VkPhysicalDevice physical_device) {
	VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);

	VkMemoryPropertyFlagBits required_flags[Gpu_Memory_Type_Count] = {
		[Gpu_Memory_Type_Host_Visible] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		[Gpu_Memory_Type_Device_Local] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	};

	VkMemoryPropertyFlagBits undesirable_flags[Gpu_Memory_Type_Count] = {
		[Gpu_Memory_Type_Host_Visible] = 0,
		[Gpu_Memory_Type_Device_Local] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	};

	Gpu_Memory_Type_Indices indices = {
		.index = {
			u32_max,
			u32_max,
		},
	};

	for (Gpu_Memory_Type type = 0; type < Gpu_Memory_Type_Count; type++) {
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

bool gpu_is_device_suitable(VkPhysicalDevice device, Allocator* scratch_allocator) {
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

	Gpu_Queue_Family_Indices queue_indices = gpu_get_physical_device_queue_families(device, scratch_allocator);
	if (!gpu_queue_family_indices_is_complete(queue_indices)) {
		return false;
	}

	u32 available_extension_count = 0;
	vkEnumerateDeviceExtensionProperties(device, NULL, &available_extension_count, NULL);

	VkExtensionProperties* available_extensions;
	allocator_new(gpu.allocator, available_extensions, available_extension_count);
	vkEnumerateDeviceExtensionProperties(device, NULL, &available_extension_count, available_extensions);

	for (u32 required_extension_idx = 0; required_extension_idx < sl_array_count(gpu_DEVICE_EXTENSIONS); required_extension_idx++) {
		const char* required_extension = gpu_DEVICE_EXTENSIONS[required_extension_idx];

		bool found_extension = false;
		for (u32 available_extension_idx = 0; available_extension_idx < available_extension_count; available_extension_idx++) {
			if (strcmp(required_extension, available_extensions[available_extension_idx].extensionName) == 0) {
				found_extension = true;
				break;
			}
		}

		if (!found_extension) {
			allocator_free(gpu.allocator, available_extensions, available_extension_count);
			return false;
		}
	}

	allocator_free(gpu.allocator, available_extensions, available_extension_count);

	return true;
}

bool gpu_is_device_discrete(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);
	return (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
}

VkPhysicalDevice gpu_find_physical_device(void) {
	gpu_log("Finding physical device.");

	Allocator* allocator = gpu.allocator;

	u32 device_count = 0;
	vkEnumeratePhysicalDevices(gpu.instance, &device_count, NULL);
	gpu_log("%u available device(s).", device_count);
	sl_assert(device_count > 0, "Failed to find GPU with Vulkan support.");

	VkPhysicalDevice* devices;
	allocator_new(allocator, devices, device_count);
	vkEnumeratePhysicalDevices(gpu.instance, &device_count, devices);

	u32 suitable_device_count = 0;
	VkPhysicalDevice* suitable_devices;
	allocator_new(allocator, suitable_devices, device_count);
	for (u32 device_idx = 0; device_idx < device_count; device_idx++) {
		if (gpu_is_device_suitable(devices[device_idx], allocator)) {
			suitable_devices[suitable_device_count++] = devices[device_idx];
		}
	}
	gpu_log("%u suitable device(s).", suitable_device_count);
	sl_assert(suitable_device_count > 0, "No suitable GPU found.");

	for (u32 suitable_device_idx = 0; suitable_device_idx < suitable_device_count; suitable_device_idx++) {
		VkPhysicalDevice device = suitable_devices[suitable_device_idx];
		if (gpu_is_device_discrete(device)) {
			// Use the first discrete GPU we find.
			return device;
		}
	}

	// Fallback
	return suitable_devices[0];
}

void gpu_init_device(void) {
	Allocator* allocator = gpu.allocator;

	VkPhysicalDevice physical_device = gpu_find_physical_device();
	gpu.physical_device = physical_device;

	VkPhysicalDeviceProperties physical_device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
	gpu_log("Creating logical device for \"%s\".", physical_device_properties.deviceName);

	gpu.device_limits = physical_device_properties.limits;

	Gpu_Queue_Family_Indices queue_indices = gpu_get_physical_device_queue_families(physical_device, allocator);
	gpu.queue_family_indices = queue_indices;

	gpu.memory_type_indices = gpu_get_physical_device_memory_type_indices(physical_device);

	const f32 queue_priority = 1.0f;

	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

	u32 queue_create_info_count = 0;
	VkDeviceQueueCreateInfo* queue_create_infos;
	allocator_new(gpu.allocator, queue_create_infos, queue_family_count);

	// Deduplicate required queues
	for (u32 queue_family_idx = 0; queue_family_idx < queue_family_count; queue_family_idx++) {
		bool create_queue = false;
		for (Gpu_Queue queue = 0; queue < Gpu_Queue_Count; queue++) {
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
	device_create_info.ppEnabledExtensionNames = gpu_DEVICE_EXTENSIONS;
	device_create_info.enabledExtensionCount = sl_array_count(gpu_DEVICE_EXTENSIONS);

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


	VkPhysicalDeviceVulkan11Features device_features_11 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.shaderDrawParameters = true,
	};
	timeline_semaphore_features.pNext = &device_features_11;

	VkResult create_device_result = vkCreateDevice(physical_device, &device_create_info, NULL, &gpu.device);
	sl_assert(create_device_result == VK_SUCCESS, "Failed to create logical device.");

	allocator_free(gpu.allocator, queue_create_infos, queue_family_count);

	for (Gpu_Queue queue = 0; queue < Gpu_Queue_Count; queue++) {
		vkGetDeviceQueue(gpu.device, queue_indices.index[queue], 0, &gpu.queue[queue]);
	}

	gpu.device_function_table = (Gpu_Device_Function_Table) {
		.vkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)vkGetDeviceProcAddr(gpu.device, "vkCmdPipelineBarrier2KHR"),
		.vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(gpu.device, "vkCmdPushDescriptorSetKHR"),
	};
}

// Texture
VkImageView gpu_texture_get_image_view(Gpu_Texture texture) {
	Gpu_Texture_Data* data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
	sl_assert(data != NULL, "Texture is invalid.");

	switch (data->data_kind) {
		case Gpu_Texture_Data_Kind_Immediate: {
			return data->imm.image_view;
		} break;
	}
}
Gpu_Texture gpu_texture_get_root(Gpu_Texture texture) {
	Gpu_Texture_Data* data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
	sl_assert(data != NULL, "Texture is invalid.");

	switch (data->data_kind) {
		case Gpu_Texture_Data_Kind_Immediate: {
			return texture;
		} break;
	}
}
Gpu_Texture_Data* gpu_texture_get_root_data(Gpu_Texture texture) {
	return gpu_texture_pool_resolve(&gpu.texture_pool, gpu_texture_get_root(texture));
}


sl_inline VkImageType gpu_texture_kind_to_vk_image_type(Gpu_Texture_Kind kind) {
	switch (kind) {
		case Gpu_Texture_Kind_1D: return VK_IMAGE_TYPE_1D;
		case Gpu_Texture_Kind_2D: return VK_IMAGE_TYPE_2D;
		case Gpu_Texture_Kind_3D: return VK_IMAGE_TYPE_3D;
	}
}
sl_inline VkImageViewType gpu_texture_kind_to_vk_image_view_type(Gpu_Texture_Kind kind) {
	switch (kind) {
		case Gpu_Texture_Kind_1D: return VK_IMAGE_VIEW_TYPE_1D;
		case Gpu_Texture_Kind_2D: return VK_IMAGE_VIEW_TYPE_2D;
		case Gpu_Texture_Kind_3D: return VK_IMAGE_VIEW_TYPE_3D;
	}
}
sl_inline VkImageUsageFlags gpu_texture_usage_to_vk_image_usage_flags(Gpu_Texture_Usage usage) {
	VkImageUsageFlags flags = 0;

	// For blit
	flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if ((usage & Gpu_Texture_Usage_Shader_Read) > 0) {
		flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	if ((usage & Gpu_Texture_Usage_Shader_Write) > 0) {
		flags |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if ((usage & Gpu_Texture_Usage_Render_Attachment) > 0) {
		flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	return flags;
}

sl_inline VkFormat gpu_format_to_vk_format(Gpu_Format format) {
	switch (format) {
		case Gpu_Format_RGBA8_Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
		case Gpu_Format_RGBA8_sRGB: return VK_FORMAT_R8G8B8A8_SRGB;
		case Gpu_Format_BGRA8_Unorm: return VK_FORMAT_B8G8R8A8_UNORM;
		case Gpu_Format_BGRA8_sRGB: return VK_FORMAT_B8G8R8A8_SRGB;
		case Gpu_Format_RGBA16_Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Gpu_Format_RGBA32_Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
}

sl_inline VkColorSpaceKHR gpu_colorspace_to_vk_colorspace(Gpu_Colorspace colorspace) {
	switch (colorspace) {
		case Gpu_Colorspace_sRGB: return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		case Gpu_Colorspace_Extended_sRGB_Linear: return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
	}
}

sl_inline VkImageLayout gpu_texture_layout_to_vk_image_layout(Gpu_Texture_Layout layout) {
	switch (layout) {
		case Gpu_Texture_Layout_Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
		case Gpu_Texture_Layout_General: return VK_IMAGE_LAYOUT_GENERAL;
		case Gpu_Texture_Layout_Shader_Read: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case Gpu_Texture_Layout_Shader_Write: return VK_IMAGE_LAYOUT_GENERAL;
		case Gpu_Texture_Layout_Color_Attachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case Gpu_Texture_Layout_Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
}

sl_inline VkFilter gpu_filter_to_vk_filter(Gpu_Filter filter) {
	switch (filter) {
		case Gpu_Filter_Nearest: return VK_FILTER_NEAREST;
		case Gpu_Filter_Linear: return VK_FILTER_LINEAR;
	}
}

sl_inline VkSamplerMipmapMode gpu_filter_to_vk_sampler_mipmap_mode(Gpu_Filter filter) {
	switch (filter) {
		case Gpu_Filter_Nearest: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case Gpu_Filter_Linear: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

sl_inline VkSamplerAddressMode gpu_sampler_address_mode_to_vk_sampler_address_mode(Gpu_Sampler_Address_Mode mode) {
	switch (mode) {
		case Gpu_Sampler_Address_Mode_Clamp_To_Edge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case Gpu_Sampler_Address_Mode_Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case Gpu_Sampler_Address_Mode_Mirrored_Repeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	}
}

bool gpu_new_image_for_texture_desc(const Gpu_Texture_Desc* desc, VkImage* out_image) {
	VkImageCreateInfo image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = gpu_texture_kind_to_vk_image_type(desc->kind),
		.usage = gpu_texture_usage_to_vk_image_usage_flags(desc->usage),
		.format = gpu_format_to_vk_format(desc->format),
		.extent = {
			.width = desc->size.x,
			.height = desc->size.y,
			.depth = desc->size.z,
		},
		.mipLevels = desc->mip_levels,
		.arrayLayers = desc->array_layers,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.pQueueFamilyIndices = &gpu.queue_family_indices.index[Gpu_Queue_Primary],
		.queueFamilyIndexCount = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
	};
	if (vkCreateImage(gpu.device, &image_create_info, NULL, out_image) == VK_SUCCESS) {
		return true;
	} else {
		return false;
	}
}

Gpu_Size_And_Align gpu_size_and_align_for_texture(const Gpu_Texture_Desc* desc) {
	VkImage image;
	if (!gpu_new_image_for_texture_desc(desc, &image)) {
		return (Gpu_Size_And_Align) {0};
	}

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(gpu.device, image, &mem_reqs);

	vkDestroyImage(gpu.device, image, NULL);

	return (Gpu_Size_And_Align) {
		.size = mem_reqs.size,

		// On older hardware tiled textures and buffers can't overlap the same page(s).
		// So we bump the alignment to prevent overlap.
		.align = sl_max(mem_reqs.alignment, gpu.device_limits.bufferImageGranularity),
	};
}

Gpu_Texture gpu_new_texture(const Gpu_Texture_Desc* desc, Gpu_Slice slice) {
	Gpu_Texture texture = gpu_texture_pool_acquire(&gpu.texture_pool);
	Gpu_Texture_Data* texture_data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
	texture_data->data_kind = Gpu_Texture_Data_Kind_Immediate;
	texture_data->imm.layout = Gpu_Texture_Layout_Undefined;
	texture_data->imm.size = desc->size;
	texture_data->imm.owned_image = true;

	if (!gpu_new_image_for_texture_desc(desc, &texture_data->imm.image)) {
		gpu_texture_pool_release(&gpu.texture_pool, texture);
		return SL_HANDLE_NULL;
	}

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(gpu.device, texture_data->imm.image, &mem_reqs);
	gpu_validate(slice.offset % mem_reqs.alignment == 0, "Slice has insufficient alignment for texture.");
	gpu_validate(slice.size >= mem_reqs.size, "Slice has insufficient size for texture.");

	Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, slice.heap);
	sl_assert(heap_data != NULL, "Invalid slice.");

	VkResult bind_result = vkBindImageMemory(gpu.device, texture_data->imm.image, heap_data->device_memory, slice.offset);
	if (bind_result != VK_SUCCESS) {
		vkDestroyImage(gpu.device, texture_data->imm.image, NULL);
		gpu_texture_pool_release(&gpu.texture_pool, texture);
		return SL_HANDLE_NULL;
	}

	VkImageViewCreateInfo view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = texture_data->imm.image,
		.format = gpu_format_to_vk_format(desc->format),
		.viewType = gpu_texture_kind_to_vk_image_view_type(desc->kind),
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

	VkResult create_view_result = vkCreateImageView(gpu.device, &view_create_info, NULL, &texture_data->imm.image_view);
	if (create_view_result != VK_SUCCESS) {
		vkDestroyImage(gpu.device, texture_data->imm.image, NULL);
		gpu_texture_pool_release(&gpu.texture_pool, texture);
		return SL_HANDLE_NULL;
	}

	return texture;
}
vec3_u32 gpu_get_texture_size(Gpu_Texture texture) {
	Gpu_Texture root_texture = gpu_texture_get_root(texture);
	Gpu_Texture_Data* root_data = gpu_texture_pool_resolve(&gpu.texture_pool, root_texture);
	sl_assert(root_data != NULL, "Texture is invalid.");
	sl_assert(root_data->data_kind == Gpu_Texture_Data_Kind_Immediate, "Root texture must be immediate.");
	return root_data->imm.size;
}
VkFormat gpu_get_texture_format(Gpu_Texture texture) {
	Gpu_Texture root_texture = gpu_texture_get_root(texture);
	Gpu_Texture_Data* root_data = gpu_texture_pool_resolve(&gpu.texture_pool, root_texture);
	sl_assert(root_data != NULL, "Texture is invalid.");
	sl_assert(root_data->data_kind == Gpu_Texture_Data_Kind_Immediate, "Root texture must be immediate.");
	return root_data->imm.format;
}
void gpu_destroy_texture(Gpu_Texture texture) {
	Gpu_Texture_Data* data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
	sl_assert(data != NULL, "Texture is invalid.");

	// TODO: Assert that it is not in-use

	// destroy all cached framebuffers that retain this texture (can assert that rc == 0).
	// ...

	switch (data->data_kind) {
		case Gpu_Texture_Data_Kind_Immediate: {
			vkDestroyImageView(gpu.device, data->imm.image_view, NULL);

			if (data->imm.owned_image) {
				vkDestroyImage(gpu.device, data->imm.image, NULL);
			}
		} break;
	}

	gpu_texture_pool_release(&gpu.texture_pool, texture);
}

VkAttachmentLoadOp gpu_load_op_to_vk(Gpu_Load_Op op) {
	switch (op) {
		case Gpu_Load_Op_Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
		case Gpu_Load_Op_Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case Gpu_Load_Op_Dont_Care: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
}
VkAttachmentStoreOp gpu_store_op_to_vk(Gpu_Store_Op op) {
	switch (op) {
		case Gpu_Store_Op_Store: return VK_ATTACHMENT_STORE_OP_STORE;
		case Gpu_Store_Op_Dont_Care: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
}

// Render Pass Object
void gpu_retain_render_pass_object(Gpu_Render_Pass_Object render_pass_object) {
	Gpu_Render_Pass_Object_Data* data = gpu_render_pass_object_pool_resolve(&gpu.render_pass_object_pool, render_pass_object);
	data->rc++;
}
void gpu_release_render_pass_object(Gpu_Render_Pass_Object render_pass_object) {
	Gpu_Render_Pass_Object_Data* data = gpu_render_pass_object_pool_resolve(&gpu.render_pass_object_pool, render_pass_object);
	sl_assert(data->rc > 0, "Over-released render pass object.");
	data->rc--;

	// TODO: Put on evictable LRU
}
Gpu_Render_Pass_Object gpu_acquire_render_pass_object(Gpu_Render_Pass_Object_Key key) {
	// Find existing entry
	{
		Gpu_Render_Pass_Object result;
		if (gpu_render_pass_object_map_get(&gpu.render_pass_object_map, key, &result)) {
			gpu_retain_render_pass_object(result);
			return result;
		}
	}

	// Make new entry
	{
		gpu_log("Created render pass object.");
		Gpu_Render_Pass_Object result = gpu_render_pass_object_pool_acquire(&gpu.render_pass_object_pool);
		Gpu_Render_Pass_Object_Data* data = gpu_render_pass_object_pool_resolve(&gpu.render_pass_object_pool, result);
		data->rc = 1;
		data->key = key;

		// Create render pass
		VkAttachmentDescription attachments[GPU_MAX_ATTACHMENTS];
		VkAttachmentReference attachment_refs[GPU_MAX_ATTACHMENTS];
		for (u8 attachment_idx = 0; attachment_idx < key.layout.attachment_count; attachment_idx++) {
			const Gpu_Render_Pass_Layout_Attachment* att = &key.layout.attachments[attachment_idx];
			attachments[attachment_idx] = (VkAttachmentDescription) {
				.format = gpu_format_to_vk_format(att->format),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = gpu_load_op_to_vk(att->load_op),
				.storeOp = gpu_store_op_to_vk(att->store_op),
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = gpu_texture_layout_to_vk_image_layout(att->initial_layout),
				.finalLayout = gpu_texture_layout_to_vk_image_layout(att->final_layout)
			};
			attachment_refs[attachment_idx] = (VkAttachmentReference) {
				.attachment = attachment_idx,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};
		}

		VkSubpassDescription subpass = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = key.layout.attachment_count,
			.pColorAttachments = attachment_refs,
		};

		VkRenderPassCreateInfo render_pass_create_info = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = key.layout.attachment_count,
			.pAttachments = attachments,
			.subpassCount = 1,
			.pSubpasses = &subpass,
		};

		VkResult create_render_pass_result = vkCreateRenderPass(gpu.device, &render_pass_create_info, NULL, &data->render_pass);
		sl_assert(create_render_pass_result == VK_SUCCESS, "Failed to create render pass.");

		gpu_render_pass_object_map_insert(&gpu.render_pass_object_map, key, result);

		return result;
	}
}

// Framebuffer
void gpu_retain_framebuffer(Gpu_Framebuffer framebuffer) {
	Gpu_Framebuffer_Data* data = gpu_framebuffer_pool_resolve(&gpu.framebuffer_pool, framebuffer);
	data->rc++;
}
void gpu_release_framebuffer(Gpu_Framebuffer framebuffer) {
	Gpu_Framebuffer_Data* data = gpu_framebuffer_pool_resolve(&gpu.framebuffer_pool, framebuffer);
	sl_assert(data->rc > 0, "Over-released framebuffer.");
	data->rc--;

	// TODO: Put on evictable LRU
}
Gpu_Framebuffer gpu_acquire_framebuffer(Gpu_Framebuffer_Key key) {
	sl_assert(key.layout.attachment_count > 0, "Framebuffer must have at least one attachment.");

	// Find existing entry
	{
		Gpu_Framebuffer result;
		if (gpu_framebuffer_map_get(&gpu.framebuffer_map, key, &result)) {
			gpu_retain_framebuffer(result);
			return result;
		}
	}

	// Make new entry
	{
		gpu_log("Creating new framebuffer.");
		Gpu_Framebuffer result = gpu_framebuffer_pool_acquire(&gpu.framebuffer_pool);
		Gpu_Framebuffer_Data* data = gpu_framebuffer_pool_resolve(&gpu.framebuffer_pool, result);
		data->rc = 1;
		data->key = key;

		const Gpu_Render_Pass_Object_Key render_pass_object_key = {
			.layout = key.layout,
		};
		data->render_pass_object = gpu_acquire_render_pass_object(render_pass_object_key);
		Gpu_Render_Pass_Object_Data* render_pass_object_data = gpu_render_pass_object_pool_resolve(&gpu.render_pass_object_pool, data->render_pass_object);

		VkImageView attachments[GPU_MAX_ATTACHMENTS];
		for (u8 attachment_idx = 0; attachment_idx < key.layout.attachment_count; attachment_idx++) {
			attachments[attachment_idx] = gpu_texture_get_image_view(key.textures[attachment_idx]);
		}

		const vec3_u32 size = gpu_get_texture_size(key.textures[0]);

		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = render_pass_object_data->render_pass,
			.attachmentCount = key.layout.attachment_count,
			.pAttachments = attachments,
			.width = size.x,
			.height = size.y,
			.layers = 1,
		};

		VkResult create_result = vkCreateFramebuffer(gpu.device, &framebuffer_info, NULL, &data->framebuffer);
		sl_assert(create_result == VK_SUCCESS, "Failed to create framebuffer.");

		gpu_framebuffer_map_insert(&gpu.framebuffer_map, key, result);

		return result;
	}
}

// Swapchain
Gpu_Swapchain gpu_new_swapchain(const Gpu_Swapchain_Init_Desc* init_desc) {
	Gpu_Swapchain swapchain = gpu_swapchain_pool_acquire(&gpu.swapchain_pool);
	Gpu_Swapchain_Data* swapchain_data = gpu_swapchain_pool_resolve(&gpu.swapchain_pool, swapchain);

	// Create surface
	swapchain_data->surface = init_desc->get_surface_fn(init_desc->ctx, gpu.instance);

	return swapchain;
}
void gpu_destroy_swapchain(Gpu_Swapchain swapchain) {
	// todo
}

void gpu_retain_swapchain_instance(Gpu_Swapchain_Instance* instance) {
	const u32 pre = atomic_fetch_add_explicit(&instance->rc, 1, memory_order_acq_rel);
	gpu_validate(pre > 0, "Can't retain a released swapchain.");
}
void gpu_release_swapchain_instance(Gpu_Swapchain_Instance* instance) {
	const u32 pre = atomic_fetch_sub_explicit(&instance->rc, 1, memory_order_acq_rel);
	gpu_validate(pre > 0, "Over-released");

	if (pre == 1) {
		gpu_log("Destroying swapchain instance %p\n", instance);

		// stop gap to prevent semaphore from being destroyed before present has waited on it.
		vkQueueWaitIdle(gpu.queue[Gpu_Queue_Present]);

		for (u32 texture_idx = 0; texture_idx < instance->texture_count; texture_idx++) {
			gpu_destroy_texture(instance->textures[texture_idx]);
			vkDestroySemaphore(gpu.device, instance->present_semaphores[texture_idx], NULL);
		}
		vkDestroySwapchainKHR(gpu.device, instance->swapchain, NULL);
		allocator_free(gpu.allocator, instance->textures, instance->texture_count);
		allocator_free(gpu.allocator, instance->present_semaphores, instance->texture_count);
		allocator_free(gpu.allocator, instance, 1);
	}
}

Gpu_Swapchain_Instance* gpu_get_instance(Gpu_Swapchain swapchain, Gpu_Swapchain_Desc desc) {
	Gpu_Swapchain_Data* swapchain_data = gpu_swapchain_pool_resolve(&gpu.swapchain_pool, swapchain);

	VkSurfaceCapabilitiesKHR capabilities;
	VkResult get_capabilities_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.physical_device, swapchain_data->surface, &capabilities);
	sl_assert(get_capabilities_result == VK_SUCCESS, "Failed to get swapchain capabilities.");

	// use the surface's size, if it has one.
	desc.size = (vec2_u32) { capabilities.currentExtent.width == u32_max ? desc.size.x : capabilities.currentExtent.width, capabilities.currentExtent.height == u32_max ? desc.size.y : capabilities.currentExtent.height };

	Gpu_Swapchain_Instance* current_instance = swapchain_data->current_instance;

	const bool must_rebuild = (current_instance == NULL) || (current_instance->desc.size.x != desc.size.x) || (current_instance->desc.size.y != desc.size.y) || (current_instance->desc.colorspace != desc.colorspace);
	if (!must_rebuild) {
		return current_instance;
	}

	Gpu_Swapchain_Instance* new_instance;
	allocator_new(gpu.allocator, new_instance, 1);
	*new_instance = (Gpu_Swapchain_Instance) {
		.rc = 1,
		.desc = desc,
	};

	const VkColorSpaceKHR colorspace = gpu_colorspace_to_vk_colorspace(desc.colorspace);

	u32 available_present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.physical_device, swapchain_data->surface, &available_present_mode_count, NULL);

	VkPresentModeKHR* available_present_modes;
	allocator_new(gpu.allocator, available_present_modes, available_present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.physical_device, swapchain_data->surface, &available_present_mode_count, available_present_modes);

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // default
	for (u32 available_present_mode_idx = 0; available_present_mode_idx < available_present_mode_count; available_present_mode_idx++) {
		if (available_present_modes[available_present_mode_idx] == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	allocator_free(gpu.allocator, available_present_modes, available_present_mode_count);

	const u32 min_image_count = sl_clamp(3, capabilities.minImageCount, (capabilities.maxImageCount == 0) ? u32_max : capabilities.maxImageCount);

	gpu_log("Rebuilding swapchain with extent (%u, %u), %u min images, instance %p.", desc.size.x, desc.size.y, min_image_count, new_instance);

	const VkExtent2D extent = {
		.width = desc.size.x,
		.height = desc.size.y,
	};

	VkFormat vk_format = gpu_format_to_vk_format(desc.format);

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

	Gpu_Queue_Family_Indices queue_indices = gpu_get_physical_device_queue_families(gpu.physical_device, gpu.allocator);
	if (queue_indices.index[Gpu_Queue_Primary] == queue_indices.index[Gpu_Queue_Present]) {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 3;
		create_info.pQueueFamilyIndices = queue_indices.index;
	}

	VkResult create_result = vkCreateSwapchainKHR(gpu.device, &create_info, NULL, &new_instance->swapchain);
	sl_assert(create_result == VK_SUCCESS, "Failed to create swapchain.");

	u32 swapchain_image_count;
	vkGetSwapchainImagesKHR(gpu.device, new_instance->swapchain, &swapchain_image_count, NULL);

	new_instance->texture_count = swapchain_image_count;
	allocator_new(gpu.allocator, new_instance->textures, swapchain_image_count);
	allocator_new(gpu.allocator, new_instance->present_semaphores, swapchain_image_count);

	// Get images
	VkImage* images;
	allocator_new(gpu.allocator, images, swapchain_image_count);
	vkGetSwapchainImagesKHR(gpu.device, new_instance->swapchain, &swapchain_image_count, images);

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

		Gpu_Texture texture = gpu_texture_pool_acquire(&gpu.texture_pool);
		Gpu_Texture_Data* texture_data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
		texture_data->data_kind = Gpu_Texture_Data_Kind_Immediate;
		texture_data->imm.size = (vec3_u32) { extent.width, extent.height, 1 };
		texture_data->imm.format = vk_format;
		texture_data->imm.layout = Gpu_Texture_Layout_Undefined;
		texture_data->imm.image = images[image_idx];
		texture_data->imm.owned_image = false;
		const VkResult view_create_result = vkCreateImageView(gpu.device, &view_create_info, NULL, &texture_data->imm.image_view);
		sl_assert(view_create_result == VK_SUCCESS, "Failed to create image view for swapchain.");

		new_instance->textures[image_idx] = texture;
	}

	// Create semaphores
	for (u32 image_idx = 0; image_idx < swapchain_image_count; image_idx++) {
		VkSemaphoreCreateInfo semaphore_create_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
		VkResult create_semaphore_result = vkCreateSemaphore(gpu.device, &semaphore_create_info, NULL, &new_instance->present_semaphores[image_idx]);
		sl_assert(create_semaphore_result == VK_SUCCESS, "Failed to create semaphore.");
	}

	allocator_free(gpu.allocator, images, swapchain_image_count);

	swapchain_data->current_instance = new_instance;

	if (current_instance != NULL) {
		gpu_release_swapchain_instance(current_instance);
	}

	return new_instance;
}

void gpu_init_resource_pools() {
	gpu.heap_pool = gpu_heap_pool_new(gpu.allocator);
	gpu.texture_pool = gpu_texture_pool_new(gpu.allocator);
	gpu.command_pool_pool = gpu_command_buffer_pool_pool_new(gpu.allocator);
	gpu.render_pipeline_pool = gpu_render_pipeline_pool_new(gpu.allocator);
	gpu.compute_pipeline_pool = gpu_compute_pipeline_pool_new(gpu.allocator);
	gpu.swapchain_pool = gpu_swapchain_pool_new(gpu.allocator);
	gpu.semaphore_pool = gpu_semaphore_pool_new(gpu.allocator);
	gpu.sampler_pool = gpu_sampler_pool_new(gpu.allocator);

	gpu.framebuffer_pool = gpu_framebuffer_pool_new(gpu.allocator);
	gpu.framebuffer_map = gpu_framebuffer_map_new(gpu.allocator, 64);
	gpu.framebuffer_lru = (Gpu_Framebuffer_LRU) {
		.oldest = SL_HANDLE_NULL,
		.newest = SL_HANDLE_NULL,
	};

	gpu.render_pass_object_pool = gpu_render_pass_object_pool_new(gpu.allocator);
	gpu.render_pass_object_map = gpu_render_pass_object_map_new(gpu.allocator, 64);
	gpu.render_pass_object_lru = (Gpu_Render_Pass_Object_LRU) {
		.oldest = SL_HANDLE_NULL,
		.newest = SL_HANDLE_NULL,
	};
}

void gpu_init(const Gpu_Desc* desc) {
	gpu = (Gpu) {};
	gpu.allocator = desc->allocator;
	gpu.swapchain_init_desc = desc->swapchain_desc;
	gpu.enqueue_mutex = sl_mutex_new();
	gpu_init_instance(desc);
	gpu_init_device();
	gpu_init_resource_pools();
	gpu.global_semaphore = gpu_new_semaphore();
}
void gpu_deinit() {
	gpu_log("Deinit");
	gpu_wait_cpu(gpu.global_semaphore, gpu.global_semaphore_value);
	gpu_destroy_semaphore(gpu.global_semaphore);
	vkDestroyInstance(gpu.instance, NULL);
	sl_mutex_destroy(&gpu.enqueue_mutex);
	gpu = (Gpu) {};
}

// Heap
Gpu_Heap gpu_new_heap(u64 bytes, Gpu_Memory_Type memory_type) {
	gpu_log("Allocating heap of size %lu.", bytes);

	Gpu_Heap heap_handle = gpu_heap_pool_acquire(&gpu.heap_pool);
	Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, heap_handle);

	VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = bytes,
		.memoryTypeIndex = gpu.memory_type_indices.index[memory_type],
	};

	VkResult allocate_result = vkAllocateMemory(gpu.device, &allocate_info, NULL, &heap_data->device_memory);
	sl_assert(allocate_result == VK_SUCCESS, "Failed to allocate memory.");

	heap_data->memory_type = memory_type;
	heap_data->size = bytes;

	VkBufferCreateInfo buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = bytes,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.pQueueFamilyIndices = &gpu.queue_family_indices.index[Gpu_Queue_Primary],
		.queueFamilyIndexCount = 1,
		.flags = 0,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	};
	VkResult create_buffer_result = vkCreateBuffer(gpu.device, &buffer_create_info, NULL, &heap_data->buffer);
	sl_assert(create_buffer_result == VK_SUCCESS, "Failed to create buffer.");

	VkResult bind_buffer_result = vkBindBufferMemory(gpu.device, heap_data->buffer, heap_data->device_memory, 0);
	sl_assert(bind_buffer_result == VK_SUCCESS, "Failed to bind buffer to heap allocation.");

	switch (memory_type) {
		case Gpu_Memory_Type_Host_Visible: {
			VkResult map_result = vkMapMemory(gpu.device, heap_data->device_memory, 0, VK_WHOLE_SIZE, 0, &heap_data->host_ptr);
			sl_assert(map_result == VK_SUCCESS, "Failed to map slice.");
		} break;

		case Gpu_Memory_Type_Device_Local: {
			heap_data->host_ptr = NULL;
		} break;

		case Gpu_Memory_Type_Count: {
			sl_abort("Invalid memory type.");
		} break;
	}

	return heap_handle;
}
void gpu_destroy_heap(Gpu_Heap heap) {
	Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, heap);

	switch (heap_data->memory_type) {
		case Gpu_Memory_Type_Host_Visible: {
			vkUnmapMemory(gpu.device, heap_data->device_memory);
		} break;

		case Gpu_Memory_Type_Device_Local: {
			// Do nothing
		} break;

		case Gpu_Memory_Type_Count: {
			sl_abort("Invalid memory type.");
		} break;
	}

	vkFreeMemory(gpu.device, heap_data->device_memory, NULL);

	gpu_heap_pool_release(&gpu.heap_pool, heap);
}
u64 gpu_get_heap_size(Gpu_Heap heap) {
	Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, heap);
	return heap_data->size;
}
Gpu_Slice gpu_get_heap_slice(Gpu_Heap heap) {
	return gpu_slice(heap, 0, gpu_get_heap_size(heap));
}

// Slice
void* gpu_get_slice_host_ptr(Gpu_Slice slice) {
	Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, slice.heap);
	sl_assert(heap_data->memory_type == Gpu_Memory_Type_Host_Visible, "Can only map host-visible memory.");
	return heap_data->host_ptr + slice.offset;
}
void gpu_flush_slice_to_gpu(Gpu_Slice slice) {
	Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, slice.heap);
	sl_assert(heap_data->memory_type == Gpu_Memory_Type_Host_Visible, "Flush only makes sense for host-visible memory.");

	const u64 aligned_offset = sl_round_down_u64(slice.offset, gpu.device_limits.nonCoherentAtomSize);
	const u64 aligned_size = sl_round_up_u64(slice.size + (slice.offset - aligned_offset), gpu.device_limits.nonCoherentAtomSize);

	VkMappedMemoryRange memory_range = {
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.offset = aligned_offset,
		.size = aligned_size,
		.memory = heap_data->device_memory
	};

	// To simplify the API, we wait for upload to complete before returning.
	// This is sub-optimal.
	VkResult flush_result = vkFlushMappedMemoryRanges(gpu.device, 1, &memory_range);
	sl_assert(flush_result == VK_SUCCESS, "Failed to flush slice upload.");
}
void gpu_flush_slice_from_gpu(Gpu_Slice slice) {
	Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, slice.heap);
	sl_assert(heap_data->memory_type == Gpu_Memory_Type_Host_Visible, "Flush only makes sense for host-visible memory.");

	const u64 aligned_offset = sl_round_down_u64(slice.offset, gpu.device_limits.nonCoherentAtomSize);
	const u64 aligned_size = sl_round_up_u64(slice.size + (slice.offset - aligned_offset), gpu.device_limits.nonCoherentAtomSize);

	VkMappedMemoryRange memory_range = {
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.offset = aligned_offset,
		.size = aligned_size,
		.memory = heap_data->device_memory
	};

	VkResult invalidate_result = vkInvalidateMappedMemoryRanges(gpu.device, 1, &memory_range);
	sl_assert(invalidate_result == VK_SUCCESS, "Failed to invalidate slice.");
}

// Sampler
Gpu_Sampler gpu_new_sampler(const Gpu_Sampler_Desc* desc) {
	gpu_log("Created sampler.");

	Gpu_Sampler result = gpu_sampler_pool_acquire(&gpu.sampler_pool);
	Gpu_Sampler_Data* data = gpu_sampler_pool_resolve(&gpu.sampler_pool, result);

	const VkSamplerCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.minFilter = gpu_filter_to_vk_filter(desc->min_filter),
		.magFilter = gpu_filter_to_vk_filter(desc->min_filter),
		.mipmapMode = gpu_filter_to_vk_sampler_mipmap_mode(desc->mip_filter),
		.addressModeU = gpu_sampler_address_mode_to_vk_sampler_address_mode(desc->address_mode_x),
		.addressModeV = gpu_sampler_address_mode_to_vk_sampler_address_mode(desc->address_mode_y),
		.addressModeW = gpu_sampler_address_mode_to_vk_sampler_address_mode(desc->address_mode_z),
		.unnormalizedCoordinates = (desc->coordinate == Gpu_Sampler_Coordinate_Pixel),
	};
	VkResult create_result = vkCreateSampler(gpu.device, &create_info, NULL, &data->sampler);
	if (create_result != VK_SUCCESS) {
		gpu_sampler_pool_release(&gpu.sampler_pool, result);
		return SL_HANDLE_NULL;
	}

	return result;
}
void gpu_destroy_sampler(Gpu_Sampler sampler) {
	Gpu_Sampler_Data* data = gpu_sampler_pool_resolve(&gpu.sampler_pool, sampler);
	vkDestroySampler(gpu.device, data->sampler, NULL);
	gpu_sampler_pool_release(&gpu.sampler_pool, sampler);
}

// Command Buffer
void gpu_init_command_buffer(Gpu_Command_Buffer_Data* command_buffer) {
	*command_buffer = (Gpu_Command_Buffer_Data) {
		.arena = sl_arena_allocator_new(gpu.allocator, 256 << 10),
		.commands = gpu_command_seq_new(gpu.allocator, 8),
		.semaphores = gpu_semaphore_seq_new(gpu.allocator, 1),
		.command_buffers = gpu_command_buffer_seq_new(gpu.allocator, 1),
		.swapchain_presents = gpu_swapchain_present_seq_new(gpu.allocator, 1),
	};

	VkCommandPoolCreateInfo command_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = gpu.queue_family_indices.index[Gpu_Queue_Primary],
	};
	VkResult command_pool_result = vkCreateCommandPool(gpu.device, &command_pool_create_info, NULL, &command_buffer->command_pool);
	sl_assert(command_pool_result == VK_SUCCESS, "Failed to create command pool.");
}

// Command Buffer Pool
Gpu_Command_Buffer_Pool gpu_new_command_buffer_pool(u32 size) {
	gpu_log("Creating command buffer pool of size %u.", size);
	Gpu_Command_Buffer_Pool pool = gpu_command_buffer_pool_pool_acquire(&gpu.command_pool_pool);
	Gpu_Command_Buffer_Pool_Data* pool_data = gpu_command_buffer_pool_pool_resolve(&gpu.command_pool_pool, pool);

	allocator_new(gpu.allocator, pool_data->command_buffers, size);
	pool_data->command_buffer_count = size;

	for (u32 i = 0; i < size; i++) {
		gpu_init_command_buffer(&pool_data->command_buffers[i]);
	}

	return pool;
}
void gpu_destroy_command_buffer_pool(Gpu_Command_Buffer_Pool pool) {
	// todo
}
Gpu_Command_Buffer_Data* gpu_resolve_command_buffer_data(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Pool_Data* pool_data = gpu_command_buffer_pool_pool_resolve(&gpu.command_pool_pool, cb.pool);
	if (pool_data == NULL) {
		return NULL;
	}
	if (cb.index > pool_data->command_buffer_count) {
		return NULL;
	}
	return &pool_data->command_buffers[cb.index];
}

bool gpu_new_command_buffer(Gpu_Command_Buffer_Pool pool, Gpu_Command_Buffer* out_cb) {
	Gpu_Command_Buffer_Pool_Data* pool_data = gpu_command_buffer_pool_pool_resolve(&gpu.command_pool_pool, pool);
	sl_assert(pool_data != NULL, "Pool is invalid.");

	Gpu_Command_Buffer cb_handle = {
		.pool = pool,
		.index = pool_data->next_command_buffer,
	};

	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb_handle);
	if (cb_data->state == Gpu_Command_Buffer_State_Recording) {
		// Command pool exceeded.
		return false;
	}

	// Wait for inflight work associated with the recorder to be completed.
	// Note: it is guaranteed that the on completion callback is processed prior to this function returning.
	gpu_wait_cpu(gpu.global_semaphore, cb_data->gpu_complete_semaphore_value);

	vkResetCommandPool(gpu.device, cb_data->command_pool, 0);

	cb_data->state = Gpu_Command_Buffer_State_Recording;
	sl_arena_allocator_reset(cb_data->arena, 0);
	cb_data->next_free_semaphore = 0;
	cb_data->next_free_command_buffer = 0;
	gpu_command_seq_clear(&cb_data->commands);
	gpu_swapchain_present_seq_clear(&cb_data->swapchain_presents);

	pool_data->next_command_buffer = (pool_data->next_command_buffer + 1) % pool_data->command_buffer_count;

	*out_cb = cb_handle;
	return true;
}

void gpu_on_complete_command_buffer_callback(void* ctx) {
	Gpu_Command_Buffer_Data* cb_data = ctx;

	const u32 present_count = gpu_swapchain_present_seq_get_count(&cb_data->swapchain_presents);
	for (u32 present_idx = 0; present_idx < present_count; present_idx++) {
		const Gpu_Swapchain_Present present_info = gpu_swapchain_present_seq_get(&cb_data->swapchain_presents, present_idx);
		gpu_release_swapchain_instance(present_info.swapchain_instance);
	}
}

VkSemaphore gpu_get_next_command_buffer_semaphore(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	if (cb_data->next_free_semaphore < gpu_semaphore_seq_get_count(&cb_data->semaphores)) {
		return gpu_semaphore_seq_get(&cb_data->semaphores, cb_data->next_free_semaphore++);
	} else {
		VkSemaphore* semaphore_ptr = gpu_semaphore_seq_push_reserve(&cb_data->semaphores);

		VkSemaphoreCreateInfo semaphore_create_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
		VkResult create_semaphore_result = vkCreateSemaphore(gpu.device, &semaphore_create_info, NULL, semaphore_ptr);
		sl_assert(create_semaphore_result == VK_SUCCESS, "Failed to create semaphore.");

		cb_data->next_free_semaphore++;
		return *semaphore_ptr;
	}
}

VkCommandBuffer gpu_get_next_vk_command_buffer(Gpu_Command_Buffer_Data* cb_data) {
	if (cb_data->next_free_command_buffer < gpu_command_buffer_seq_get_count(&cb_data->command_buffers)) {
		return gpu_command_buffer_seq_get(&cb_data->command_buffers, cb_data->next_free_command_buffer++);
	} else {
		VkCommandBuffer* command_buffer_ptr = gpu_command_buffer_seq_push_reserve(&cb_data->command_buffers);

		const VkCommandBufferAllocateInfo allocate_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = cb_data->command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		VkResult allocate_result = vkAllocateCommandBuffers(gpu.device, &allocate_info, command_buffer_ptr);
		sl_assert(allocate_result == VK_SUCCESS, "Failed to create command buffer.");

		cb_data->next_free_command_buffer++;
		return *command_buffer_ptr;
	}
}

sl_inline VkDescriptorType gpu_binding_kind_to_vk_descriptor_type(Gpu_Binding_Kind binding_kind) {
	switch (binding_kind) {
		case Gpu_Binding_Kind_Storage_Texture: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case Gpu_Binding_Kind_Sampled_Texture: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case Gpu_Binding_Kind_Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
		case Gpu_Binding_Kind_Slice: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	}
}

sl_inline void gpu_execute_transition_textures_to_layout(SL_Arena_Allocator* arena, VkCommandBuffer vk_cb, const Gpu_Texture* textures, const Gpu_Texture_Layout* layouts, u32 count) {
	if (count == 0) {
		return;
	}
	const u64 reset_position = sl_arena_allocator_get_position(arena);

	u32 next_barrier = 0;
	VkImageMemoryBarrier2* barriers;
	allocator_new(&arena->allocator, barriers, count);

	for (u32 texture_idx = 0; texture_idx < count; texture_idx++) {
		Gpu_Texture_Data* texture_data = gpu_texture_get_root_data(textures[texture_idx]);
		gpu_validate(texture_data != NULL, "Invalid texture.");

		Gpu_Texture_Layout old_layout = texture_data->imm.layout;
		Gpu_Texture_Layout new_layout = layouts[texture_idx];
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

			.oldLayout = gpu_texture_layout_to_vk_image_layout(old_layout),
			.newLayout = gpu_texture_layout_to_vk_image_layout(new_layout),

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

		gpu.device_function_table.vkCmdPipelineBarrier2KHR(vk_cb, &dep);
	}

	sl_arena_allocator_reset(arena, reset_position);
}

sl_inline void gpu_write_bindings(SL_Arena_Allocator* arena, VkCommandBuffer vk_cb, VkPipelineLayout pipeline_layout, const Gpu_Binding* bindings, u32 binding_count) {
	if (binding_count == 0) {
		return;
	}
	const u64 reset_position = sl_arena_allocator_get_position(arena);

	VkWriteDescriptorSet* writes;
	allocator_new(&arena->allocator, writes, binding_count);

	for (u32 binding_idx = 0; binding_idx < binding_count; binding_idx++) {
		VkWriteDescriptorSet* write = &writes[binding_idx];
		Gpu_Binding binding = bindings[binding_idx];
		switch (binding.kind) {
			case Gpu_Binding_Kind_Storage_Texture:
			case Gpu_Binding_Kind_Sampled_Texture: {
				Gpu_Texture_Data* texture_data = gpu_texture_pool_resolve(&gpu.texture_pool, binding.texture);

				VkDescriptorImageInfo* image_info;
				allocator_new(&arena->allocator, image_info, 1);
				*image_info = (VkDescriptorImageInfo) {
					.imageLayout = gpu_texture_layout_to_vk_image_layout(texture_data->imm.layout),
					.imageView = texture_data->imm.image_view,
					.sampler = VK_NULL_HANDLE,
				};

				*write = (VkWriteDescriptorSet) {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = VK_NULL_HANDLE,
					.dstBinding = binding.index,
					.descriptorType = gpu_binding_kind_to_vk_descriptor_type(binding.kind),
					.pImageInfo = image_info,
					.dstArrayElement = 0,
					.descriptorCount = 1,
				};
			} break;

			case Gpu_Binding_Kind_Sampler: {
				Gpu_Sampler_Data* sampler_data = gpu_sampler_pool_resolve(&gpu.sampler_pool, binding.sampler);

				VkDescriptorImageInfo* image_info;
				allocator_new(&arena->allocator, image_info, 1);
				*image_info = (VkDescriptorImageInfo) {
					.sampler = sampler_data->sampler,
				};

				*write = (VkWriteDescriptorSet) {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = VK_NULL_HANDLE,
					.dstBinding = binding.index,
					.descriptorType = gpu_binding_kind_to_vk_descriptor_type(binding.kind),
					.pImageInfo = image_info,
					.dstArrayElement = 0,
					.descriptorCount = 1,
				};
			} break;

			case Gpu_Binding_Kind_Slice: {
				Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, binding.slice.heap);

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
					.descriptorType = gpu_binding_kind_to_vk_descriptor_type(binding.kind),
					.pBufferInfo = buffer_info,
					.dstArrayElement = 0,
					.descriptorCount = 1,
				};
			} break;
		}
	}

	gpu.device_function_table.vkCmdPushDescriptorSetKHR(vk_cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, binding_count, writes);

	sl_arena_allocator_reset(arena, reset_position);
}

typedef struct Gpu_Command_Buffer_Emitter_Semaphore {
	VkSemaphore semaphore;
	u64 value;
} Gpu_Command_Buffer_Emitter_Semaphore;
sl_seq(Gpu_Command_Buffer_Emitter_Semaphore, Gpu_Command_Buffer_Emitter_Semaphore_Seq, gpu_command_buffer_emitter_semaphore_seq);

typedef struct Gpu_Command_Buffer_Emitter {
	Gpu_Command_Buffer_Data* cb_data;
	Gpu_Command_Buffer_Emitter_Semaphore_Seq wait;
	Gpu_Command_Buffer_Emitter_Semaphore_Seq signal;
	VkCommandBuffer cb;
} Gpu_Command_Buffer_Emitter;

void gpu_init_command_buffer_emitter(Gpu_Command_Buffer_Data* cb_data, Gpu_Command_Buffer_Emitter* emitter) {
	*emitter = (Gpu_Command_Buffer_Emitter) {
		.cb_data = cb_data,
		.wait = gpu_command_buffer_emitter_semaphore_seq_new(&cb_data->arena->allocator, 0),
		.signal = gpu_command_buffer_emitter_semaphore_seq_new(&cb_data->arena->allocator, 0),
		.cb = VK_NULL_HANDLE,
	};
}
void gpu_flush_command_buffer_emitter(Gpu_Command_Buffer_Emitter* emitter) {
	const u32 wait_count = gpu_command_buffer_emitter_semaphore_seq_get_count(&emitter->wait);
	const u32 signal_count = gpu_command_buffer_emitter_semaphore_seq_get_count(&emitter->signal);

	if ((emitter->cb == VK_NULL_HANDLE) && (wait_count == 0) && (signal_count == 0)) {
		return;
	}

	SL_Arena_Allocator* arena = emitter->cb_data->arena;
	const u64 arena_reset_position = sl_arena_allocator_get_position(arena);

	VkCommandBuffer cb;
	if (emitter->cb == VK_NULL_HANDLE) {
		cb = gpu_get_next_vk_command_buffer(emitter->cb_data);
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
		const Gpu_Command_Buffer_Emitter_Semaphore* semaphore = gpu_command_buffer_emitter_semaphore_seq_get_ptr(&emitter->wait, wait_idx);
		wait_semaphores[wait_idx] = semaphore->semaphore;
		wait_values[wait_idx] = semaphore->value;
		wait_stage_flags[wait_idx] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}

	for (u32 signal_idx = 0; signal_idx < signal_count; signal_idx++) {
		const Gpu_Command_Buffer_Emitter_Semaphore* semaphore = gpu_command_buffer_emitter_semaphore_seq_get_ptr(&emitter->signal, signal_idx);
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
	const VkResult submit_result = vkQueueSubmit(gpu.queue[Gpu_Queue_Primary], 1, &submit_info, VK_NULL_HANDLE);
	sl_assert(submit_result == VK_SUCCESS, "Failed to submit command buffer.");

	gpu_command_buffer_emitter_semaphore_seq_clear(&emitter->wait);
	gpu_command_buffer_emitter_semaphore_seq_clear(&emitter->signal);

	sl_arena_allocator_reset(arena, arena_reset_position);
}
VkCommandBuffer gpu_fetch_command_buffer_emitter(Gpu_Command_Buffer_Emitter* emitter) {
	const u32 signal_count = gpu_command_buffer_emitter_semaphore_seq_get_count(&emitter->signal);
	if ((emitter->cb != VK_NULL_HANDLE) && (signal_count > 0)) {
		gpu_flush_command_buffer_emitter(emitter);
	}

	if (emitter->cb == VK_NULL_HANDLE) {
		emitter->cb = gpu_get_next_vk_command_buffer(emitter->cb_data);
		const VkCommandBufferBeginInfo vk_cb_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		vkBeginCommandBuffer(emitter->cb, &vk_cb_begin_info);
	}

	return emitter->cb;
}
void gpu_add_wait_to_command_buffer_emitter(Gpu_Command_Buffer_Emitter* emitter, VkSemaphore semaphore, u64 value) {
	if (emitter->cb != VK_NULL_HANDLE) {
		// Flush, so all prior commands can execute before waiting on semaphore
		gpu_flush_command_buffer_emitter(emitter);
	}

	Gpu_Command_Buffer_Emitter_Semaphore entry = {
		.semaphore = semaphore,
		.value = value,
	};
	gpu_command_buffer_emitter_semaphore_seq_push(&emitter->wait, entry);
}
void gpu_add_signal_to_command_buffer_emitter(Gpu_Command_Buffer_Emitter* emitter, VkSemaphore semaphore, u64 value) {
	Gpu_Command_Buffer_Emitter_Semaphore entry = {
		.semaphore = semaphore,
		.value = value,
	};
	gpu_command_buffer_emitter_semaphore_seq_push(&emitter->signal, entry);
}

void gpu_enqueue(Gpu_Command_Buffer cb, bool wait_until_completed) {
	sl_mutex_lock(&gpu.enqueue_mutex);

	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");
	cb_data->state = Gpu_Command_Buffer_State_Enqueued;

	Gpu_Command_Buffer_Emitter cb_emitter;
	gpu_init_command_buffer_emitter(cb_data, &cb_emitter);

	// Wait on global semaphore
	{
		Gpu_Semaphore_Data* global_semaphore_data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, gpu.global_semaphore);
		gpu_add_wait_to_command_buffer_emitter(&cb_emitter, global_semaphore_data->vk_semaphore, gpu.global_semaphore_value);
	}

	const u64 command_count = gpu_command_seq_get_count(&cb_data->commands);
	for (u64 command_idx = 0; command_idx < command_count; command_idx++) {
		const Gpu_Command command = gpu_command_seq_get(&cb_data->commands, command_idx);

		switch (command.kind) {
			case Gpu_Command_Kind_Transition_Texture_Layouts: {
				VkCommandBuffer vk_cb = gpu_fetch_command_buffer_emitter(&cb_emitter);

				Gpu_Command_Transition_Texture_Layouts* transition_texture_layouts = command.data.transition_texture_layouts;
				gpu_execute_transition_textures_to_layout(cb_data->arena, vk_cb, transition_texture_layouts->textures, transition_texture_layouts->layouts, transition_texture_layouts->count);
			} break;

			case Gpu_Command_Kind_Begin_Render: {
				VkCommandBuffer vk_cb = gpu_fetch_command_buffer_emitter(&cb_emitter);

				Gpu_Command_Begin_Render* begin_render = command.data.begin_render;
				sl_assert(begin_render->layout.attachment_count > 0, "Must have at least one attachment in render pass.");

				// Validate and transition texture layouts
				for (u8 attachment_idx = 0; attachment_idx < begin_render->values.attachment_count; attachment_idx++) {
					const Gpu_Render_Pass_Layout_Attachment* att = &begin_render->layout.attachments[attachment_idx];
					Gpu_Texture texture = begin_render->values.attachments[attachment_idx].texture;
					Gpu_Texture_Data* texture_data = gpu_texture_get_root_data(texture);
					sl_assert((att->initial_layout == Gpu_Texture_Layout_Undefined) || (texture_data->imm.layout == att->initial_layout), "Initial layout must either be undefined, or match the current layout of the texture.");
					texture_data->imm.layout = att->final_layout;
				}

				// Acquire cached framebuffer
				Gpu_Framebuffer_Key fb_key = gpu_new_framebuffer_key(&begin_render->layout, &begin_render->values);
				Gpu_Framebuffer fb = gpu_acquire_framebuffer(fb_key);
				Gpu_Framebuffer_Data* fb_data = gpu_framebuffer_pool_resolve(&gpu.framebuffer_pool, fb);

				// Render pass
				VkClearValue clear_values[GPU_MAX_ATTACHMENTS];
				for (u8 attachment_idx = 0; attachment_idx < begin_render->values.attachment_count; attachment_idx++) {
					const Gpu_Render_Pass_Values_Attachment* att = &begin_render->values.attachments[attachment_idx];
					clear_values[attachment_idx].color = (VkClearColorValue) {
						.float32 = {
							att->clear_value.x,
							att->clear_value.y,
							att->clear_value.z,
							att->clear_value.w
						},
					};
				}

				const vec3_u32 render_size = gpu_get_texture_size(begin_render->values.attachments[0].texture);

				const Gpu_Render_Pass_Object_Data* render_pass_object_data = gpu_render_pass_object_pool_resolve(&gpu.render_pass_object_pool, fb_data->render_pass_object);

				VkRenderPassBeginInfo render_pass_begin_info = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = render_pass_object_data->render_pass,
					.framebuffer = fb_data->framebuffer,
					.renderArea = {
						.offset = { 0, 0 },
						.extent = { render_size.x, render_size.y },
					},
					.clearValueCount = sl_array_count(clear_values),
					.pClearValues = clear_values,
				};
				vkCmdBeginRenderPass(vk_cb, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

				const VkRect2D scissor = {
					.offset = { 0, 0 },
					.extent = { render_size.x, render_size.y },
				};
				vkCmdSetScissor(vk_cb, 0, 1, &scissor);

				const VkViewport viewport = {
					.x = 0.0f,
					.y = 0.0f,
					.width = (f32)render_size.x,
					.height = (f32)render_size.y,
					.minDepth = 0.0f,
					.maxDepth = 1.0f,
				};
				vkCmdSetViewport(vk_cb, 0, 1, &viewport);

			} break;

			case Gpu_Command_Kind_End_Render: {
				VkCommandBuffer vk_cb = gpu_fetch_command_buffer_emitter(&cb_emitter);

				vkCmdEndRenderPass(vk_cb);
			} break;

			case Gpu_Command_Kind_Draw: {
				VkCommandBuffer vk_cb = gpu_fetch_command_buffer_emitter(&cb_emitter);
				Gpu_Command_Draw* draw = command.data.draw;
				Gpu_Render_Pipeline_Data* pipeline_data = gpu_render_pipeline_pool_resolve(&gpu.render_pipeline_pool, draw->desc.pipeline);
				gpu_write_bindings(cb_data->arena, vk_cb, pipeline_data->pipeline_layout, draw->desc.bindings, draw->desc.binding_count);
				vkCmdBindPipeline(vk_cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_data->pipeline);
				vkCmdDraw(vk_cb, draw->desc.vertex_count, draw->desc.instance_count, 0, 0);
			} break;

			case Gpu_Command_Kind_Dispatch: {
				VkCommandBuffer vk_cb = gpu_fetch_command_buffer_emitter(&cb_emitter);

				Gpu_Command_Dispatch* dispatch = command.data.dispatch;
				Gpu_Compute_Pipeline_Data* pipeline_data = gpu_compute_pipeline_pool_resolve(&gpu.compute_pipeline_pool, dispatch->pipeline);
				gpu_write_bindings(cb_data->arena, vk_cb, pipeline_data->pipeline_layout, dispatch->bindings, dispatch->binding_count);
				vkCmdBindPipeline(vk_cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_data->pipeline);
				vkCmdDispatch(vk_cb, dispatch->group_count.x, dispatch->group_count.y, dispatch->group_count.z);
			} break;

			case Gpu_Command_Kind_Blit: {
				VkCommandBuffer vk_cb = gpu_fetch_command_buffer_emitter(&cb_emitter);

				Gpu_Command_Blit* blit = command.data.blit;
				const Gpu_Blit_Desc* blit_desc = &blit->desc;
				Gpu_Texture_Data* src_data = gpu_texture_get_root_data(blit_desc->src);
				Gpu_Texture_Data* dst_data = gpu_texture_get_root_data(blit_desc->dst);
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
				vkCmdBlitImage(vk_cb, src_data->imm.image, gpu_texture_layout_to_vk_image_layout(src_data->imm.layout), dst_data->imm.image, gpu_texture_layout_to_vk_image_layout(dst_data->imm.layout), 1, &region, VK_FILTER_NEAREST);
			} break;

			case Gpu_Command_Kind_Barrier: {
				VkCommandBuffer vk_cb = gpu_fetch_command_buffer_emitter(&cb_emitter);

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

				gpu.device_function_table.vkCmdPipelineBarrier2KHR(vk_cb, &dep);
			} break;

			case Gpu_Command_Kind_Wait: {
				Gpu_Command_Wait* wait = command.data.wait;
				gpu_add_wait_to_command_buffer_emitter(&cb_emitter, wait->semaphore, wait->value);
			} break;

			case Gpu_Command_Kind_Signal: {
				Gpu_Command_Signal* signal = command.data.signal;
				gpu_add_signal_to_command_buffer_emitter(&cb_emitter, signal->semaphore, signal->value);
			} break;
		}
	}

	const u32 present_count = gpu_swapchain_present_seq_get_count(&cb_data->swapchain_presents);

	// Transition all swapchain images for present
	if (present_count > 0) {
		VkCommandBuffer vk_cb = gpu_fetch_command_buffer_emitter(&cb_emitter);
		for (u32 present_idx = 0; present_idx < present_count; present_idx++) {
			const Gpu_Swapchain_Present present_info = gpu_swapchain_present_seq_get(&cb_data->swapchain_presents, present_idx);
			Gpu_Texture_Layout present_layout = Gpu_Texture_Layout_Present;
			gpu_execute_transition_textures_to_layout(cb_data->arena, vk_cb, &present_info.texture, &present_layout, 1);
		}
	}

	// Signal all swapchain semaphores
	for (u32 present_idx = 0; present_idx < present_count; present_idx++) {
		const Gpu_Swapchain_Present present_info = gpu_swapchain_present_seq_get(&cb_data->swapchain_presents, present_idx);
		gpu_add_signal_to_command_buffer_emitter(&cb_emitter, present_info.present_semaphore, 0);
	}

	// Signal global semaphore
	{
		Gpu_Semaphore_Data* global_semaphore_data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, gpu.global_semaphore);

		const u64 signal_value = ++gpu.global_semaphore_value;
		gpu_add_signal_to_command_buffer_emitter(&cb_emitter, global_semaphore_data->vk_semaphore, signal_value);
		cb_data->gpu_complete_semaphore_value = signal_value;
	}

	gpu_flush_command_buffer_emitter(&cb_emitter);

	// Presents
	if (present_count > 0) {
		VkSwapchainKHR* swapchains;
		VkSemaphore* wait_semaphores;
		u32* image_indices;
		allocator_new(&cb_data->arena->allocator, swapchains, present_count);
		allocator_new(&cb_data->arena->allocator, wait_semaphores, present_count);
		allocator_new(&cb_data->arena->allocator, image_indices, present_count);

		for (u32 present_idx = 0; present_idx < present_count; present_idx++) {
			const Gpu_Swapchain_Present present_info = gpu_swapchain_present_seq_get(&cb_data->swapchain_presents, present_idx);
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

		VkResult present_result = vkQueuePresentKHR(gpu.queue[Gpu_Queue_Present], &present_info);
		sl_assert((present_result == VK_SUCCESS) || (present_result == VK_ERROR_OUT_OF_DATE_KHR) || (present_result == VK_SUBOPTIMAL_KHR), "Failed to present command buffer.");
	}

	sl_mutex_unlock(&gpu.enqueue_mutex);

	// Clean up notification
	gpu_notify(gpu.global_semaphore, cb_data->gpu_complete_semaphore_value, cb_data, gpu_on_complete_command_buffer_callback);

	if (wait_until_completed) {
		gpu_wait_cpu(gpu.global_semaphore, cb_data->gpu_complete_semaphore_value);
	}
}

void gpu_transition_texture_layouts(Gpu_Command_Buffer cb, const Gpu_Texture* textures, const Gpu_Texture_Layout* layouts, u32 count) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Texture* textures_copy;
	Gpu_Texture_Layout* layouts_copy;
	allocator_new(&cb_data->arena->allocator, textures_copy, count);
	allocator_new(&cb_data->arena->allocator, layouts_copy, count);
	memcpy(textures_copy, textures, sizeof(Gpu_Texture) * count);
	memcpy(layouts_copy, layouts, sizeof(Gpu_Texture_Layout) * count);

	Gpu_Command_Transition_Texture_Layouts* transition_texture_layouts;
	allocator_new(&cb_data->arena->allocator, transition_texture_layouts, 1);
	*transition_texture_layouts = (Gpu_Command_Transition_Texture_Layouts) {
		.textures = textures_copy,
		.layouts = layouts_copy,
		.count = count,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Transition_Texture_Layouts,
		.data = {
			.transition_texture_layouts = transition_texture_layouts,
		},
	});
}

bool gpu_fetch_swapchain_texture(Gpu_Swapchain swapchain, Gpu_Command_Buffer cb, Gpu_Swapchain_Desc swapchain_desc, u64 timeout, Gpu_Texture* out_texture) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	VkSemaphore semaphore = gpu_get_next_command_buffer_semaphore(cb);

	u32 image_index;
	Gpu_Swapchain_Instance* swapchain_instance;
	while (true) {
		swapchain_instance = gpu_get_instance(swapchain, swapchain_desc);
		VkResult acquire_image_result = vkAcquireNextImageKHR(gpu.device, swapchain_instance->swapchain, timeout, semaphore, VK_NULL_HANDLE, &image_index);

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
	gpu_retain_swapchain_instance(swapchain_instance);

	Gpu_Texture swapchain_texture = swapchain_instance->textures[image_index];
	Gpu_Texture_Data* swapchain_texture_data = gpu_texture_pool_resolve(&gpu.texture_pool, swapchain_texture);

	// Always discard the original contents of swapchain texture.
	swapchain_texture_data->imm.layout = Gpu_Texture_Layout_Undefined;

	// Wait for swapchain image to be available.
	{
		Gpu_Command_Wait* wait;
		allocator_new(&cb_data->arena->allocator, wait, 1);
		*wait = (Gpu_Command_Wait) {
			.semaphore = semaphore,
			.value = 0,
		};

		gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
			.kind = Gpu_Command_Kind_Wait,
			.data = {
				.wait = wait,
			},
		});
	}

	// Automatically present image when command buffer completes
	gpu_swapchain_present_seq_push(&cb_data->swapchain_presents, (Gpu_Swapchain_Present) {
		.swapchain_instance = swapchain_instance,
		.present_semaphore = swapchain_instance->present_semaphores[image_index],
		.texture = swapchain_texture,
		.image_index = image_index,
		.image_available_semaphore = semaphore,
	});

	*out_texture = swapchain_texture;
	return true;
}

void gpu_begin_render(Gpu_Command_Buffer cb, const Gpu_Render_Pass_Layout* layout, const Gpu_Render_Pass_Values* values) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Command_Begin_Render* begin_render;
	allocator_new(&cb_data->arena->allocator, begin_render, 1);
	*begin_render = (Gpu_Command_Begin_Render) {
		.layout = *layout,
		.values = *values,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Begin_Render,
		.data = {
			.begin_render = begin_render,
		},
	});
}
void gpu_end_render(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_End_Render,
	});
}

bool gpu_new_pipeline_layout(const Gpu_Layout_Binding* bindings, u32 binding_count, VkShaderStageFlagBits stage_flags, VkPipelineLayout* out_layout) {
	VkDescriptorSetLayoutBinding* vk_bindings;
	allocator_new(gpu.allocator, vk_bindings, binding_count);

	for (u32 binding_idx = 0; binding_idx < binding_count; binding_idx++) {
		vk_bindings[binding_idx] = (VkDescriptorSetLayoutBinding) {
			.descriptorType = gpu_binding_kind_to_vk_descriptor_type(bindings[binding_idx].kind),
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
	VkResult create_set_layout_result = vkCreateDescriptorSetLayout(gpu.device, &set_layout_create_info, NULL, &set_layout);
	allocator_free(gpu.allocator, vk_bindings, binding_count);
	if (create_set_layout_result != VK_SUCCESS) {
		return false;
	}

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pSetLayouts = &set_layout,
		.setLayoutCount = 1,
	};

	VkPipelineLayout pipeline_layout;
	VkResult create_pipeline_layout_result = vkCreatePipelineLayout(gpu.device, &pipeline_layout_create_info, NULL, &pipeline_layout);
	if (create_pipeline_layout_result != VK_SUCCESS) {
		return false;
	}

	*out_layout = pipeline_layout;
	return true;
}

VkPrimitiveTopology gpu_primitive_kind_to_vk_primitive_topology(Gpu_Primitive_Kind primitive_kind) {
	switch (primitive_kind) {
		case Gpu_Primitive_Kind_Triangle: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

VkCullModeFlags gpu_cull_mode_to_vk_cull_mode_flags(Gpu_Cull_Mode cull_mode) {
	switch (cull_mode) {
		case Gpu_Cull_Mode_None: return VK_CULL_MODE_NONE;
		case Gpu_Cull_Mode_Front: return VK_CULL_MODE_FRONT_BIT;
		case Gpu_Cull_Mode_Back: return VK_CULL_MODE_BACK_BIT;
	}
}

Gpu_Render_Pipeline gpu_new_render_pipeline(const Gpu_Render_Pipeline_Desc* desc) {
	VkShaderModule vertex_module;
	{
		const VkShaderModuleCreateInfo module_create_info = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pCode = desc->vertex_code.code,
			.codeSize = desc->vertex_code.size,
		};
		VkResult create_module_result = vkCreateShaderModule(gpu.device, &module_create_info, NULL, &vertex_module);
		if (create_module_result != VK_SUCCESS) {
			gpu_log("Failed to create render pipeline (VkShaderModule).");
			return SL_HANDLE_NULL;
		}
	}

	VkShaderModule fragment_module;
	{
		const VkShaderModuleCreateInfo module_create_info = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pCode = desc->fragment_code.code,
			.codeSize = desc->fragment_code.size,
		};
		VkResult create_module_result = vkCreateShaderModule(gpu.device, &module_create_info, NULL, &fragment_module);
		if (create_module_result != VK_SUCCESS) {
			gpu_log("Failed to create render pipeline (VkShaderModule).");
			vkDestroyShaderModule(gpu.device, vertex_module, NULL);
			return SL_HANDLE_NULL;
		}
	}

	Gpu_Render_Pipeline pipeline = gpu_render_pipeline_pool_acquire(&gpu.render_pipeline_pool);
	Gpu_Render_Pipeline_Data* pipeline_data = gpu_render_pipeline_pool_resolve(&gpu.render_pipeline_pool, pipeline);

	bool create_layout_result = gpu_new_pipeline_layout(desc->bindings, desc->binding_count, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &pipeline_data->pipeline_layout);
	if (!create_layout_result) {
		gpu_log("Failed to create compute pipeline (VkPipelineLayout).");
		vkDestroyShaderModule(gpu.device, vertex_module, NULL);
		vkDestroyShaderModule(gpu.device, fragment_module, NULL);
		gpu_render_pipeline_pool_release(&gpu.render_pipeline_pool, pipeline);
		return SL_HANDLE_NULL;
	}

	const VkPipelineShaderStageCreateInfo stages[2] = {
		{
		    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		    .stage = VK_SHADER_STAGE_VERTEX_BIT,
		    .module = vertex_module,
		    .pName = desc->vertex_entry_point,
		},
		{
		    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		    .module = fragment_module,
		    .pName = desc->fragment_entry_point,
		}
	};

	const VkPipelineVertexInputStateCreateInfo vertex_input = {
    	.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	const VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = gpu_primitive_kind_to_vk_primitive_topology(desc->primitive_kind),
	};

	const VkPipelineViewportStateCreateInfo viewport_state = {
    	.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    	.viewportCount = 1,
    	.scissorCount = 1
	};

	const VkPipelineRasterizationStateCreateInfo rasterization_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	    .polygonMode = VK_POLYGON_MODE_FILL,
	    .cullMode = gpu_cull_mode_to_vk_cull_mode_flags(desc->cull_mode),
	    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	    .lineWidth = 1.0f,
	};

	const VkPipelineMultisampleStateCreateInfo msaa_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};

	VkPipelineColorBlendAttachmentState blend_attachments[GPU_MAX_ATTACHMENTS];

	// TODO
	blend_attachments[0] = (VkPipelineColorBlendAttachmentState) {
		.colorWriteMask = 0xF,
	};

	const VkPipelineColorBlendStateCreateInfo blend_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	    .attachmentCount = 1, // TODO
	    .pAttachments = blend_attachments
	};

	const VkDynamicState dynamic_states[] = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR
	};

	const VkPipelineDynamicStateCreateInfo dynamic_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	    .dynamicStateCount = 2,
	    .pDynamicStates = dynamic_states
	};

	const Gpu_Render_Pass_Object_Key render_pass_object_key = {
		.layout = desc->render_pass_layout,
	};
	pipeline_data->render_pass_object = gpu_acquire_render_pass_object(render_pass_object_key);
	const Gpu_Render_Pass_Object_Data* render_pass_object_data = gpu_render_pass_object_pool_resolve(&gpu.render_pass_object_pool, pipeline_data->render_pass_object);

	const VkGraphicsPipelineCreateInfo pipe = {
	    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    .stageCount = 2,
	    .pStages = stages,
	    .pVertexInputState = &vertex_input,
	    .pInputAssemblyState = &input_assembly,
	    .pViewportState = &viewport_state,
	    .pRasterizationState = &rasterization_state,
	    .pMultisampleState = &msaa_state,
	    .pColorBlendState = &blend_state,
	    .pDynamicState = &dynamic_state,
	    .layout = pipeline_data->pipeline_layout,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.renderPass = render_pass_object_data->render_pass,
	};
	VkResult create_pipeline_result = vkCreateGraphicsPipelines(gpu.device, VK_NULL_HANDLE, 1, &pipe, NULL, &pipeline_data->pipeline);
	vkDestroyShaderModule(gpu.device, vertex_module, NULL);
	vkDestroyShaderModule(gpu.device, fragment_module, NULL);

	if (create_pipeline_result != VK_SUCCESS) {
		vkDestroyPipelineLayout(gpu.device, pipeline_data->pipeline_layout, NULL);
		gpu_release_render_pass_object(pipeline_data->render_pass_object);
		gpu_render_pipeline_pool_release(&gpu.render_pipeline_pool, pipeline);
		return SL_HANDLE_NULL;
	}

	return pipeline;
}

Gpu_Compute_Pipeline gpu_new_compute_pipeline(const Gpu_Compute_Pipeline_Desc* desc) {
	if (desc->code.size == 0) {
		return SL_HANDLE_NULL;
	}

	Gpu_Compute_Pipeline pipeline = gpu_compute_pipeline_pool_acquire(&gpu.compute_pipeline_pool);
	Gpu_Compute_Pipeline_Data* pipeline_data = gpu_compute_pipeline_pool_resolve(&gpu.compute_pipeline_pool, pipeline);

	VkShaderModuleCreateInfo module_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pCode = desc->code.code,
		.codeSize = desc->code.size,
	};
	VkResult create_module_result = vkCreateShaderModule(gpu.device, &module_create_info, NULL, &pipeline_data->shader_module);
	if (create_module_result != VK_SUCCESS) {
		gpu_log("Failed to create compute pipeline (VkShaderModule).");
		gpu_compute_pipeline_pool_release(&gpu.compute_pipeline_pool, pipeline);
		return SL_HANDLE_NULL;
	}

	bool create_layout_result = gpu_new_pipeline_layout(desc->bindings, desc->binding_count, VK_SHADER_STAGE_COMPUTE_BIT, &pipeline_data->pipeline_layout);
	if (!create_layout_result) {
		gpu_log("Failed to create compute pipeline (VkPipelineLayout).");
		vkDestroyShaderModule(gpu.device, pipeline_data->shader_module, NULL);
		gpu_compute_pipeline_pool_release(&gpu.compute_pipeline_pool, pipeline);
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
	VkResult create_pipeline_result = vkCreateComputePipelines(gpu.device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline_data->pipeline);
	if (create_pipeline_result != VK_SUCCESS) {
		gpu_log("Failed to create compute pipeline (VkPipeline).");
		vkDestroyShaderModule(gpu.device, pipeline_data->shader_module, NULL);
		vkDestroyPipelineLayout(gpu.device, pipeline_data->pipeline_layout, NULL);
		gpu_compute_pipeline_pool_release(&gpu.compute_pipeline_pool, pipeline);
		return SL_HANDLE_NULL;
	}

	gpu_log("Created compute pipeline.");
	return pipeline;
}

void gpu_draw(Gpu_Command_Buffer cb, const Gpu_Draw_Desc* desc) {
	gpu_validate(gpu_compute_pipeline_pool_resolve(&gpu.compute_pipeline_pool, desc->pipeline), "Invalid pipeline.");

	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	gpu_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Command_Draw* draw;
	allocator_new(&cb_data->arena->allocator, draw, 1);
	*draw = (Gpu_Command_Draw) {
		.desc = *desc,
	};

	allocator_new(&cb_data->arena->allocator, draw->desc.bindings, desc->binding_count);
	memcpy((void*)draw->desc.bindings, desc->bindings, sizeof(Gpu_Binding) * desc->binding_count);

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Draw,
		.data.draw = draw,
	});
}

void gpu_dispatch(Gpu_Command_Buffer cb, Gpu_Compute_Pipeline pipeline, const Gpu_Binding* bindings, u32 binding_count, vec3_u32 group_count) {
	gpu_validate(gpu_compute_pipeline_pool_resolve(&gpu.compute_pipeline_pool, pipeline), "Invalid pipeline.");

	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	gpu_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Binding* bindings_copy;
	allocator_new(&cb_data->arena->allocator, bindings_copy, binding_count);
	memcpy(bindings_copy, bindings, sizeof(Gpu_Binding) * binding_count);

	Gpu_Command_Dispatch* dispatch;
	allocator_new(&cb_data->arena->allocator, dispatch, 1);
	*dispatch = (Gpu_Command_Dispatch) {
		.pipeline = pipeline,
		.bindings = bindings_copy,
		.binding_count = binding_count,
		.group_count = group_count,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Dispatch,
		.data.dispatch = dispatch,
	});
}

void gpu_blit(Gpu_Command_Buffer cb, const Gpu_Blit_Desc* desc) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	gpu_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Command_Blit* blit;
	allocator_new(&cb_data->arena->allocator, blit, 1);
	*blit = (Gpu_Command_Blit) {
		.desc = *desc,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Blit,
		.data.blit = blit,
	});
}

void gpu_barrier(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	gpu_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Barrier,
	});
}

void gpu_semaphore_flush_pending_callbacks(Gpu_Semaphore_Data* data) {
	for (u32 callback_idx = 0; callback_idx < gpu_semaphore_on_notify_callback_seq_get_count(&data->pending_callbacks);) {
		Gpu_Semaphore_On_Notify_Callback callback = gpu_semaphore_on_notify_callback_seq_get(&data->pending_callbacks, callback_idx);
		if (callback.value <= data->current_value) {
			callback.fn(callback.ctx);
			gpu_semaphore_on_notify_callback_seq_remove(&data->pending_callbacks, callback_idx);
		} else {
			callback_idx++;
		}
	}
}

void* gpu_semaphore_thread(void* ctx) {
	Gpu_Semaphore_Data* data = ctx;

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
		VkResult wait_result = vkWaitSemaphores(gpu.device, &wait_info, u64_max);
		sl_assert(wait_result == VK_SUCCESS, "Failed to wait on semaphore.");

		VkResult counter_result = vkGetSemaphoreCounterValue(gpu.device, data->vk_semaphore, &wait_value);
		sl_assert(counter_result == VK_SUCCESS, "Failed to get current semaphore value.");

		sl_mutex_lock(&data->mutex);
		data->current_value = sl_max(data->current_value, wait_value);

		gpu_semaphore_flush_pending_callbacks(data);

		if (data->destroying && (wait_value >= data->current_value)) {
			sl_mutex_unlock(&data->mutex);
			break;
		}
		sl_mutex_unlock(&data->mutex);
	}

	return NULL;
}

Gpu_Semaphore gpu_new_semaphore(void) {
	Gpu_Semaphore result = gpu_semaphore_pool_acquire(&gpu.semaphore_pool);
	Gpu_Semaphore_Data* result_data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, result);

	VkSemaphoreTypeCreateInfo semaphore_type_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = 0,
	};
	VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &semaphore_type_create_info,
	};
	VkResult create_semaphore_result = vkCreateSemaphore(gpu.device, &semaphore_create_info, NULL, &result_data->vk_semaphore);
	sl_assert(create_semaphore_result == VK_SUCCESS, "Failed to create timeline semaphore");

	result_data->current_value = 0;
	result_data->mutex = sl_mutex_new();
	result_data->thread = sl_thread_new(result_data, gpu_semaphore_thread);
	result_data->pending_callbacks = gpu_semaphore_on_notify_callback_seq_new(gpu.allocator, 0);

	return result;
}

void gpu_destroy_semaphore(Gpu_Semaphore semaphore) {
	Gpu_Semaphore_Data* data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, semaphore);

	sl_mutex_lock(&data->mutex);
	sl_assert(gpu_semaphore_on_notify_callback_seq_get_count(&data->pending_callbacks) == 0, "Can't destroy a semaphore that has pending notifications (i.e. gpu_notify()).");
	data->destroying = true;
	sl_mutex_unlock(&data->mutex);

	// in order to clean up the thread, we must signal the semaphore.
	gpu_signal_cpu(semaphore, u64_max);

	sl_thread_join(&data->thread);
	sl_mutex_destroy(&data->mutex);

	gpu_semaphore_on_notify_callback_seq_destroy(&data->pending_callbacks);
	gpu_semaphore_pool_release(&gpu.semaphore_pool, semaphore);
}

void gpu_notify(Gpu_Semaphore semaphore, u64 value, void* ctx, Gpu_On_Notify_Fn fn) {
	Gpu_Semaphore_Data* data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, semaphore);
	sl_mutex_lock(&data->mutex);
	const bool pending_notify = (value > data->current_value);
	if (pending_notify) {
		Gpu_Semaphore_On_Notify_Callback callback = {
			.ctx = ctx,
			.value = value,
			.fn = fn,
		};
		gpu_semaphore_on_notify_callback_seq_push(&data->pending_callbacks, callback);
	}
	sl_mutex_unlock(&data->mutex);

	if (!pending_notify) {
		fn(ctx);
	}
}

void gpu_wait_gpu(Gpu_Command_Buffer cb, Gpu_Semaphore semaphore, u64 value) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Semaphore_Data* semaphore_data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, semaphore);
	sl_assert(semaphore_data != NULL, "Invalid semaphore");

	Gpu_Command_Wait* wait;
	allocator_new(&cb_data->arena->allocator, wait, 1);
	*wait = (Gpu_Command_Wait) {
		.semaphore = semaphore_data->vk_semaphore,
		.value = value,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Wait,
		.data = {
			.wait = wait,
		},
	});
}
void gpu_signal_gpu(Gpu_Command_Buffer cb, Gpu_Semaphore semaphore, u64 value) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Semaphore_Data* semaphore_data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, semaphore);
	sl_assert(semaphore_data != NULL, "Invalid semaphore");

	Gpu_Command_Signal* signal;
	allocator_new(&cb_data->arena->allocator, signal, 1);
	*signal = (Gpu_Command_Signal) {
		.semaphore = semaphore_data->vk_semaphore,
		.value = value,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Signal,
		.data = {
			.signal = signal,
		},
	});
}

void gpu_signal_cpu(Gpu_Semaphore semaphore, u64 value) {
	Gpu_Semaphore_Data* data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, semaphore);
	sl_assert(data, "Invalid semaphore.");

	VkSemaphoreSignalInfo signal_info = {
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
	    .pNext = NULL,
	    .semaphore = data->vk_semaphore,
	    .value = value,
	};
	VkResult result = vkSignalSemaphore(gpu.device, &signal_info);
	sl_assert(result == VK_SUCCESS, "Failed to signal semaphore.");

	sl_mutex_lock(&data->mutex);
	data->current_value = sl_max(data->current_value, value);
	gpu_semaphore_flush_pending_callbacks(data);
	sl_mutex_unlock(&data->mutex);
}
void gpu_wait_cpu(Gpu_Semaphore semaphore, u64 value) {
	Gpu_Semaphore_Data* data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, semaphore);
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
		VkResult result = vkWaitSemaphores(gpu.device, &wait_info, u64_max);
		sl_assert(result == VK_SUCCESS, "Failed to wait on semaphore.");

		sl_mutex_lock(&data->mutex);
		data->current_value = sl_max(data->current_value, value);
		gpu_semaphore_flush_pending_callbacks(data);
		sl_mutex_unlock(&data->mutex);
	}
}
