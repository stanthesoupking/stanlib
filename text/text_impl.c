#include "core.h"
#include "gpu/gpu.h"
#include "text.h"
#include <SDL3/SDL_gpu.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/stb_truetype.h"

#define SL_FONT_ATLAS_FIRST_CHAR 32
#define SL_FONT_ATLAS_LAST_CHAR 127
#define SL_FONT_ATLAS_CHAR_COUNT (SL_FONT_ATLAS_LAST_CHAR - SL_FONT_ATLAS_FIRST_CHAR) + 1

typedef struct SL_Font_Atlas {
	Allocator* allocator;
	Gpu_Texture texture;
	stbtt_bakedchar chars[SL_FONT_ATLAS_CHAR_COUNT];
	Range_s32 y_range;
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

	Range_s32 y_range = {
		.start = s32_max,
		.end = s32_min,
	};
	for (u32 i = 0; i < SL_FONT_ATLAS_CHAR_COUNT; i++) {
		const f32 char_h = result->chars[i].y1 - result->chars[i].y0;
		y_range.start = sl_min(y_range.start, result->chars[i].yoff);
		y_range.end = sl_max(y_range.end, result->chars[i].yoff + char_h);
	}
	result->y_range = y_range;

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

Range_s32 sl_get_font_y_range(SL_Font_Atlas* atlas) {
	return atlas->y_range;
}

void sl_get_font_geometry_for_string(SL_Font_Atlas* atlas, const char* string, vec2_f32 position, vec4_f32 color, Textured_Quad_f32* out_quads, u32* out_quad_count) {
	u32 quad_count = 0;

	const Gpu_Texture_Desc* texture_desc = gpu_get_texture_desc(atlas->texture);
	const vec2_f32 texture_size = { texture_desc->size.x, texture_desc->size.y };

	f32 cx = position.x;
	f32 cy = position.y;
	for (u32 i = 0; string[i] != '\0'; i++) {
		char c = string[i];
		if (c >= SL_FONT_ATLAS_FIRST_CHAR && c <= SL_FONT_ATLAS_LAST_CHAR) {
			stbtt_bakedchar baked_char = atlas->chars[c - SL_FONT_ATLAS_FIRST_CHAR];

			const u32 char_w = baked_char.x1 - baked_char.x0;
			const u32 char_h = baked_char.y1 - baked_char.y0;

			if (out_quads != NULL) {
				vec2_f32 p_start = { round(cx + baked_char.xoff), round(cy + baked_char.yoff) };
				vec2_f32 p_end = add_vec2_f32(p_start, (vec2_f32) { char_w, char_h });
				out_quads[quad_count] = (Textured_Quad_f32) {
					.position = {
						{ p_start.x, p_start.y, 0.0f, 1.0f },
						{ p_end.x, p_start.y, 0.0f, 1.0f },
						{ p_start.x, p_end.y, 0.0f, 1.0f },
						{ p_end.x, p_end.y, 0.0f, 1.0f },
					},
					.tint = {
						color,
						color,
						color,
						color
					},
					.uv = {
						{ baked_char.x0 / texture_size.x, baked_char.y0 / texture_size.y },
						{ baked_char.x1 / texture_size.x, baked_char.y0 / texture_size.y },
						{ baked_char.x0 / texture_size.x, baked_char.y1 / texture_size.y },
						{ baked_char.x1 / texture_size.x, baked_char.y1 / texture_size.y },
					},
				};
			}
			quad_count++;

			cx += baked_char.xadvance;
		}
	}

	if (out_quad_count != NULL) {
		*out_quad_count = quad_count;
	}
}

Rect_s32 sl_font_atlas_measure_string(SL_Font_Atlas* atlas, const char* string) {
	if (string[0] == '\0') {
		return (Rect_s32) {};
	}

	f32 cx = 0.0f;

	Rect_s32 rect = {
		.start = { s32_max, s32_max },
		.end = { s32_min, s32_min }
	};
	for (u32 i = 0; string[i] != '\0'; i++) {
		char c = string[i];

		if (c >= SL_FONT_ATLAS_FIRST_CHAR && c <= SL_FONT_ATLAS_LAST_CHAR) {
			stbtt_bakedchar baked_char = atlas->chars[c - SL_FONT_ATLAS_FIRST_CHAR];

			const u32 char_w = baked_char.x1 - baked_char.x0;
			const u32 char_h = baked_char.y1 - baked_char.y0;

			vec2_s32 p_start = { round(cx + baked_char.xoff), round(baked_char.yoff) };
			vec2_s32 p_end = add_vec2_s32(p_start, (vec2_s32) { char_w, char_h });

			rect.start = min_vec2_s32(rect.start, p_start);
			rect.end = max_vec2_s32(rect.end, p_end);

			cx += baked_char.xadvance;
		}
	}

	return rect;
}
