#pragma once

#include <vulkan/vulkan.h>
#include "core.h"

#define GPU_MAX_ATTACHMENTS 8

typedef SL_Handle Gpu_Heap;
typedef SL_Handle Gpu_Command_Buffer_Pool;
typedef SL_Handle Gpu_Texture;
typedef SL_Handle Gpu_Sampler;
typedef SL_Handle Gpu_Render_Pipeline;
typedef SL_Handle Gpu_Compute_Pipeline;
typedef SL_Handle Gpu_Swapchain;
typedef SL_Handle Gpu_Semaphore;

typedef enum Gpu_Format {
	Gpu_Format_RGBA8_Unorm,
	Gpu_Format_RGBA8_sRGB,
	Gpu_Format_BGRA8_Unorm,
	Gpu_Format_BGRA8_sRGB,
	Gpu_Format_RGBA16_Float,
	Gpu_Format_RGBA32_Float,
} Gpu_Format;

typedef struct Gpu_Size_And_Align {
	u64 size;
	u64 align;
} Gpu_Size_And_Align;
#define gpu_size_and_align_of_type(x) ((Gpu_Size_And_Align) { .size = sizeof(x), .align = sl_align_of(x) })

typedef enum Gpu_Primitive_Kind {
	Gpu_Primitive_Kind_Triangle,
} Gpu_Primitive_Kind;

typedef enum Gpu_Cull_Mode {
	Gpu_Cull_Mode_None,
	Gpu_Cull_Mode_Front,
	Gpu_Cull_Mode_Back,
} Gpu_Cull_Mode;

typedef enum Gpu_Texture_Kind {
	Gpu_Texture_Kind_1D,
	Gpu_Texture_Kind_2D,
	Gpu_Texture_Kind_3D,
} Gpu_Texture_Kind;

typedef enum Gpu_Texture_Usage {
	Gpu_Texture_Usage_None = 0,
	Gpu_Texture_Usage_Shader_Read = 1 << 0,
	Gpu_Texture_Usage_Shader_Write = 1 << 1,
	Gpu_Texture_Usage_Render_Attachment = 1 << 2,
} Gpu_Texture_Usage;

typedef enum Gpu_Texture_Layout {
	Gpu_Texture_Layout_Undefined,
	Gpu_Texture_Layout_General,
	Gpu_Texture_Layout_Shader_Read,
	Gpu_Texture_Layout_Shader_Write,
	Gpu_Texture_Layout_Color_Attachment,
	Gpu_Texture_Layout_Present,
} Gpu_Texture_Layout;

typedef enum Gpu_Colorspace {
	Gpu_Colorspace_sRGB,
	Gpu_Colorspace_Extended_sRGB_Linear,
} Gpu_Colorspace;

typedef enum Gpu_Filter {
	Gpu_Filter_Nearest,
	Gpu_Filter_Linear,
} Gpu_Filter;

typedef enum Gpu_Sampler_Coordinate {
	Gpu_Sampler_Coordinate_Normalised,
	Gpu_Sampler_Coordinate_Pixel,
} Gpu_Sampler_Coordinate;

typedef enum Gpu_Sampler_Address_Mode {
	Gpu_Sampler_Address_Mode_Clamp_To_Edge,
	Gpu_Sampler_Address_Mode_Repeat,
	Gpu_Sampler_Address_Mode_Mirrored_Repeat,
	// todo: clamp to border
} Gpu_Sampler_Address_Mode;

typedef struct Gpu_Swapchain_Desc {
    vec2_u32 size;
	Gpu_Format format;
	Gpu_Colorspace colorspace;
} Gpu_Swapchain_Desc;

typedef struct Gpu_Command_Buffer {
	Gpu_Command_Buffer_Pool pool;
	u32 index;
} Gpu_Command_Buffer;

typedef enum Gpu_Load_Op {
	Gpu_Load_Op_Load,
	Gpu_Load_Op_Clear,
	Gpu_Load_Op_Dont_Care
} Gpu_Load_Op;

typedef enum Gpu_Store_Op {
	Gpu_Store_Op_Store,
	Gpu_Store_Op_Dont_Care
} Gpu_Store_Op;

typedef struct Gpu_Render_Pass_Layout_Attachment {
	Gpu_Format format;
	Gpu_Load_Op load_op;
	Gpu_Store_Op store_op;

	Gpu_Texture_Layout initial_layout;
	Gpu_Texture_Layout final_layout;
} Gpu_Render_Pass_Layout_Attachment;

typedef struct Gpu_Render_Pass_Layout {
	Gpu_Render_Pass_Layout_Attachment attachments[GPU_MAX_ATTACHMENTS];
	u8 attachment_count;
} Gpu_Render_Pass_Layout;

