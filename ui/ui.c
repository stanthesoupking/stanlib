#include "ui.h"
#include "blitter/blitter.h"
#include "core.h"
#include "gpu/gpu.h"
#include "text/text.h"
#include <string.h>

UI_ID ui_id(UI_ID parent, u32 item, u32 index) {
	SL_Hasher hasher;
	sl_hasher_init(&hasher);
	sl_hasher_push(&hasher, immutable_buffer_for(parent.hash));
	sl_hasher_push(&hasher, immutable_buffer_for(item));
	sl_hasher_push(&hasher, immutable_buffer_for(index));
	return (UI_ID) {
		.hash = sl_hasher_finalise(&hasher),
	};
}

typedef enum UI_Stack_Item_Kind {
	UI_Stack_Item_Kind_Panel,
} UI_Stack_Item_Kind;

typedef struct UI_Stack_Item {
	UI_Stack_Item_Kind kind;

	union {
		struct {
			Rect_f32 rect;
			vec4_f32 color;
		} panel;
	};
} UI_Stack_Item;

#define UI_STACK_SIZE 32

u64 ui_id_get_hash(UI_ID id) {
	return id.hash;
}
bool ui_id_equals(UI_ID a, UI_ID b) {
	return a.hash == b.hash;
}
bool ui_id_is_null(UI_ID a) {
	return a.hash == 0;
}

typedef struct UI {
	Allocator* allocator;
	Gpu_Texture texture;
	vec2_f32 texture_size;

	u64 frame_index;

	UI_ID hot_item;
	UI_ID active_item;

	vec2_f32 mouse_position;
	bool mouse_button_down;
	bool mouse_button_pressed;

	// is the mouse obscured (i.e. by a panel)
	bool mouse_obscured;

	UI_Stack_Item stack[UI_STACK_SIZE];
	u8 stack_length;

	SL_Blitter_Command_Seq blitter_commands;

	SL_Arena_Allocator* arena;
} UI;

UI* ui_new(Allocator* allocator, Gpu_Texture texture) {
	const Gpu_Texture_Desc* texture_desc = gpu_get_texture_desc(texture);
	const vec2_f32 texture_size = { texture_desc->size.x, texture_desc->size.y };

	UI* ui;
	allocator_new(allocator, ui, 1);
	*ui = (UI) {
		.allocator = allocator,
		.texture = texture,
		.texture_size = texture_size,
		.frame_index = 0ull,
		.hot_item = UI_ID_NULL,
		.active_item = UI_ID_NULL,
		.blitter_commands = sl_blitter_command_seq_new(allocator, 256),
		.arena = sl_arena_allocator_new(allocator, 64ull * 1024ull),
	};
	return ui;
}
void ui_destroy(UI* ui) {
	Allocator* allocator = ui->allocator;
	*ui = (UI) {0};
	allocator_free(allocator, ui, 1);
}

void ui_begin(UI* ui, UI_Event_Seq* event_sink) {
	++ui->frame_index;

	sl_arena_allocator_reset(ui->arena, 0);
	sl_blitter_command_seq_clear(&ui->blitter_commands);

	// Handle events
	{
		ui->mouse_obscured = false;
		ui->mouse_button_pressed = false;
		ui->hot_item = UI_ID_NULL;

		const u32 event_count = ui_event_seq_get_count(event_sink);
		for (u32 i = 0; i < event_count; i++) {
			UI_Event event = ui_event_seq_get(event_sink, i);
			switch (event.kind) {
				case UI_Event_Kind_Mouse_Move: {
					ui->mouse_position = event.mouse_move.position;
				} break;

				case UI_Event_Kind_Mouse_Mouse_Down: {
					ui->mouse_button_down = true;
				} break;

				case UI_Event_Kind_Mouse_Mouse_Up: {
					ui->mouse_button_pressed |= ui->mouse_button_down;
					ui->mouse_button_down = false;
				} break;
			}
		}
		ui_event_seq_clear(event_sink);
	}
}

void ui_begin_panel(UI* ui, Rect_f32 rect, vec4_f32 color) {
	// push to stack
	ui->stack[ui->stack_length++] = (UI_Stack_Item) {
		.kind = UI_Stack_Item_Kind_Panel,
		.panel = {
			.rect = rect,
			.color = color,
		},
	};
}

