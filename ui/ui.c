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

typedef struct UI_Element UI_Element;

typedef struct UI_Element_VTable {
	UI_Element* (*add_child)(UI* ui, UI_Element* self);
	UI_Extent (*get_extent)(UI* ui, UI_Element* self);
	void (*layout)(UI* ui, UI_Element* self);
	void (*render)(UI* ui, UI_Element* self, SL_Blitter* blitter);
} UI_Element_VTable;

typedef struct UI_Element {
	void* data;
	const UI_Element_VTable* vtable;

	Rect_f32 rect;
} UI_Element;

sl_seq(UI_Element, UI_Element_Seq, ui_element_seq);

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

typedef struct UI_Frame {
	Rect_f32 rect;
	UI_Element zstack;
} UI_Frame;
sl_seq(UI_Frame, UI_Frame_Seq, ui_frame_seq);

#define UI_MAX_DEPTH 64

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

	UI_Frame_Seq frames;

	UI_Element* stack[UI_MAX_DEPTH];
	u32 stack_length;

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
		.arena = sl_arena_allocator_new(allocator, 64ull * 1024ull),
		.frames = ui_frame_seq_new(allocator, 0),
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
	ui_frame_seq_clear(&ui->frames);

	ui->stack_length = 0;

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

void ui_draw_image(UI* ui, SL_Blitter* blitter, Rect_f32 rect, Rect_f32 atlas_rect, vec4_f32 color) {
	Textured_Quad_f32* quad;
	allocator_new(&ui->arena->allocator, quad, 1);
	*quad = textured_quad_for_sub_region_f32(rect, div_rect_vec_f32(atlas_rect, ui->texture_size), color);
	sl_blitter_draw_textured_quads(blitter, ui->texture, quad, 1);
}

void ui_pop(UI* ui) {
	sl_debug_assert(ui->stack_length > 0, "Nothing to pop.");
	if (ui->stack_length > 0) {
		ui->stack_length--;
	}
}

void ui_draw_nine_patch(UI* ui, SL_Blitter* blitter, Rect_f32 rect, UI_Nine_Patch nine_patch, vec4_f32 tint) {
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

	sl_blitter_draw_textured_quads(blitter, ui->texture, quads, next_quad);
}

// bool ui_button(UI* ui, UI_ID id, const UI_Button_Style* style, const char* label, Rect_f32 rect) {
// 	const u32 label_len = strlen(label);

// 	char* label_copy;
// 	allocator_new(&ui->arena->allocator, label_copy, label_len + 1);
// 	strcpy(label_copy, label);

// 	const Range_s32 font_y_range = sl_get_font_y_range(style->font);
// 	const Rect_s32 label_rect = sl_font_atlas_measure_string(style->font, label);
// 	const vec2_s32 label_size = size_rect_s32(label_rect);
// 	const vec2_f32 rect_size = size_rect_f32(rect);
// 	const vec2_f32 text_offset = {
// 		.x = rect.start.x + (rect_size.x * 0.5f) - (label_size.x * 0.5f),
// 		.y = rect.start.y + (rect_size.y * 0.5f) - (font_y_range.start * 0.5f),
// 	};

// 	const bool mouse_over = !ui->mouse_obscured && contains_rect_f32(rect, ui->mouse_position);
// 	if (mouse_over) {
// 		ui->hot_item = id;
// 		ui->mouse_obscured = true;
// 	}

// 	if (ui_id_is_null(ui->active_item) && (ui->mouse_button_down || ui->mouse_button_pressed) && mouse_over) {
// 		ui->active_item = id;
// 	}

// 	UI_Button_State button_state;
// 	if (ui_id_equals(ui->active_item, id)) {
// 		button_state = UI_Button_State_Active;
// 	} else if (ui_id_equals(ui->hot_item, id)) {
// 		button_state = UI_Button_State_Hot;
// 	} else {
// 		button_state = UI_Button_State_Normal;
// 	}
// 	const UI_Button_State_Style* state_style = &style->state[button_state];

// 	sl_blitter_command_seq_push(&ui->blitter_commands, (SL_Blitter_Command) {
// 		.kind = SL_Blitter_Command_Kind_Draw_Text,
// 		.draw_text = {
// 			.font = style->font,
// 			.string = label_copy,
// 			.position = add_vec2_f32(text_offset, state_style->label_offset),
// 			.color = state_style->label_color,
// 		},
// 	});

