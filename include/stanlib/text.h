#pragma once

#include <stanlib/core.h>
#include <stanlib/atlas_pack.h>
#include <stanlib/gpu.h>

typedef struct SL_Font SL_Font;
SL_Font* sl_font_load(Allocator* allocator, Immutable_Buffer font_buffer, f32 size, SL_Atlas_Packer* packer);
void sl_font_destroy(SL_Font* font);

Range_s32 sl_font_get_y_range(SL_Font* font);

void sl_font_generate_geometry_for_string(SL_Font* font, Gpu_Texture texture, const char* string, vec2_f32 position, vec4_f32 color, Textured_Quad_f32* out_quads, u32* out_quad_count);

Rect_s32 sl_font_measure_string(SL_Font* font, const char* string);