void ui_draw_image(UI* ui, Rect_f32 rect, Rect_f32 atlas_rect, vec4_f32 color) {
	Textured_Quad_f32* quad;
	allocator_new(&ui->arena->allocator, quad, 1);
	*quad = textured_quad_for_sub_region_f32(rect, div_rect_vec_f32(atlas_rect, ui->texture_size), color);
	sl_blitter_command_seq_push(&ui->blitter_commands, (SL_Blitter_Command) {
		.kind = SL_Blitter_Command_Kind_Draw_Textured_Quads,
		.draw_textured_quads = {
			.texture = ui->texture,
			.quads = quad,
			.quad_count = 1,
		},
	});
}

void ui_end_panel(UI* ui) {
	sl_assert(ui->stack_length > 0, "Nothing to pop.");
	const UI_Stack_Item top = ui->stack[--ui->stack_length];
	sl_assert(top.kind == UI_Stack_Item_Kind_Panel, "Top of the stack must be a panel.");

	if (top.panel.color.w > 0.0f) {
		ui_draw_image(ui, top.panel.rect, (Rect_f32) {}, top.panel.color);
	}

	if (contains_rect_f32(top.panel.rect, ui->mouse_position)) {
		ui->mouse_obscured = true;
	}
}

void ui_draw_nine_patch(UI* ui, Rect_f32 rect, UI_Nine_Patch nine_patch, vec4_f32 tint) {
	Textured_Quad_f32* quads;
	allocator_new(&ui->arena->allocator, quads, 9);

	u32 next_quad = 0;

	// tl
	{
		const Rect_f32 patch_rect = {
			.start = rect.start,
			.end = { rect.start.x + nine_patch.margins.left, rect.start.y + nine_patch.margins.top },
		};
		const Rect_f32 patch_uv_rect = {
			.start = { nine_patch.atlas_region.start.x, nine_patch.atlas_region.start.y },
			.end = { nine_patch.atlas_region.start.x + nine_patch.margins.left, nine_patch.atlas_region.start.y + nine_patch.margins.top },
		};
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, ui->texture_size), tint);
	}

	// tr
	{
		const Rect_f32 patch_rect = {
			.start = { rect.end.x - nine_patch.margins.right, rect.start.y },
			.end = { rect.end.x, rect.start.y + nine_patch.margins.top },
		};
		const Rect_f32 patch_uv_rect = {
			.start = { nine_patch.atlas_region.end.x - nine_patch.margins.right, nine_patch.atlas_region.start.y },
			.end = { nine_patch.atlas_region.end.x, nine_patch.atlas_region.start.y + nine_patch.margins.top },
		};
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, ui->texture_size), tint);
	}

	// bl
	{
		const Rect_f32 patch_rect = {
			.start = { rect.start.x, rect.end.y - nine_patch.margins.bottom },
			.end = { rect.start.x + nine_patch.margins.left, rect.end.y },
		};
		const Rect_f32 patch_uv_rect = {
			.start = { nine_patch.atlas_region.start.x, nine_patch.atlas_region.end.y - nine_patch.margins.bottom },
			.end = { nine_patch.atlas_region.start.x + nine_patch.margins.left, nine_patch.atlas_region.end.y },
		};
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, ui->texture_size), tint);
	}

	// br
	{
		const Rect_f32 patch_rect = {
			.start = { rect.end.x - nine_patch.margins.right, rect.end.y - nine_patch.margins.bottom },
			.end = { rect.end.x, rect.end.y },
		};
		const Rect_f32 patch_uv_rect = {
			.start = { nine_patch.atlas_region.end.x - nine_patch.margins.right, nine_patch.atlas_region.end.y - nine_patch.margins.bottom },
			.end = { nine_patch.atlas_region.end.x, nine_patch.atlas_region.end.y },
		};
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, ui->texture_size), tint);
	}

	// t
	{
		const Rect_f32 patch_rect = {
			.start = { rect.start.x + nine_patch.margins.left, rect.start.y },
			.end = { rect.end.x - nine_patch.margins.right, rect.start.y + nine_patch.margins.top },
		};
		const Rect_f32 patch_uv_rect = {
			.start = { nine_patch.atlas_region.start.x + nine_patch.margins.left, nine_patch.atlas_region.start.y },
			.end = { nine_patch.atlas_region.start.x + nine_patch.margins.left, nine_patch.atlas_region.start.y + nine_patch.margins.top },
		};
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, ui->texture_size), tint);
	}

	// b
	{
		const Rect_f32 patch_rect = {
			.start = { rect.start.x + nine_patch.margins.left, rect.end.y - nine_patch.margins.bottom },
			.end = { rect.end.x - nine_patch.margins.right, rect.end.y },
		};
		const Rect_f32 patch_uv_rect = {
			.start = { nine_patch.atlas_region.start.x + nine_patch.margins.left, nine_patch.atlas_region.end.y - nine_patch.margins.bottom },
			.end = { nine_patch.atlas_region.start.x + nine_patch.margins.left, nine_patch.atlas_region.end.y },
		};
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, ui->texture_size), tint);
	}

	// l
	{
		const Rect_f32 patch_rect = {
			.start = { rect.start.x, rect.start.y + nine_patch.margins.top },
			.end = { rect.start.x + nine_patch.margins.left, rect.end.y - nine_patch.margins.bottom },
		};
		const Rect_f32 patch_uv_rect = {
			.start = { nine_patch.atlas_region.start.x, nine_patch.atlas_region.start.y + nine_patch.margins.top },
			.end = { nine_patch.atlas_region.start.x + nine_patch.margins.left, nine_patch.atlas_region.end.y - nine_patch.margins.bottom },
		};
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, ui->texture_size), tint);
	}

	// r
	{
		const Rect_f32 patch_rect = {
			.start = { rect.end.x - nine_patch.margins.right, rect.start.y + nine_patch.margins.top },
			.end = { rect.end.x, rect.end.y - nine_patch.margins.bottom },
		};
		const Rect_f32 patch_uv_rect = {
			.start = { nine_patch.atlas_region.end.x - nine_patch.margins.right, nine_patch.atlas_region.start.y + nine_patch.margins.top },
			.end = { nine_patch.atlas_region.end.x, nine_patch.atlas_region.end.y - nine_patch.margins.bottom },
		};
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, ui->texture_size), tint);
	}

	// c
	{
		const Rect_f32 patch_rect = {
			.start = { rect.start.x + nine_patch.margins.left, rect.start.y + nine_patch.margins.top },
			.end = { rect.end.x - nine_patch.margins.right, rect.end.y - nine_patch.margins.bottom },
		};
		const Rect_f32 patch_uv_rect = {
			.start = { nine_patch.atlas_region.start.x + nine_patch.margins.left, nine_patch.atlas_region.start.y + nine_patch.margins.top },
			.end = { nine_patch.atlas_region.end.x - nine_patch.margins.right, nine_patch.atlas_region.end.y - nine_patch.margins.bottom },
		};
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, ui->texture_size), tint);
	}

	sl_blitter_command_seq_push(&ui->blitter_commands, (SL_Blitter_Command) {
		.kind = SL_Blitter_Command_Kind_Draw_Textured_Quads,
		.draw_textured_quads = {
			.texture = ui->texture,
			.quads = quads,
			.quad_count = next_quad,
		},
	});
}

