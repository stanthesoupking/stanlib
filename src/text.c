#include <stanlib/core.h>
#include <stanlib/gpu.h>
#include <stanlib/text.h>

#include "vendor/stb_truetype.h"

#define SL_FONT_FIRST_CHAR 32
#define SL_FONT_LAST_CHAR 127
#define SL_FONT_CHAR_COUNT (SL_FONT_LAST_CHAR - SL_FONT_FIRST_CHAR) + 1

typedef struct SL_Font_Char {
	SL_Image* image;
	
	vec2_f32 offset;
	f32 advance_x;
	Rect_u32 atlas_rect;
} SL_Font_Char;

typedef struct SL_Font {
	Allocator* allocator;
	SL_Font_Char chars[SL_FONT_CHAR_COUNT];
	
	Range_s32 y_range;
} SL_Font;

SL_Font* sl_font_load(Allocator* allocator, Immutable_Buffer font_buffer, f32 size, SL_Atlas_Packer* packer) {
	stbtt_fontinfo font_info;
	if (!stbtt_InitFont(&font_info, font_buffer.data, 0)) {
		return NULL;
	}
	
	SL_Font* font;
	allocator_new(allocator, font, 1);
	*font = (SL_Font) {
		.allocator = allocator,
	};
	
	const f32 scale = stbtt_ScaleForPixelHeight(&font_info, size);
	
	Range_s32 y_range = {
		.start = s32_max,
		.end = s32_min,
	};
	
	for (u32 char_idx = 0; char_idx < SL_FONT_CHAR_COUNT; char_idx++) {
		const s32 glyph_index = stbtt_FindGlyphIndex(&font_info, char_idx + SL_FONT_FIRST_CHAR);
		
		int advance, lsb;
		stbtt_GetGlyphHMetrics(&font_info, glyph_index, &advance, &lsb);
		
		int x0, y0, x1, y1;
		stbtt_GetGlyphBitmapBox(&font_info, glyph_index, scale, scale, &x0, &y0, &x1, &y1);
		
		const f32 char_h = y1 - y0;
		y_range.start = sl_min(y_range.start, y0);
		y_range.end = sl_max(y_range.end, y0 + char_h);
		
		const vec2_u32 image_size = {
			(u32)(x1 - x0),
			(u32)(y1 - y0),
		};
				
		const u64 staging_size = image_size.x * image_size.y;
		u8* staging;
		allocator_new(allocator, staging, staging_size);
		memset(staging, 0, staging_size);
		
		stbtt_MakeGlyphBitmap(&font_info, staging, image_size.x, image_size.y, image_size.x, scale, scale, glyph_index);
		
		SL_Image* image = sl_image_new(allocator, image_size, image_size.x * 4);
		
		for (u32 img_y = 0; img_y < image_size.y; img_y++) {
			u8* staging_row = staging + (img_y * image_size.x);
			u8* img_row = (u8*)image->buffer.data + (img_y * image->row_length);
			for (u32 img_x = 0; img_x < image_size.x; img_x++) {
				const u8 value = staging_row[img_x];
				u8* img_pixel = img_row + (img_x * 4);
				
				const vec4_f32 value_v4 = linear_to_srgb_vec4_f32(splat_vec4_f32(value / 255.0f));
				img_pixel[0] = (u8)roundf(value_v4.x * 255.0f);
				img_pixel[1] = (u8)roundf(value_v4.y * 255.0f);
				img_pixel[2] = (u8)roundf(value_v4.z * 255.0f);
				img_pixel[3] = (u8)roundf(value_v4.w * 255.0f);
			}
		}
		
		allocator_free(allocator, staging, staging_size);
		
		font->chars[char_idx] = (SL_Font_Char) {
			.image = image,
			.offset = { x0, y0 },
			.advance_x = scale * (f32)advance,
		};
		
		sl_atlas_packer_add(packer, image, &font->chars[char_idx].atlas_rect);
	}

	font->y_range = y_range;
	
	return font;
}
void sl_font_destroy(SL_Font* font) {
	Allocator* allocator = font->allocator;
	allocator_free(allocator, font, 1);
}

Range_s32 sl_font_get_y_range(SL_Font* font) {
	return font->y_range;
}

void sl_font_generate_geometry_for_string(SL_Font* font, Gpu_Texture texture, const char* string, vec2_f32 position, vec4_f32 color, Textured_Quad_f32* out_quads, u32* out_quad_count) {
	u32 quad_count = 0;
	
	const Gpu_Texture_Desc* texture_desc = gpu_get_texture_desc(texture);
	const vec2_f32 texture_size = { texture_desc->size.x, texture_desc->size.y };
	
	f32 cx = position.x;
	f32 cy = position.y;
	for (u32 i = 0; string[i] != '\0'; i++) {
		char c = string[i];
		if (c >= SL_FONT_FIRST_CHAR && c <= SL_FONT_LAST_CHAR) {
			SL_Font_Char baked_char = font->chars[c - SL_FONT_FIRST_CHAR];
			
			const u32 char_w = baked_char.atlas_rect.end.x - baked_char.atlas_rect.start.x;
			const u32 char_h = baked_char.atlas_rect.end.y - baked_char.atlas_rect.start.y;
			
			if (out_quads != NULL) {
				vec2_f32 p_start = { round(cx + baked_char.offset.x), round(cy + baked_char.offset.y) };
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
							{ baked_char.atlas_rect.start.x / texture_size.x, baked_char.atlas_rect.start.y / texture_size.y },
							{ baked_char.atlas_rect.end.x / texture_size.x, baked_char.atlas_rect.start.y / texture_size.y },
							{ baked_char.atlas_rect.start.x / texture_size.x, baked_char.atlas_rect.end.y / texture_size.y },
							{ baked_char.atlas_rect.end.x / texture_size.x, baked_char.atlas_rect.end.y / texture_size.y },
						},
				};
			}
			quad_count++;
			
			cx += baked_char.advance_x;
		}
	}
	
	if (out_quad_count != NULL) {
		*out_quad_count = quad_count;
	}
}

Rect_s32 sl_font_measure_string(SL_Font* font, const char* string) {
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
		
		if (c >= SL_FONT_FIRST_CHAR && c <= SL_FONT_LAST_CHAR) {
			SL_Font_Char baked_char = font->chars[c - SL_FONT_FIRST_CHAR];
			
			const u32 char_w = baked_char.atlas_rect.end.x - baked_char.atlas_rect.start.x;
			const u32 char_h = baked_char.atlas_rect.end.y - baked_char.atlas_rect.start.y;
			
			vec2_s32 p_start = { round(cx + baked_char.offset.x), round(baked_char.offset.y) };
			vec2_s32 p_end = add_vec2_s32(p_start, (vec2_s32) { char_w, char_h });
			
			rect.start = min_vec2_s32(rect.start, p_start);
			rect.end = max_vec2_s32(rect.end, p_end);
			
			cx += baked_char.advance_x;
		}
	}
	
	return rect;
}