// 	ui_draw_nine_patch(ui, rect, state_style->backing, state_style->backing_color);

// 	if (ui_id_equals(ui->active_item, id) && ui->mouse_button_pressed) {
// 		ui->mouse_button_pressed = false;
// 		ui->active_item = UI_ID_NULL;
// 		return mouse_over;
// 	} else {
// 		return false;
// 	}
// }

// bool ui_slider_f32(UI* ui, UI_ID id, const UI_Slider_Style* style, f32* value, Range_f32 range, Rect_f32 rect) {
// 	const vec2_f32 rect_size = size_rect_f32(rect);
// 	const f32 center_y = (rect.end.y + rect.start.y) * 0.5f;
// 	const vec2_f32 needle_image_size = size_rect_f32(style->needle_image);

// 	// track layout
// 	const f32 track_lr_padding = ceil(needle_image_size.x * 0.5);
// 	const vec2_f32 track_rect_start = {
// 		rect.start.x + track_lr_padding,
// 		round(center_y - (style->track_height * 0.5f)),
// 	};
// 	const Rect_f32 track_rect = {
// 		.start = track_rect_start,
// 		.end = {
// 			rect.end.x - track_lr_padding,
// 			.y = track_rect_start.y + style->track_height,
// 		},
// 	};

// 	const bool mouse_over = !ui->mouse_obscured && contains_rect_f32(rect, ui->mouse_position);
// 	if (mouse_over) {
// 		ui->hot_item = id;
// 		ui->mouse_obscured = true;
// 	}

// 	if (ui_id_is_null(ui->active_item) && (ui->mouse_button_down || ui->mouse_button_pressed) && mouse_over) {
// 		ui->active_item = id;
// 	}

// 	const bool change_value = ui_id_equals(ui->active_item, id) && ui->mouse_button_down || ui->mouse_button_pressed;
// 	if (change_value) {
// 		ui->mouse_button_pressed = false;

// 		const f32 mouse_progress = saturate_f32((ui->mouse_position.x - track_rect.start.x) / (track_rect.end.x - track_rect.start.x));
// 		*value = lerp_f32(range.start, range.end, mouse_progress);
// 	}

// 	// needle
// 	const f32 progress = saturate_f32((*value - range.start) / (range.end - range.start));
// 	const vec2_f32 needle_offset = {
// 		.x = round(lerp_f32(track_rect.start.x, track_rect.end.x, progress) - (needle_image_size.x * 0.5f)),
// 		.y = round(center_y - (needle_image_size.y * 0.5f)),
// 	};
// 	const Rect_f32 needle_rect = {
// 		.start = needle_offset,
// 		.end = add_vec2_f32(needle_image_size, needle_offset),
// 	};
// 	ui_draw_image(ui, needle_rect, style->needle_image, style->needle_color);

// 	// track
// 	ui_draw_image(ui, track_rect, (Rect_f32) { .start = {} }, style->track_color);

// 	if (ui_id_equals(ui->active_item, id) && !ui->mouse_button_down) {
// 		ui->active_item = UI_ID_NULL;
// 	}

// 	return change_value;
// }

// void ui_label(UI* ui, UI_ID id, const UI_Label_Style* style, const char* label, Rect_f32 rect) {
// 	const u32 label_len = strlen(label);

// 	char* label_copy;
// 	allocator_new(&ui->arena->allocator, label_copy, label_len + 1);
// 	strcpy(label_copy, label);

// 	const Range_s32 font_y_range = sl_get_font_y_range(style->font);
// 	const vec2_f32 rect_size = size_rect_f32(rect);
// 	const vec2_f32 text_offset = {
// 		.x = rect.start.x,
// 		.y = rect.start.y + (rect_size.y * 0.5f) - (font_y_range.start * 0.5f),
// 	};

// 	sl_blitter_command_seq_push(&ui->blitter_commands, (SL_Blitter_Command) {
// 		.kind = SL_Blitter_Command_Kind_Draw_Text,
// 		.draw_text = {
// 			.font = style->font,
// 			.string = label_copy,
// 			.position = text_offset,
// 			.color = style->color,
// 		},
// 	});

Rect_f32 ui_padded_rect(Rect_f32 rect, UI_Padding padding) {
	return (Rect_f32) {
		.start = {
			rect.start.x + padding.left,
			rect.start.y + padding.top,
		},
		.end = {
			rect.end.x - padding.right,
			rect.end.y - padding.bottom,
		},
	};
}