typedef struct Gpu_Render_Pass_Values_Attachment {
	Gpu_Texture texture;
	vec4_f32 clear_value;
} Gpu_Render_Pass_Values_Attachment;

typedef struct Gpu_Render_Pass_Values {
	Gpu_Render_Pass_Values_Attachment attachments[GPU_MAX_ATTACHMENTS];
	u8 attachment_count;
} Gpu_Render_Pass_Values;

typedef struct Gpu_Slice {
	Gpu_Heap heap;
	u64 offset;
	u64 size;
} Gpu_Slice;
#define GPU_SLICE_NULL ((Gpu_Slice) {})
sl_inline bool gpu_slice_is_null(Gpu_Slice slice) {
	return slice.heap.index == 0;
}
sl_inline Gpu_Slice gpu_subslice(Gpu_Slice slice, u64 offset, u64 size) {
	return (Gpu_Slice) {
		.heap = slice.heap,
		.offset = slice.offset + offset,
		.size = size,
	};
}
sl_inline Gpu_Slice gpu_slice(Gpu_Heap heap, u64 offset, u64 size) {
	return (Gpu_Slice) {
		.heap = heap,
		.offset = offset,
		.size = size,
	};
}
sl_inline bool gpu_slice_suballocate(Gpu_Slice basis, Gpu_Size_And_Align size_and_align, Gpu_Slice* out_allocation, Gpu_Slice* out_remainder) {
	const u64 aligned_offset = sl_round_up_u64(basis.offset, size_and_align.align);
	if (aligned_offset + size_and_align.size > basis.size) {
		return false;
	}

	*out_allocation = (Gpu_Slice) {
		.heap = basis.heap,
		.offset = aligned_offset,
		.size = size_and_align.size,
	};
	*out_remainder = (Gpu_Slice) {
		.heap = basis.heap,
		.offset = aligned_offset + size_and_align.size,
		.size = basis.size - (basis.offset - aligned_offset + size_and_align.size),
	};
	return true;
}

void* gpu_get_slice_host_ptr(Gpu_Slice slice);
void gpu_flush_slice_to_gpu(Gpu_Slice slice);
void gpu_flush_slice_from_gpu(Gpu_Slice slice);

typedef enum Gpu_Binding_Kind {
	Gpu_Binding_Kind_Storage_Texture,
	Gpu_Binding_Kind_Sampled_Texture,
	Gpu_Binding_Kind_Sampler,
	Gpu_Binding_Kind_Slice,
} Gpu_Binding_Kind;

typedef struct Gpu_Binding {
	Gpu_Binding_Kind kind;
	union {
		Gpu_Texture texture;
		Gpu_Sampler sampler;
		Gpu_Slice slice;
	};
	u32 index;
} Gpu_Binding;

typedef struct Gpu_Layout_Binding {
	Gpu_Binding_Kind kind;
	u32 index;
} Gpu_Layout_Binding;

typedef enum Gpu_Memory_Type {
	Gpu_Memory_Type_Host_Visible,
	Gpu_Memory_Type_Device_Local,
	Gpu_Memory_Type_Count
} Gpu_Memory_Type;

typedef struct Gpu_Code {
	const u32* code;
	u64 size;
} Gpu_Code;

typedef VkSurfaceKHR (*Gpu_Get_Surface_Fn)(void* ctx, VkInstance instance);

typedef struct Gpu_Swapchain_Init_Desc {
	void* ctx;
	Gpu_Get_Surface_Fn get_surface_fn;
} Gpu_Swapchain_Init_Desc;

typedef struct Gpu_Desc {
	Allocator* allocator;

	u32 required_extension_count;
	char const* const* required_extensions;

	Gpu_Swapchain_Init_Desc swapchain_desc;
} Gpu_Desc;

void gpu_init(const Gpu_Desc* desc);
void gpu_deinit();

// Swapchain
Gpu_Swapchain gpu_new_swapchain(const Gpu_Swapchain_Init_Desc* init_desc);
void gpu_destroy_swapchain(Gpu_Swapchain swapchain);

bool gpu_fetch_swapchain_texture(Gpu_Swapchain swapchain, Gpu_Command_Buffer cb, Gpu_Swapchain_Desc swapchain_desc, u64 timeout, Gpu_Texture* out_texture);

// Texture
typedef struct Gpu_Texture_Desc {
	Gpu_Texture_Kind kind;
	Gpu_Texture_Usage usage;
	Gpu_Format format;
	vec3_u32 size;
	u32 mip_levels;
	u32 array_layers;
} Gpu_Texture_Desc;
Gpu_Size_And_Align gpu_size_and_align_for_texture(const Gpu_Texture_Desc* desc);
Gpu_Texture gpu_new_texture(const Gpu_Texture_Desc* desc, Gpu_Slice slice);
vec3_u32 gpu_get_texture_size(Gpu_Texture texture);

