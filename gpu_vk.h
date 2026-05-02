#pragma once

#include <vulkan/vulkan.h>
#include "core.h"

#define GPU_VK_MAX_ATTACHMENTS 8

typedef SL_Handle Gpu_Vk_Heap;
typedef SL_Handle Gpu_Vk_Command_Buffer_Pool;
typedef SL_Handle Gpu_Vk_Texture;
typedef SL_Handle Gpu_Vk_Sampler;
typedef SL_Handle Gpu_Vk_Compute_Pipeline;
typedef SL_Handle Gpu_Vk_Swapchain;
typedef SL_Handle Gpu_Vk_Semaphore;

typedef enum Gpu_Vk_Format {
	Gpu_Vk_Format_RGBA8_Unorm,
	Gpu_Vk_Format_RGBA8_sRGB,
	Gpu_Vk_Format_BGRA8_Unorm,
	Gpu_Vk_Format_BGRA8_sRGB,
	Gpu_Vk_Format_RGBA16_Float,
	Gpu_Vk_Format_RGBA32_Float,
} Gpu_Vk_Format;

typedef struct Gpu_Vk_Size_And_Align {
	u64 size;
	u64 align;
} Gpu_Vk_Size_And_Align;
#define gpu_vk_size_and_align_of_type(x) ((Gpu_Vk_Size_And_Align) { .size = sizeof(x), .align = sl_align_of(x) })

typedef enum Gpu_Vk_Texture_Kind {
	Gpu_Vk_Texture_Kind_1D,
	Gpu_Vk_Texture_Kind_2D,
	Gpu_Vk_Texture_Kind_3D,
} Gpu_Vk_Texture_Kind;

typedef enum Gpu_Vk_Texture_Usage {
	Gpu_Vk_Texture_Usage_None = 0,
	Gpu_Vk_Texture_Usage_Shader_Read = 1 << 0,
	Gpu_Vk_Texture_Usage_Shader_Write = 1 << 1,
	Gpu_Vk_Texture_Usage_Render_Attachment = 1 << 2,
} Gpu_Vk_Texture_Usage;

typedef enum Gpu_Vk_Texture_Layout {
	Gpu_Vk_Texture_Layout_Undefined,
	Gpu_Vk_Texture_Layout_General,
	Gpu_Vk_Texture_Layout_Shader_Read,
	Gpu_Vk_Texture_Layout_Shader_Write,
	Gpu_Vk_Texture_Layout_Color_Attachment,
	Gpu_Vk_Texture_Layout_Present,
} Gpu_Vk_Texture_Layout;

typedef enum Gpu_Vk_Colorspace {
	Gpu_Vk_Colorspace_sRGB,
	Gpu_Vk_Colorspace_Extended_sRGB_Linear,
} Gpu_Vk_Colorspace;

typedef struct Gpu_Vk_Swapchain_Desc {
    vec2_u32 size;
	Gpu_Vk_Format format;
	Gpu_Vk_Colorspace colorspace;
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
sl_inline bool gpu_vk_slice_suballocate(Gpu_Vk_Slice basis, Gpu_Vk_Size_And_Align size_and_align, Gpu_Vk_Slice* out_allocation, Gpu_Vk_Slice* out_remainder) {
	const u64 aligned_offset = sl_round_up_u64(basis.offset, size_and_align.align);
	if (aligned_offset + size_and_align.size > basis.size) {
		return false;
	}

	*out_allocation = (Gpu_Vk_Slice) {
		.heap = basis.heap,
		.offset = aligned_offset,
		.size = size_and_align.size,
	};
	*out_remainder = (Gpu_Vk_Slice) {
		.heap = basis.heap,
		.offset = aligned_offset + size_and_align.size,
		.size = basis.size - (basis.offset - aligned_offset + size_and_align.size),
	};
	return true;
}

void* gpu_vk_get_slice_host_ptr(Gpu_Vk_Slice slice);
void gpu_vk_flush_slice_to_gpu(Gpu_Vk_Slice slice);
void gpu_vk_flush_slice_from_gpu(Gpu_Vk_Slice slice);

typedef enum Gpu_Vk_Binding_Kind {
	Gpu_Vk_Binding_Kind_Storage_Texture,
	Gpu_Vk_Binding_Kind_Sampled_Texture, // todo
	Gpu_Vk_Binding_Kind_Slice,
} Gpu_Vk_Binding_Kind;

typedef struct Gpu_Vk_Binding_Storage_Texture {
	Gpu_Vk_Texture texture;
} Gpu_Vk_Binding_Storage_Texture;

typedef struct Gpu_Vk_Binding_Sampled_Texture {
	Gpu_Vk_Texture texture;
	Gpu_Vk_Sampler sampler;
} Gpu_Vk_Binding_Sampled_Texture;

typedef struct Gpu_Vk_Binding {
	Gpu_Vk_Binding_Kind kind;
	union {
		Gpu_Vk_Binding_Storage_Texture storage_texture;
		Gpu_Vk_Binding_Sampled_Texture sampled_texture;
		Gpu_Vk_Slice slice;
	};
	u32 index;
} Gpu_Vk_Binding;

typedef struct Gpu_Vk_Layout_Binding {
	Gpu_Vk_Binding_Kind kind;
	u32 index;
} Gpu_Vk_Layout_Binding;

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
} Gpu_Vk_Swapchain_Init_Desc;