// typedef struct UI_Padding_Element {
// 	UI_Padding padding;
// 	UI_Element* child;
// } UI_Padding_Element;

// void ui_add_padding(UI* ui, UI_Padding padding) {
// 	sl_debug_assert(ui->stack_length > 0, "Must be a container to pad.");

// 	UI_Padding_Element* padding_el;
// 	allocator_new(&ui->arena->allocator, padding_el, 1);
// 	*padding_el = (UI_Padding_Element) {
// 		.padding = padding,
// 		.child = ui->stack[ui->stack_length - 1],
// 	};
// 	UI_Element* element = ui_add_leaf(ui);
// 	*element = (UI_Element) {
// 		.data = hstack,
// 		.vtable = &ui_hstack_vtable,
// 	};
// }

// MARK: Distribute

typedef struct UI_Distribute_Item {
	f32 value;
	f32 min;
	f32 max;
} UI_Distribute_Item;

void ui_distribute(f32 size, f32 spacing, UI_Distribute_Item* items, u32 item_count) {
	f32 remaining = size;

	if (item_count > 0) {
		remaining -= spacing * (f32)(item_count - 1);
	}

	// 1: Assign min width to everything
	f32 non_fill_remaining = 0.0f;
	u32 fill_count = 0;
	for (u32 i = 0; i < item_count; i++) {
		// All elements start out at minimum size
		UI_Distribute_Item* item = &items[i];
		item->value = item->min;
		remaining -= item->min;

		if (item->max == UI_FILL) {
			fill_count++;
		} else {
			non_fill_remaining += item->max - item->min;
		}
	}

	// 2: Distribute remaining evenly to elements with max != UI_FILL
	if (non_fill_remaining > 0.0f) {
		const f32 amount_per_child = saturate_f32(remaining / non_fill_remaining);
		for (u32 i = 0; i < item_count; i++) {
			UI_Distribute_Item* item = &items[i];
			if (item->max != UI_FILL) {
				const f32 additional_width = (item->max - item->min) * amount_per_child;
				item->value += additional_width;
			}
		}
		remaining -= non_fill_remaining;
	}

	// 3: Give remaining width to elements with max == UI_FILL
	if (fill_count > 0) {
		const f32 amount_per_child = sl_max(remaining / (f32)fill_count, 0.0f);
		for (u32 i = 0; i < item_count; i++) {
			UI_Distribute_Item* item = &items[i];
			if (item->max == UI_FILL) {
				item->value += amount_per_child;
			}
		}
	}
}

UI_Extent ui_extent_combine(UI_Extent parent, UI_Extent child) {
	UI_Extent result = {
		.min_width = sl_max(parent.min_width, child.min_width),
		.min_height = sl_max(parent.min_height, child.min_height),
	};

	if (parent.max_width == UI_FILL) {
		result.max_width = UI_FILL;
	} else if (child.max_width == UI_FILL) {
		result.max_width = result.min_width;
	} else {
		result.max_width = sl_max(parent.max_width, child.max_width);
	}

	if (parent.max_height == UI_FILL) {
		result.max_height = UI_FILL;
	} else if (child.max_height == UI_FILL) {
		result.max_height = result.min_height;
	} else {
		result.max_height = sl_max(parent.max_height, child.max_height);
	}

	return result;
}

UI_Extent ui_extent_add_padding(UI_Extent extent, UI_Padding padding) {
	return (UI_Extent) {
		.min_width = extent.min_width + padding.left + padding.right,
		.min_height = extent.min_height + padding.top + padding.bottom,
		.max_width = extent.max_width + padding.left + padding.right,
		.max_height = extent.max_width + padding.top + padding.bottom,
	};
}

typedef struct UI_HStack {
	UI_Extent extent;
	UI_Padding padding;
	UI_Vertical_Alignment alignment;
	f32 spacing;
	UI_Element_Seq children;
} UI_HStack;

