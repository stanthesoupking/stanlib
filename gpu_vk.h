#pragma once

#include <vulkan/vulkan.h>

typedef SL_Handle Gpu_Vk_Heap;
typedef SL_Handle Gpu_Vk_Command_Buffer;
typedef SL_Handle Gpu_Vk_Texture;

typedef struct Gpu_Vk_Swapchain_Image {
	u32 image_index;
	Gpu_Vk_Texture texture;
} Gpu_Vk_Swapchain_Image;

typedef struct Gpu_Vk_Slice {
	Gpu_Vk_Heap heap;
	u64 offset;
	u64 size;
} Gpu_Vk_Slice;
#define GPU_SLICE_NULL ((Gpu_Vk_Slice) {})
sl_inline bool gpu_vk_slice_is_null(Gpu_Vk_Slice slice) {
	return slice.heap.index == 0;
}
sl_inline Gpu_Vk_Slice gpu_vk_slice_subrange(Gpu_Vk_Slice slice, u64 offset, u64 size) {
	return (Gpu_Vk_Slice) {
		.heap = slice.heap,
		.offset = slice.offset + offset,
		.size = size,
	};
}
sl_inline Gpu_Vk_Slice gpu_vk_slice(Gpu_Vk_Heap heap, u64 offset, u64 size) {
	return (Gpu_Vk_Slice) {
		.heap = heap,
		.offset = offset,
		.size = size,
	};
}

void* gpu_vk_get_host_ptr(Gpu_Vk_Slice slice);
void gpu_vk_slice_flush(Gpu_Vk_Slice slice);

typedef enum Gpu_Vk_Memory_Type {
	Gpu_Vk_Memory_Type_Host_Visible,
	Gpu_Vk_Memory_Type_Device_Local,
	Gpu_Vk_Memory_Type_Count
} Gpu_Vk_Memory_Type;

typedef VkSurfaceKHR (*Gpu_Vk_Get_Surface_Fn)(void* ctx, VkInstance instance);
typedef VkExtent2D (*Gpu_Vk_Get_Swapchain_Extent_Fn)(void* ctx);

typedef struct Gpu_Vk_Swapchain_Desc {
	void* ctx;
	Gpu_Vk_Get_Surface_Fn get_surface_fn;
	Gpu_Vk_Get_Swapchain_Extent_Fn get_swapchain_extent_fn;
} Gpu_Vk_Swapchain_Desc;

typedef struct Gpu_Vk_Desc {
	Allocator* allocator;

	u32 required_extension_count;
	char const* const* required_extensions;

	Gpu_Vk_Swapchain_Desc swapchain_desc;
} Gpu_Vk_Desc;

void gpu_vk_init(const Gpu_Vk_Desc* desc);
void gpu_vk_deinit();

// Heap
Gpu_Vk_Heap gpu_vk_heap_new(u64 bytes, Gpu_Vk_Memory_Type memory_type);
void gpu_vk_heap_destroy(Gpu_Vk_Heap heap);
u64 gpu_vk_heap_get_size(Gpu_Vk_Heap heap);

// Swapchain
Gpu_Vk_Swapchain_Image gpu_vk_get_next_swapchain_image(Gpu_Vk_Command_Buffer command_buffer);
void gpu_vk_present_swapchain_image(Gpu_Vk_Command_Buffer command_buffer, Gpu_Vk_Swapchain_Image swapchain_texture);

// Command Buffer
Gpu_Vk_Command_Buffer gpu_vk_command_buffer_new();
void gpu_vk_command_buffer_submit(Gpu_Vk_Command_Buffer command_buffer);

void gpu_vk_dummy_render();