typedef struct Gpu_Vk_Desc {
	Allocator* allocator;

	u32 required_extension_count;
	char const* const* required_extensions;

	Gpu_Vk_Swapchain_Init_Desc swapchain_desc;
} Gpu_Vk_Desc;

void gpu_vk_init(const Gpu_Vk_Desc* desc);
void gpu_vk_deinit();

// Swapchain
Gpu_Vk_Swapchain gpu_vk_new_swapchain(const Gpu_Vk_Swapchain_Init_Desc* init_desc);
void gpu_vk_destroy_swapchain(Gpu_Vk_Swapchain swapchain);

bool gpu_vk_fetch_swapchain_texture(Gpu_Vk_Swapchain swapchain, Gpu_Vk_Command_Buffer cb, Gpu_Vk_Swapchain_Desc swapchain_desc, u64 timeout, Gpu_Vk_Texture* out_texture);

// Texture
typedef struct Gpu_Vk_Texture_Desc {
	Gpu_Vk_Texture_Kind kind;
	Gpu_Vk_Texture_Usage usage;
	Gpu_Vk_Format format;
	vec3_u32 size;
	u32 mip_levels;
	u32 array_layers;
} Gpu_Vk_Texture_Desc;
Gpu_Vk_Size_And_Align gpu_vk_size_and_align_for_texture(const Gpu_Vk_Texture_Desc* desc);
Gpu_Vk_Texture gpu_vk_new_texture(const Gpu_Vk_Texture_Desc* desc, Gpu_Vk_Slice slice);
vec3_u32 gpu_vk_get_texture_size(Gpu_Vk_Texture texture);

// Heap
Gpu_Vk_Heap gpu_vk_new_heap(u64 bytes, Gpu_Vk_Memory_Type memory_type);
void gpu_vk_destroy_heap(Gpu_Vk_Heap heap);
u64 gpu_vk_get_heap_size(Gpu_Vk_Heap heap);
Gpu_Vk_Slice gpu_vk_get_heap_slice(Gpu_Vk_Heap heap);

// Sampler

// Command Buffer Pool
Gpu_Vk_Command_Buffer_Pool gpu_vk_new_command_buffer_pool(u32 size);
void gpu_vk_destroy_command_buffer_pool(Gpu_Vk_Command_Buffer_Pool pool);

// Command Buffer
bool gpu_vk_new_command_buffer(Gpu_Vk_Command_Buffer_Pool pool, Gpu_Vk_Command_Buffer* out_cb);
void gpu_vk_enqueue(Gpu_Vk_Command_Buffer cb, bool wait_until_completed);

void gpu_vk_transition_texture_layouts(Gpu_Vk_Command_Buffer cb, const Gpu_Vk_Texture* textures, const Gpu_Vk_Texture_Layout* layouts, u32 count);

// Render
void gpu_vk_begin_render(Gpu_Vk_Command_Buffer cb, const Gpu_Vk_Render_Pass* render_pass);
void gpu_vk_end_render(Gpu_Vk_Command_Buffer cb);
// void gpu_vk_draw(Gpu_Vk_Command_Buffer cb);

// Compute
typedef struct Gpu_Vk_Compute_Pipeline_Desc {
	Gpu_Vk_Code code;
	const char* entry_point;

	const Gpu_Vk_Layout_Binding* bindings;
	u32 binding_count;
} Gpu_Vk_Compute_Pipeline_Desc;
Gpu_Vk_Compute_Pipeline gpu_vk_new_compute_pipeline(const Gpu_Vk_Compute_Pipeline_Desc* desc);

void gpu_vk_dispatch(Gpu_Vk_Command_Buffer cb, Gpu_Vk_Compute_Pipeline pipeline, const Gpu_Vk_Binding* bindings, u32 binding_count, vec3_u32 group_count);

typedef struct Gpu_Vk_Blit_Desc {
	Gpu_Vk_Texture src;
	vec3_u32 src_start;
	vec3_u32 src_end;
	u32 src_array_layer;
	u32 src_mip_level;

	Gpu_Vk_Texture dst;
	vec3_u32 dst_start;
	vec3_u32 dst_end;
	u32 dst_array_layer;
	u32 dst_mip_level;
} Gpu_Vk_Blit_Desc;
void gpu_vk_blit(Gpu_Vk_Command_Buffer cb, const Gpu_Vk_Blit_Desc* desc);

void gpu_vk_barrier(Gpu_Vk_Command_Buffer cb);

// Semaphore
Gpu_Vk_Semaphore gpu_vk_new_semaphore(void);
void gpu_vk_destroy_semaphore(Gpu_Vk_Semaphore semaphore);

typedef void (*Gpu_Vk_On_Notify_Fn)(void* ctx);
void gpu_vk_notify(Gpu_Vk_Semaphore semaphore, u64 value, void* ctx, Gpu_Vk_On_Notify_Fn fn);

void gpu_vk_wait_gpu(Gpu_Vk_Command_Buffer cb, Gpu_Vk_Semaphore semaphore, u64 value);
void gpu_vk_signal_gpu(Gpu_Vk_Command_Buffer cb, Gpu_Vk_Semaphore semaphore, u64 value);

void gpu_vk_wait_cpu(Gpu_Vk_Semaphore semaphore, u64 value);
void gpu_vk_signal_cpu(Gpu_Vk_Semaphore semaphore, u64 value);