UI_Element* ui_hstack_add_child(UI* ui, UI_Element* self) {
	UI_HStack* hstack = self->data;
	return ui_element_seq_push_reserve(&hstack->children);
}
UI_Extent ui_hstack_get_extent(UI* ui, UI_Element* self) {
	UI_HStack* hstack = self->data;

	const u32 child_count = ui_element_seq_get_count(&hstack->children);

	UI_Extent children_extent = {0};
	if (child_count > 0) {
		for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
			UI_Element* child = ui_element_seq_get_ptr(&hstack->children, child_idx);
			const UI_Extent child_extent = child->vtable->get_extent(ui, child);

			children_extent.min_width += child_extent.min_width;
			if (child_extent.max_width != UI_FILL) {
				children_extent.max_width += child_extent.max_width;
			}

			children_extent.min_height = sl_max(children_extent.min_height, child_extent.min_height);
			if (child_extent.max_height != UI_FILL) {
				children_extent.max_height = sl_max(children_extent.max_height, child_extent.max_height);
			}
		}
		children_extent.max_width += hstack->spacing * (f32)(child_count - 1);
	}

	children_extent = ui_extent_add_padding(children_extent, hstack->padding);

	return ui_extent_combine(hstack->extent, children_extent);
}
void ui_hstack_layout(UI* ui, UI_Element* self) {
	UI_HStack* hstack = self->data;
	const u32 child_count = ui_element_seq_get_count(&hstack->children);

	const u64 initial_arena_position = sl_arena_allocator_get_position(ui->arena);

	UI_Distribute_Item* distribute_items;
	allocator_new(&ui->arena->allocator, distribute_items, child_count);

	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&hstack->children, child_idx);
		const UI_Extent child_extent = child->vtable->get_extent(ui, child);
		distribute_items[child_idx] = (UI_Distribute_Item) {
			.value = 0.0f,
			.min = child_extent.min_width,
			.max = child_extent.max_width,
		};
	}

	const Rect_f32 padded_rect = ui_padded_rect(self->rect, hstack->padding);
	const vec2_f32 padded_rect_size = size_rect_f32(padded_rect);
	ui_distribute(padded_rect_size.x, hstack->spacing, distribute_items, child_count);

	f32 current_x = padded_rect.start.x;
	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&hstack->children, child_idx);
		const UI_Distribute_Item* distribute_item = &distribute_items[child_idx];
		const UI_Extent child_extent = child->vtable->get_extent(ui, child);

		const f32 child_height = sl_clamp(padded_rect_size.y, child_extent.min_height, (child_extent.max_height == UI_FILL) ? padded_rect_size.y : child_extent.max_height);

		f32 offset_y;
		switch (hstack->alignment) {
			case UI_Vertical_Alignment_Top: {
				offset_y = padded_rect.start.y;
			} break;

			case UI_Vertical_Alignment_Center: {
				offset_y = padded_rect.start.y + (padded_rect_size.y * 0.5f) - (child_height * 0.5f);
			} break;

			case UI_Vertical_Alignment_Bottom: {
				offset_y = padded_rect.start.y + padded_rect_size.y - child_height;
			} break;
		}

		const f32 child_width = distribute_item->value;

		child->rect = (Rect_f32) {
			.start = { current_x, offset_y },
			.end = { current_x + child_width, offset_y + child_height },
		};

		current_x += child_width + hstack->spacing;
	}

	sl_arena_allocator_reset(ui->arena, initial_arena_position);

	// Now layout sub-children
	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&hstack->children, child_idx);
		if (child->vtable->layout) {
			child->vtable->layout(ui, child);
		}
	}
}

void ui_hstack_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_HStack* hstack = self->data;
	const u32 child_count = ui_element_seq_get_count(&hstack->children);
	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&hstack->children, child_idx);
		if (child->vtable->render) {
			child->vtable->render(ui, child, blitter);
		}
	}
}
const static UI_Element_VTable ui_hstack_vtable = {
	.add_child = ui_hstack_add_child,
	.get_extent = ui_hstack_get_extent,
	.layout = ui_hstack_layout,
	.render = ui_hstack_render
};

UI_Element* ui_add_leaf(UI* ui) {
	sl_debug_assert(ui->stack_length > 0, "Stack can't be empty.");
	UI_Element* parent = ui->stack[ui->stack_length - 1];
	return parent->vtable->add_child(ui, parent);
}

void ui_push_hstack(UI* ui, UI_Extent extent, UI_Padding padding, UI_Vertical_Alignment alignment, f32 spacing) {
	UI_HStack* hstack;
	allocator_new(&ui->arena->allocator, hstack, 1);
	*hstack = (UI_HStack) {
		.extent = extent,
		.padding = padding,
		.alignment = alignment,
		.spacing = spacing,
		.children = ui_element_seq_new(&ui->arena->allocator, 0),
	};
	UI_Element* element = ui_add_leaf(ui);
	*element = (UI_Element) {
		.data = hstack,
		.vtable = &ui_hstack_vtable,
	};
	ui->stack[ui->stack_length++] = element;
}