// Heap
Gpu_Heap gpu_new_heap(u64 bytes, Gpu_Memory_Type memory_type);
void gpu_destroy_heap(Gpu_Heap heap);
u64 gpu_get_heap_size(Gpu_Heap heap);
Gpu_Slice gpu_get_heap_slice(Gpu_Heap heap);

// Sampler
typedef struct Gpu_Sampler_Desc {
	Gpu_Filter min_filter;
	Gpu_Filter mag_filter;
	Gpu_Filter mip_filter;

	Gpu_Sampler_Address_Mode address_mode_x;
	Gpu_Sampler_Address_Mode address_mode_y;
	Gpu_Sampler_Address_Mode address_mode_z;

	Gpu_Sampler_Coordinate coordinate;
} Gpu_Sampler_Desc;
Gpu_Sampler gpu_new_sampler(const Gpu_Sampler_Desc* desc);
void gpu_destroy_sampler(Gpu_Sampler sampler);

// Command Buffer Pool
Gpu_Command_Buffer_Pool gpu_new_command_buffer_pool(u32 size);
void gpu_destroy_command_buffer_pool(Gpu_Command_Buffer_Pool pool);

// Command Buffer
bool gpu_new_command_buffer(Gpu_Command_Buffer_Pool pool, Gpu_Command_Buffer* out_cb);
void gpu_enqueue(Gpu_Command_Buffer cb, bool wait_until_completed);

void gpu_transition_texture_layouts(Gpu_Command_Buffer cb, const Gpu_Texture* textures, const Gpu_Texture_Layout* layouts, u32 count);

// Render
void gpu_begin_render(Gpu_Command_Buffer cb, const Gpu_Render_Pass_Layout* layout, const Gpu_Render_Pass_Values* values);
void gpu_end_render(Gpu_Command_Buffer cb);

typedef struct Gpu_Render_Pipeline_Desc {
	Gpu_Primitive_Kind primitive_kind;
	Gpu_Cull_Mode cull_mode;

	Gpu_Render_Pass_Layout render_pass_layout;

	Gpu_Code vertex_code;
	Gpu_Code fragment_code;

	const char* vertex_entry_point;
	const char* fragment_entry_point;

	const Gpu_Layout_Binding* bindings;
	u32 binding_count;
} Gpu_Render_Pipeline_Desc;
Gpu_Render_Pipeline gpu_new_render_pipeline(const Gpu_Render_Pipeline_Desc* desc);

typedef struct Gpu_Draw_Desc {
	Gpu_Render_Pipeline pipeline;

	const Gpu_Binding* bindings;
	u32 binding_count;

	u32 instance_count;
	u32 vertex_count;
} Gpu_Draw_Desc;
void gpu_draw(Gpu_Command_Buffer cb, const Gpu_Draw_Desc* desc);

// Compute
typedef struct Gpu_Compute_Pipeline_Desc {
	Gpu_Code code;
	const char* entry_point;

	const Gpu_Layout_Binding* bindings;
	u32 binding_count;
} Gpu_Compute_Pipeline_Desc;
Gpu_Compute_Pipeline gpu_new_compute_pipeline(const Gpu_Compute_Pipeline_Desc* desc);

void gpu_dispatch(Gpu_Command_Buffer cb, Gpu_Compute_Pipeline pipeline, const Gpu_Binding* bindings, u32 binding_count, vec3_u32 group_count);

typedef struct Gpu_Blit_Desc {
	Gpu_Texture src;
	vec3_u32 src_start;
	vec3_u32 src_end;
	u32 src_array_layer;
	u32 src_mip_level;

	Gpu_Texture dst;
	vec3_u32 dst_start;
	vec3_u32 dst_end;
	u32 dst_array_layer;
	u32 dst_mip_level;
} Gpu_Blit_Desc;
void gpu_blit(Gpu_Command_Buffer cb, const Gpu_Blit_Desc* desc);

void gpu_barrier(Gpu_Command_Buffer cb);

// Semaphore
Gpu_Semaphore gpu_new_semaphore(void);
void gpu_destroy_semaphore(Gpu_Semaphore semaphore);

typedef void (*Gpu_On_Notify_Fn)(void* ctx);
void gpu_notify(Gpu_Semaphore semaphore, u64 value, void* ctx, Gpu_On_Notify_Fn fn);

void gpu_wait_gpu(Gpu_Command_Buffer cb, Gpu_Semaphore semaphore, u64 value);
void gpu_signal_gpu(Gpu_Command_Buffer cb, Gpu_Semaphore semaphore, u64 value);

void gpu_wait_cpu(Gpu_Semaphore semaphore, u64 value);
void gpu_signal_cpu(Gpu_Semaphore semaphore, u64 value);