bool ui_button(UI* ui, UI_ID id, const UI_Button_Style* style, const char* label, Rect_f32 rect) {
	const u32 label_len = strlen(label);

	char* label_copy;
	allocator_new(&ui->arena->allocator, label_copy, label_len + 1);
	strcpy(label_copy, label);

	const Range_s32 font_y_range = sl_get_font_y_range(style->font);
	const Rect_s32 label_rect = sl_font_atlas_measure_string(style->font, label);
	const vec2_s32 label_size = size_rect_s32(label_rect);
	const vec2_f32 rect_size = size_rect_f32(rect);
	const vec2_f32 text_offset = {
		.x = rect.start.x + (rect_size.x * 0.5f) - (label_size.x * 0.5f),
		.y = rect.start.y + (rect_size.y * 0.5f) - (font_y_range.start * 0.5f),
	};

	const bool mouse_over = !ui->mouse_obscured && contains_rect_f32(rect, ui->mouse_position);
	if (mouse_over) {
		ui->hot_item = id;
		ui->mouse_obscured = true;
	}

	if (ui_id_is_null(ui->active_item) && (ui->mouse_button_down || ui->mouse_button_pressed) && mouse_over) {
		ui->active_item = id;
	}

	UI_Button_State button_state;
	if (ui_id_equals(ui->active_item, id)) {
		button_state = UI_Button_State_Active;
	} else if (ui_id_equals(ui->hot_item, id)) {
		button_state = UI_Button_State_Hot;
	} else {
		button_state = UI_Button_State_Normal;
	}
	const UI_Button_State_Style* state_style = &style->state[button_state];

	sl_blitter_command_seq_push(&ui->blitter_commands, (SL_Blitter_Command) {
		.kind = SL_Blitter_Command_Kind_Draw_Text,
		.draw_text = {
			.font = style->font,
			.string = label_copy,
			.position = add_vec2_f32(text_offset, state_style->label_offset),
			.color = state_style->label_color,
		},
	});

	ui_draw_nine_patch(ui, rect, state_style->backing, state_style->backing_color);

	if (ui_id_equals(ui->active_item, id) && ui->mouse_button_pressed) {
		ui->mouse_button_pressed = false;
		ui->active_item = UI_ID_NULL;
		return mouse_over;
	} else {
		return false;
	}
}

