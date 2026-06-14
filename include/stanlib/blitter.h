#pragma once

#include <stanlib/core.h>
#include <stanlib/gpu.h>
#include <stanlib/text.h>

// MARK: Blitter Resources

typedef struct SL_Blitter_Resources SL_Blitter_Resources;

SL_Blitter_Resources* sl_blitter_resources_new(Allocator* allocator);
void sl_blitter_resources_destroy(SL_Blitter_Resources* resources);

// MARK: Blitter

typedef struct SL_Blitter SL_Blitter;

typedef struct SL_Blitter_Desc {
	Allocator* allocator;
	SL_Blitter_Resources* resources;
	Gpu_Command_Buffer command_buffer;
	vec2_f32 viewport;
} SL_Blitter_Desc;

typedef enum SL_Blitter_Command_Kind {
	SL_Blitter_Command_Kind_Draw_Text,
	SL_Blitter_Command_Kind_Draw_Texture,
	SL_Blitter_Command_Kind_Draw_Textured_Quads,
} SL_Blitter_Command_Kind;

// For external batching of draws.
// Useful for UI, where the layout is built from top-to-bottom, but draws need to be executed in bottom-to-top.
typedef struct SL_Blitter_Command {
	SL_Blitter_Command_Kind kind;
	union {
		struct {
			SL_Font_Atlas* font;
			const char* string;
			vec2_f32 position;
			vec4_f32 color;
		} draw_text;

		struct {
			Gpu_Texture texture;
			vec2_f32 position;
			vec4_f32 color;
		} draw_texture;

		struct {
			Gpu_Texture texture;
			const Textured_Quad_f32* quads;
			u32 quad_count;
		} draw_textured_quads;
	};
} SL_Blitter_Command;
sl_seq(SL_Blitter_Command, SL_Blitter_Command_Seq, sl_blitter_command_seq)

void sl_blitter_begin(SL_Blitter** blitter, const SL_Blitter_Desc* desc);
void sl_blitter_draw_text(SL_Blitter* blitter, SL_Font_Atlas* font, const char* string, vec2_f32 position, vec4_f32 color);
void sl_blitter_draw_texture(SL_Blitter* blitter, Gpu_Texture texture, vec2_f32 position, vec4_f32 color);
void sl_blitter_draw_textured_quads(SL_Blitter* blitter, Gpu_Texture texture, const Textured_Quad_f32* quads, u32 quad_count);
void sl_blitter_execute_command(SL_Blitter* blitter, SL_Blitter_Command command);
void sl_blitter_end(SL_Blitter** blitter, Gpu_Slice* parameters_slice);