typedef struct UI_VStack {
	UI_Extent extent;
	UI_Padding padding;
	UI_Horizontal_Alignment alignment;
	f32 spacing;
	UI_Element_Seq children;
} UI_VStack;

UI_Element* ui_vstack_add_child(UI* ui, UI_Element* self) {
	UI_VStack* vstack = self->data;
	return ui_element_seq_push_reserve(&vstack->children);
}
UI_Extent ui_vstack_get_extent(UI* ui, UI_Element* self) {
	UI_VStack* vstack = self->data;

	const u32 child_count = ui_element_seq_get_count(&vstack->children);

	UI_Extent children_extent = {0};
	if (child_count > 0) {
		for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
			UI_Element* child = ui_element_seq_get_ptr(&vstack->children, child_idx);
			const UI_Extent child_extent = child->vtable->get_extent(ui, child);

			children_extent.min_height += child_extent.min_height;
			if (child_extent.max_height != UI_FILL) {
				children_extent.max_height += child_extent.max_height;
			}

			children_extent.min_width = sl_max(children_extent.min_width, child_extent.min_width);
			if (child_extent.max_width != UI_FILL) {
				children_extent.max_width = sl_max(children_extent.max_width, child_extent.max_width);
			}
		}
		children_extent.max_height += vstack->spacing * (f32)(child_count - 1);
	}

	children_extent = ui_extent_add_padding(children_extent, vstack->padding);

	return ui_extent_combine(vstack->extent, children_extent);
}
void ui_vstack_layout(UI* ui, UI_Element* self) {
	UI_VStack* vstack = self->data;
	const u32 child_count = ui_element_seq_get_count(&vstack->children);

	const u64 initial_arena_position = sl_arena_allocator_get_position(ui->arena);

	UI_Distribute_Item* distribute_items;
	allocator_new(&ui->arena->allocator, distribute_items, child_count);

	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&vstack->children, child_idx);
		const UI_Extent child_extent = child->vtable->get_extent(ui, child);
		distribute_items[child_idx] = (UI_Distribute_Item) {
			.value = 0.0f,
			.min = child_extent.min_height,
			.max = child_extent.max_height,
		};
	}

	const Rect_f32 padded_rect = ui_padded_rect(self->rect, vstack->padding);
	const vec2_f32 padded_rect_size = size_rect_f32(padded_rect);
	ui_distribute(padded_rect_size.y, vstack->spacing, distribute_items, child_count);

	f32 current_y = padded_rect.start.y;
	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&vstack->children, child_idx);
		const UI_Distribute_Item* distribute_item = &distribute_items[child_idx];
		const UI_Extent child_extent = child->vtable->get_extent(ui, child);

		const f32 child_width = sl_clamp(padded_rect_size.x, child_extent.min_width, (child_extent.max_width == UI_FILL) ? padded_rect_size.x : child_extent.max_width);

		f32 offset_x;
		switch (vstack->alignment) {
			case UI_Vertical_Alignment_Top: {
				offset_x = padded_rect.start.x;
			} break;

			case UI_Vertical_Alignment_Center: {
				offset_x = padded_rect.start.x + (padded_rect_size.x * 0.5f) - (child_width * 0.5f);
			} break;

			case UI_Vertical_Alignment_Bottom: {
				offset_x = padded_rect.start.x + padded_rect_size.x - child_width;
			} break;
		}

		const f32 child_height = distribute_item->value;

		child->rect = (Rect_f32) {
			.start = { offset_x, current_y },
			.end = { offset_x + child_width, current_y + child_height },
		};

		current_y += child_height + vstack->spacing;
	}

	sl_arena_allocator_reset(ui->arena, initial_arena_position);

	// Now layout sub-children
	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&vstack->children, child_idx);
		if (child->vtable->layout) {
			child->vtable->layout(ui, child);
		}
	}
}

void ui_vstack_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_VStack* vstack = self->data;
	const u32 child_count = ui_element_seq_get_count(&vstack->children);
	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&vstack->children, child_idx);
		if (child->vtable->render) {
			child->vtable->render(ui, child, blitter);
		}
	}
}
const static UI_Element_VTable ui_vstack_vtable = {
	.add_child = ui_vstack_add_child,
	.get_extent = ui_vstack_get_extent,
	.layout = ui_vstack_layout,
	.render = ui_vstack_render
};

