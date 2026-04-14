#pragma once

#include <vulkan/vulkan.h>
#include "core.h"

#define GPU_VK_MAX_ATTACHMENTS 8

typedef SL_Handle Gpu_Vk_Heap;
typedef SL_Handle Gpu_Vk_Command_Buffer_Pool;
typedef SL_Handle Gpu_Vk_Texture;
typedef SL_Handle Gpu_Vk_Compute_Pipeline;

typedef struct Gpu_Vk_Swapchain_Desc {
	vec2_u32 size;
	// format
	// colorspace
} Gpu_Vk_Swapchain_Desc;

typedef struct Gpu_Vk_Command_Buffer {
	Gpu_Vk_Command_Buffer_Pool pool;
	u32 index;
} Gpu_Vk_Command_Buffer;

typedef enum Gpu_Vk_Load_Op {
	Gpu_Vk_Load_Op_Load,
	Gpu_Vk_Load_Op_Clear,
	Gpu_Vk_Load_Op_Dont_Care
} Gpu_Vk_Load_Op;

typedef enum Gpu_Vk_Store_Op {
	Gpu_Vk_Store_Op_Store,
	Gpu_Vk_Store_Op_Dont_Care
} Gpu_Vk_Store_Op;

typedef struct Gpu_Vk_Render_Attachment {
	Gpu_Vk_Texture texture;
	vec4_f32 clear_value;
	Gpu_Vk_Load_Op load_op;
	Gpu_Vk_Store_Op store_op;
} Gpu_Vk_Render_Attachment;

typedef struct Gpu_Vk_Render_Pass {
	Gpu_Vk_Render_Attachment attachments[GPU_VK_MAX_ATTACHMENTS];
	u8 attachment_count;
} Gpu_Vk_Render_Pass;

typedef struct Gpu_Vk_Slice {
	Gpu_Vk_Heap heap;
	u64 offset;
	u64 size;
} Gpu_Vk_Slice;
#define GPU_SLICE_NULL ((Gpu_Vk_Slice) {})
sl_inline bool gpu_vk_slice_is_null(Gpu_Vk_Slice slice) {
	return slice.heap.index == 0;
}
sl_inline Gpu_Vk_Slice gpu_vk_subslice(Gpu_Vk_Slice slice, u64 offset, u64 size) {
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
void* gpu_vk_get_slice_host_ptr(Gpu_Vk_Slice slice);
void gpu_vk_flush_slice(Gpu_Vk_Slice slice);

typedef enum Gpu_Vk_Memory_Type {
	Gpu_Vk_Memory_Type_Host_Visible,
	Gpu_Vk_Memory_Type_Device_Local,
	Gpu_Vk_Memory_Type_Count
} Gpu_Vk_Memory_Type;

typedef struct Gpu_Vk_Code {
	const u32* code;
	u64 size;
} Gpu_Vk_Code;

typedef VkSurfaceKHR (*Gpu_Vk_Get_Surface_Fn)(void* ctx, VkInstance instance);
typedef VkExtent2D (*Gpu_Vk_Get_Swapchain_Extent_Fn)(void* ctx);

typedef struct Gpu_Vk_Swapchain_Init_Desc {
	void* ctx;
	Gpu_Vk_Get_Surface_Fn get_surface_fn;
	Gpu_Vk_Get_Swapchain_Extent_Fn get_swapchain_extent_fn;
} Gpu_Vk_Swapchain_Init_Desc;

typedef struct Gpu_Vk_Desc {
	Allocator* allocator;

	u32 required_extension_count;
	char const* const* required_extensions;

	Gpu_Vk_Swapchain_Init_Desc swapchain_desc;
} Gpu_Vk_Desc;

void gpu_vk_init(const Gpu_Vk_Desc* desc);
void gpu_vk_deinit();

// Texture
vec2_u16 gpu_vk_get_texture_size(Gpu_Vk_Texture texture);

// Heap
Gpu_Vk_Heap gpu_vk_new_heap(u64 bytes, Gpu_Vk_Memory_Type memory_type);
void gpu_vk_destroy_heap(Gpu_Vk_Heap heap);
u64 gpu_vk_get_heap_size(Gpu_Vk_Heap heap);

// Swapchain
Gpu_Vk_Texture gpu_vk_fetch_swapchain_texture(Gpu_Vk_Command_Buffer cb, Gpu_Vk_Swapchain_Desc swapchain_desc);
void gpu_vk_present_swapchain_texture(Gpu_Vk_Command_Buffer cb, Gpu_Vk_Texture swapchain_texture);

// Command Buffer Pool
Gpu_Vk_Command_Buffer_Pool gpu_vk_new_command_buffer_pool(u32 size);
void gpu_vk_destroy_command_buffer_pool(Gpu_Vk_Command_Buffer_Pool pool);

// Command Buffer
bool gpu_vk_new_command_buffer(Gpu_Vk_Command_Buffer_Pool pool, Gpu_Vk_Command_Buffer* out_cb);
void gpu_vk_enqueue(Gpu_Vk_Command_Buffer cb, bool wait_until_completed);

// Render
void gpu_vk_begin_render(Gpu_Vk_Command_Buffer cb, const Gpu_Vk_Render_Pass* render_pass);
void gpu_vk_end_render(Gpu_Vk_Command_Buffer cb);
// void gpu_vk_draw(Gpu_Vk_Command_Buffer cb);

// Compute
typedef struct Gpu_Vk_Compute_Pipeline_Desc {
	Gpu_Vk_Code code;
	const char* entry_point;
} Gpu_Vk_Compute_Pipeline_Desc;
Gpu_Vk_Compute_Pipeline gpu_vk_new_compute_pipeline(const Gpu_Vk_Compute_Pipeline_Desc* desc);

void gpu_vk_dispatch(Gpu_Vk_Command_Buffer cb);
