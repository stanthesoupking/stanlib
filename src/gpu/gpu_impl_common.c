#include <stanlib/gpu.h>
#include <string.h>

#define GPU_LOGGING 0
#define GPU_VALIDATION 1

#if GPU_LOGGING
	#define gpu_log(fmt, ...) \
		printf("[Gpu]: " fmt "\n", ##__VA_ARGS__)
#else
	#define gpu_log(fmt, ...) ((void)0)
#endif

#if GPU_VALIDATION
	#define gpu_validate(condition, message) sl_assert(condition, message)
#else
	#define gpu_validate(condition, message) ((void)0)
#endif

typedef struct Gpu_Callback {
	void* ctx;
	Gpu_Callback_Fn fn;
} Gpu_Callback;
sl_seq(Gpu_Callback, Gpu_Callback_Seq, gpu_callback_seq);

typedef struct Gpu_Semaphore_On_Notify_Callback {
	u64 value;
	void* ctx;
	Gpu_Callback_Fn fn;
} Gpu_Semaphore_On_Notify_Callback;
sl_seq(Gpu_Semaphore_On_Notify_Callback, Gpu_Semaphore_On_Notify_Callback_Seq, gpu_semaphore_on_notify_callback_seq);

typedef enum Gpu_Command_Buffer_State {
	Gpu_Command_Buffer_State_Idle,
	Gpu_Command_Buffer_State_Recording,
	Gpu_Command_Buffer_State_Enqueued
} Gpu_Command_Buffer_State;

sl_inline u8 gpu_bytes_per_pixel_for_format(Gpu_Format format) {
	switch (format) {
		case Gpu_Format_R8_Unorm:
			return 1;

		case Gpu_Format_RGBA8_Unorm:
		case Gpu_Format_RGBA8_sRGB:
		case Gpu_Format_BGRA8_Unorm:
		case Gpu_Format_BGRA8_sRGB:
			return 4;

		case Gpu_Format_RGBA16_Float:
			return 8;

		case Gpu_Format_RGBA32_Float:
			return 16;
	}
}

u32 gpu_get_command_buffer_index(Gpu_Command_Buffer cb) {
	return cb.index;
}

Gpu_Texture gpu_new_texture_from_image(SL_Image* image, Gpu_Slice* inout_staging_allocator, Gpu_Slice* inout_texture_allocator, Gpu_Command_Buffer command_buffer) {
	// For reverting:
	const Gpu_Slice texture_allocator_initial = *inout_texture_allocator;

	const Gpu_Texture_Desc texture_desc = {
		.mip_levels = 1,
		.array_layers = 1,
		.format = Gpu_Format_RGBA8_sRGB,
		.usage = Gpu_Texture_Usage_Shader_Read,
		.kind = Gpu_Texture_Kind_2D,
		.size = { image->size.x, image->size.y, 1 },
	};

	Gpu_Slice texture_slice;
	if (!gpu_slice_suballocate(*inout_texture_allocator, gpu_size_and_align_for_texture(&texture_desc), &texture_slice, inout_texture_allocator)) {
		return SL_HANDLE_NULL;
	}

	Gpu_Slice staging_slice;
	if (!gpu_slice_suballocate(*inout_staging_allocator, (Gpu_Size_And_Align) { .size = image->buffer.size, .align = 1 }, &staging_slice, inout_staging_allocator)) {
		*inout_texture_allocator = texture_allocator_initial;
		return SL_HANDLE_NULL;
	}

	void* staging_slice_ptr = gpu_get_slice_host_ptr(staging_slice);
	memcpy(staging_slice_ptr, image->buffer.data, image->buffer.size);

	Gpu_Texture texture = gpu_new_texture(&texture_desc, texture_slice);

	const Gpu_Texture_Layout blit_layout = Gpu_Texture_Layout_General;
	gpu_transition_texture_layouts(command_buffer, &texture, &blit_layout, 1);

	const Gpu_Copy_Slice_To_Texture_Desc blit = {
		.src = staging_slice,
		.src_row_length = (u32)image->row_length / 4,
		.dst = texture,
		.dst_start = { 0, 0, 0 },
		.dst_end = { image->size.x, image->size.y, 1 },
		.dst_mip_level = 0,
		.dst_array_layer = 0,
	};
	gpu_copy_slice_to_texture(command_buffer, &blit);

	const Gpu_Texture_Layout read_layout = Gpu_Texture_Layout_Shader_Read;
	gpu_transition_texture_layouts(command_buffer, &texture, &read_layout, 1);

	return texture;
}
