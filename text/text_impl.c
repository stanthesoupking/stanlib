#include "core.h"
#include "gpu/gpu.h"
#include "text.h"
#include <SDL3/SDL_gpu.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/stb_truetype.h"

#define SL_FONT_ATLAS_FIRST_CHAR 32
#define SL_FONT_ATLAS_CHAR_COUNT 96

typedef struct SL_Font_Atlas {
	Allocator* allocator;
	Gpu_Texture texture;
	stbtt_bakedchar chars[SL_FONT_ATLAS_CHAR_COUNT];
} SL_Font_Atlas;

SL_Font_Atlas* sl_new_font_atlas(const SL_Font_Atlas_Desc* desc) {
	const Gpu_Slice atlas_allocator_initial = *desc->atlas_allocator;
	const Gpu_Slice staging_allocator_initial = *desc->staging_allocator;

	const Gpu_Texture_Desc texture_desc = {
		.kind = Gpu_Texture_Kind_2D,
		.usage = Gpu_Texture_Usage_Shader_Read,
		.format = Gpu_Format_R8_Unorm,
		.size = { desc->atlas_size, desc->atlas_size, 1 },
		.mip_levels = 1,
		.array_layers = 1,
	};

	const Gpu_Size_And_Align texture_size_and_align = gpu_size_and_align_for_texture(&texture_desc);
	Gpu_Slice texture_slice;
	if (!gpu_slice_suballocate(*desc->atlas_allocator, texture_size_and_align, &texture_slice, desc->atlas_allocator)) {
		return NULL;
	}

	const u64 row_length = desc->atlas_size;
	const u64 staging_length = row_length * desc->atlas_size;
	Gpu_Slice staging_slice;
	if (!gpu_slice_suballocate(*desc->staging_allocator, (Gpu_Size_And_Align) { .size = staging_length, .align = 1 }, &staging_slice, desc->staging_allocator)) {
		*desc->atlas_allocator = atlas_allocator_initial;
		return NULL;
	}

	SL_Font_Atlas* result;
	allocator_new(desc->allocator, result, 1);
	*result = (SL_Font_Atlas) {
		.allocator = desc->allocator,
	};

	void* staging_pixels = gpu_get_slice_host_ptr(staging_slice);
	sl_memset(staging_pixels, 0, staging_length);
	const s32 bake_result = stbtt_BakeFontBitmap(desc->font.data, 0, desc->font_size, staging_pixels, desc->atlas_size, desc->atlas_size, SL_FONT_ATLAS_FIRST_CHAR, SL_FONT_ATLAS_CHAR_COUNT, result->chars);
	if (bake_result == -1) {
		*desc->atlas_allocator = atlas_allocator_initial;
		*desc->staging_allocator = staging_allocator_initial;
		allocator_free(desc->allocator, result, 1);
		return NULL;
	}
	gpu_flush_slice_to_gpu(staging_slice);

	result->texture = gpu_new_texture(&texture_desc, texture_slice);

	Gpu_Texture_Layout texture_layout_0 = Gpu_Texture_Layout_General;
	gpu_transition_texture_layouts(desc->command_buffer, &result->texture, &texture_layout_0, 1);

	const Gpu_Copy_Slice_To_Texture_Desc copy_desc = {
		.src = staging_slice,
		.src_row_length = row_length,
		.dst = result->texture,
		.dst_start = { 0, 0, 0 },
		.dst_end = { desc->atlas_size, desc->atlas_size, 1 },
		.dst_mip_level = 0,
		.dst_array_layer = 0,
	};
	gpu_copy_slice_to_texture(desc->command_buffer, &copy_desc);

	Gpu_Texture_Layout texture_layout_1 = Gpu_Texture_Layout_Shader_Read;
	gpu_transition_texture_layouts(desc->command_buffer, &result->texture, &texture_layout_1, 1);

	return result;
}
void sl_destroy_font_atlas(SL_Font_Atlas* atlas) {
	Allocator* allocator = atlas->allocator;
	gpu_destroy_texture(atlas->texture);
	allocator_free(allocator, atlas, 1);
}

Gpu_Texture sl_get_font_atlas_texture(SL_Font_Atlas* atlas) {
	return atlas->texture;
}