bool ui_slider_f32(UI* ui, UI_ID id, const UI_Slider_Style* style, f32* value, Range_f32 range, Rect_f32 rect) {
	const vec2_f32 rect_size = size_rect_f32(rect);
	const f32 center_y = (rect.end.y + rect.start.y) * 0.5f;
	const vec2_f32 needle_image_size = size_rect_f32(style->needle_image);

	// track layout
	const f32 track_lr_padding = ceil(needle_image_size.x * 0.5);
	const vec2_f32 track_rect_start = {
		rect.start.x + track_lr_padding,
		round(center_y - (style->track_height * 0.5f)),
	};
	const Rect_f32 track_rect = {
		.start = track_rect_start,
		.end = {
			rect.end.x - track_lr_padding,
			.y = track_rect_start.y + style->track_height,
		},
	};

	const bool mouse_over = !ui->mouse_obscured && contains_rect_f32(rect, ui->mouse_position);
	if (mouse_over) {
		ui->hot_item = id;
		ui->mouse_obscured = true;
	}

	if (ui_id_is_null(ui->active_item) && (ui->mouse_button_down || ui->mouse_button_pressed) && mouse_over) {
		ui->active_item = id;
	}

	const bool change_value = ui_id_equals(ui->active_item, id) && ui->mouse_button_down || ui->mouse_button_pressed;
	if (change_value) {
		ui->mouse_button_pressed = false;

		const f32 mouse_progress = saturate_f32((ui->mouse_position.x - track_rect.start.x) / (track_rect.end.x - track_rect.start.x));
		*value = lerp_f32(range.start, range.end, mouse_progress);
	}

	// needle
	const f32 progress = saturate_f32((*value - range.start) / (range.end - range.start));
	const vec2_f32 needle_offset = {
		.x = round(lerp_f32(track_rect.start.x, track_rect.end.x, progress) - (needle_image_size.x * 0.5f)),
		.y = round(center_y - (needle_image_size.y * 0.5f)),
	};
	const Rect_f32 needle_rect = {
		.start = needle_offset,
		.end = add_vec2_f32(needle_image_size, needle_offset),
	};
	ui_draw_image(ui, needle_rect, style->needle_image, style->needle_color);

	// track
	ui_draw_image(ui, track_rect, (Rect_f32) { .start = {} }, style->track_color);

	if (ui_id_equals(ui->active_item, id) && !ui->mouse_button_down) {
		ui->active_item = UI_ID_NULL;
	}

	return change_value;
}

void ui_label(UI* ui, UI_ID id, const UI_Label_Style* style, const char* label, Rect_f32 rect) {
	const u32 label_len = strlen(label);

	char* label_copy;
	allocator_new(&ui->arena->allocator, label_copy, label_len + 1);
	strcpy(label_copy, label);

	const Range_s32 font_y_range = sl_get_font_y_range(style->font);
	const vec2_f32 rect_size = size_rect_f32(rect);
	const vec2_f32 text_offset = {
		.x = rect.start.x,
		.y = rect.start.y + (rect_size.y * 0.5f) - (font_y_range.start * 0.5f),
	};

	sl_blitter_command_seq_push(&ui->blitter_commands, (SL_Blitter_Command) {
		.kind = SL_Blitter_Command_Kind_Draw_Text,
		.draw_text = {
			.font = style->font,
			.string = label_copy,
			.position = text_offset,
			.color = style->color,
		},
	});
}

void ui_end(UI* ui) {
	if (ui->mouse_button_pressed) {
		// unconsumed mouse press, reset the active item (it must be stale).
		ui->active_item = UI_ID_NULL;
	}
}

void ui_render(UI* ui, SL_Blitter* blitter) {
	// ui is built top-to-bottom
	for (s32 i = (s32)sl_blitter_command_seq_get_count(&ui->blitter_commands) - 1; i >= 0; i--) {
		sl_blitter_execute_command(blitter, sl_blitter_command_seq_get(&ui->blitter_commands, (u64)i));
	}
}