void ui_push_vstack(UI* ui, UI_Extent extent, UI_Padding padding, UI_Horizontal_Alignment alignment, f32 spacing) {
	UI_VStack* vstack;
	allocator_new(&ui->arena->allocator, vstack, 1);
	*vstack = (UI_VStack) {
		.extent = extent,
		.padding = padding,
		.alignment = alignment,
		.spacing = spacing,
		.children = ui_element_seq_new(&ui->arena->allocator, 0),
	};
	UI_Element* element = ui_add_leaf(ui);
	*element = (UI_Element) {
		.data = vstack,
		.vtable = &ui_vstack_vtable,
	};
	ui->stack[ui->stack_length++] = element;
}

UI_Extent ui_spacer_get_extent(UI* ui, UI_Element* self) {
	return (UI_Extent) {
		.min_width = 0.0f,
		.max_width = UI_FILL,
		.min_height = 0.0f,
		.max_height = UI_FILL,
	};
}
const static UI_Element_VTable ui_spacer_vtable = {
	.get_extent = ui_spacer_get_extent,
};
void ui_spacer(UI* ui) {
	UI_Element* element = ui_add_leaf(ui);
	*element = (UI_Element) {
		.data = NULL,
		.vtable = &ui_spacer_vtable,
	};
}

typedef struct UI_ZStack {
	UI_Extent extent;
	UI_Padding padding;
	UI_Element_Seq children;
} UI_ZStack;

UI_Element* ui_zstack_add_child(UI* ui, UI_Element* self) {
	UI_ZStack* zstack = self->data;
	return ui_element_seq_push_reserve(&zstack->children);
}
UI_Extent ui_zstack_get_extent(UI* ui, UI_Element* self) {
	UI_ZStack* zstack = self->data;

	UI_Extent children_extent = {0};
	const u32 child_count = ui_element_seq_get_count(&zstack->children);
	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&zstack->children, child_idx);
		const UI_Extent child_extent = child->vtable->get_extent(ui, child);
		children_extent.min_width = sl_max(children_extent.min_width, child_extent.min_width);
		children_extent.min_height = sl_max(children_extent.min_height, child_extent.min_height);

		if (child_extent.max_width != UI_FILL) {
			children_extent.max_width = sl_max(children_extent.max_width, child_extent.max_width);
		}
		if (child_extent.max_height != UI_FILL) {
			children_extent.max_height = sl_max(children_extent.max_height, child_extent.max_height);
		}
	}

	children_extent = ui_extent_add_padding(children_extent, zstack->padding);

	return ui_extent_combine(zstack->extent, children_extent);
}
void ui_zstack_layout(UI* ui, UI_Element* self) {
	UI_ZStack* zstack = self->data;

	const Rect_f32 padded_rect = ui_padded_rect(self->rect, zstack->padding);
	const vec2_f32 padded_rect_size = size_rect_f32(padded_rect);
	const vec2_f32 padded_rect_center = centre_rect_f32(padded_rect);

	const u32 child_count = ui_element_seq_get_count(&zstack->children);
	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&zstack->children, child_idx);
		const UI_Extent child_extent = child->vtable->get_extent(ui, child);

		const vec2_f32 child_size = {
			sl_clamp(padded_rect_size.x, child_extent.min_width, (child_extent.max_width == UI_FILL) ? padded_rect_size.x : child_extent.max_width),
			sl_clamp(padded_rect_size.y, child_extent.min_height, (child_extent.max_height == UI_FILL) ? padded_rect_size.y : child_extent.max_height),
		};

		const vec2_f32 child_offset = sub_vec2_f32(padded_rect_center, mul_vec2_f32(child_size, (vec2_f32) { 0.5f, 0.5f }));

		const Rect_f32 child_rect = {
			.start = child_offset,
			.end = add_vec2_f32(child_offset, child_size),
		};

		child->rect = child_rect;
		if (child->vtable->layout) {
			child->vtable->layout(ui, child);
		}
	}
}
void ui_zstack_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_ZStack* zstack = self->data;
	const u32 child_count = ui_element_seq_get_count(&zstack->children);
	for (u32 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&zstack->children, child_idx);
		if (child->vtable->render) {
			child->vtable->render(ui, child, blitter);
		}
	}
}
const static UI_Element_VTable ui_zstack_vtable = {
	.add_child = ui_zstack_add_child,
	.get_extent = ui_zstack_get_extent,
	.layout = ui_zstack_layout,
	.render = ui_zstack_render,
};

UI_Element ui_zstack_new(UI* ui, UI_Extent extent, UI_Padding padding) {
	UI_ZStack* zstack;
	allocator_new(&ui->arena->allocator, zstack, 1);
	*zstack = (UI_ZStack) {
		.extent = extent,
		.padding = padding,
		.children = ui_element_seq_new(&ui->arena->allocator, 0),
	};
	UI_Element element = {
		.data = zstack,
		.vtable = &ui_zstack_vtable,
	};
	return element;
}

void ui_push_zstack(UI* ui, UI_Extent extent, UI_Padding padding) {
	UI_Element* element = ui_add_leaf(ui);
	*element = ui_zstack_new(ui, extent, padding);
	ui->stack[ui->stack_length++] = element;
}

void ui_begin_frame(UI* ui, Rect_f32 rect) {
	sl_debug_assert(ui->stack_length == 0, "Must have an empty stack to begin a new frame.");
	UI_Frame* frame = ui_frame_seq_push_reserve(&ui->frames);
	*frame = (UI_Frame) {
		.rect = rect,
		.zstack = ui_zstack_new(ui, UI_EXTENT_FILL, UI_PADDING_NONE),
	};

	ui->stack_length = 0;
	ui->stack[ui->stack_length++] = &frame->zstack;
}
void ui_end_frame(UI* ui) {
	sl_debug_assert(ui->stack_length == 1, "Stack should only contain frame's zstack.");
	ui->stack_length = 0;
}

typedef struct UI_Color {
	UI_Extent extent;
	vec4_f32 color;
} UI_Color;

UI_Extent ui_color_get_extent(UI* ui, UI_Element* self) {
	UI_Color* color = self->data;
	return color->extent;
}
void ui_color_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_Color* color = self->data;
	ui_draw_image(ui, blitter, self->rect, (Rect_f32){0}, color->color);
}
const static UI_Element_VTable ui_color_vtable = {
	.get_extent = ui_color_get_extent,
	.render = ui_color_render,
};

void ui_color(UI* ui, UI_Extent extent, vec4_f32 color) {
	UI_Color* color_el;
	allocator_new(&ui->arena->allocator, color_el, 1);
	*color_el = (UI_Color) {
		.extent = extent,
		.color = color,
	};
	UI_Element* element = ui_add_leaf(ui);
	*element = (UI_Element) {
		.data = color_el,
		.vtable = &ui_color_vtable,
	};
}
void ui_button(UI* ui, UI_ID id, UI_Extent extent, const UI_Button_Style* style, const char* label, UI_Callback on_press) {
	// UI_Element* element = ui_element_seq_push_reserve(&ui->elements);
	// *element = (UI_Element) {
	// 	.kind = UI_Element_Kind_Button,
	// 	.parent = ui_top(ui),
	// 	.extent = extent,
	// 	.button = {
	// 		// todo
	// 	},
	// };
}
void ui_slider_f32(UI* ui, UI_ID id, UI_Extent extent, const UI_Slider_Style* style, f32* value, Range_f32 range, UI_Callback on_change) {

}
void ui_label(UI* ui, UI_ID id, UI_Extent extent, const UI_Label_Style* style, const char* label) {

}

void ui_calculate_extents_recurse(UI_Element* element) {

}

void ui_layout_frames(UI* ui) {
	const u32 frame_count = ui_frame_seq_get_count(&ui->frames);
	for (u32 i = 0; i < frame_count; i++) {
		UI_Frame* frame = ui_frame_seq_get_ptr(&ui->frames, i);
		frame->zstack.rect = frame->rect;
		frame->zstack.vtable->layout(ui, &frame->zstack);
	}
}

void ui_end(UI* ui) {
	ui_layout_frames(ui);

	if (ui->mouse_button_pressed) {
		// unconsumed mouse press, reset the active item (it must be stale).
		ui->active_item = UI_ID_NULL;
	}
}

void ui_render(UI* ui, SL_Blitter* blitter) {
	const u32 frame_count = ui_frame_seq_get_count(&ui->frames);
	for (u32 i = 0; i < frame_count; i++) {
		UI_Frame* frame = ui_frame_seq_get_ptr(&ui->frames, i);
		frame->zstack.vtable->render(ui, &frame->zstack, blitter);
	}
}
