#include "stanlib/gpu.h"
#include <stanlib/core.h>
#include <stanlib/ui.h>
#include <stanlib/blitter.h>
#include <stanlib/text.h>

#include <string.h>

// MARK: ID

UI_ID ui_id_internal(UI_ID parent, u32 item, u32 index, bool internal) {
	SL_Hasher hasher;
	sl_hasher_init(&hasher);
	sl_hasher_push(&hasher, immutable_buffer_for(internal));
	sl_hasher_push(&hasher, immutable_buffer_for(parent.hash));
	sl_hasher_push(&hasher, immutable_buffer_for(item));
	sl_hasher_push(&hasher, immutable_buffer_for(index));
	return (UI_ID) {
		.hash = sl_hasher_finalise(&hasher),
	};
}

UI_ID ui_id(UI_ID parent, u32 item, u32 index) {
	return ui_id_internal(parent, item, index, false);
}
u64 ui_id_get_hash(UI_ID id) {
	return id.hash;
}
bool ui_id_equals(UI_ID a, UI_ID b) {
	return a.hash == b.hash;
}
bool ui_id_is_null(UI_ID a) {
	return a.hash == 0;
}

// MARK: Gesture

typedef struct UI_Gesture {
	Allocator* allocator;

	const UI_Gesture_VTable* vtable;

	void* ctx;

	UI_Element* element;

	// The frame index, at the time the gesture was created.
	u64 init_frame_index;
} UI_Gesture;

typedef struct UI_Persistent_State {
	bool exists;
	UI_ID id;
	u64 last_access_frame;
	u64 generation;
	void* data;
	void (*destroy)(void* data);
} UI_Persistent_State;

typedef struct UI_Persistent_State_Handle {
	UI_Persistent_State* state;
	u64 generation;
} UI_Persistent_State_Handle;

sl_seq(UI_Persistent_State, UI_Persistent_State_Seq, ui_persistent_state_seq);
sl_seq(UI_Persistent_State*, UI_Persistent_State_Ptr_Seq, ui_persistent_state_ptr_seq);
sl_seq(UI_Persistent_State_Handle, UI_Persistent_State_Handle_Seq, ui_persistent_state_handle_seq);
sl_hashmap(UI_ID, UI_Persistent_State*, UI_Persistent_State_Map, ui_persistent_state_map, ui_id_get_hash, ui_id_equals);

typedef struct UI_Persistent_Store {
	Allocator* allocator;
	UI_Persistent_State_Seq states;
	UI_Persistent_State_Ptr_Seq freelist;
	UI_Persistent_State_Map map;
} UI_Persistent_Store;

void ui_persistent_store_init(UI_Persistent_Store* store, Allocator* allocator) {
	*store = (UI_Persistent_Store) {
		.allocator = allocator,
		.states = ui_persistent_state_seq_new(allocator, 64),
		.freelist = ui_persistent_state_ptr_seq_new(allocator, 64),
		.map = ui_persistent_state_map_new(allocator, 64),
	};
}
void ui_persistent_store_deinit(UI_Persistent_Store* store) {
	ui_persistent_state_seq_destroy(&store->states);
	ui_persistent_state_ptr_seq_destroy(&store->freelist);
	ui_persistent_state_map_destroy(&store->map);
	*store = (UI_Persistent_Store) {0};
}
UI_Persistent_State* ui_persistent_store_acquire(UI_Persistent_Store* store, UI_ID id, u64 frame) {
	UI_Persistent_State* value;
	if (ui_persistent_state_map_get(&store->map, id, &value)) {
		value->last_access_frame = frame;
		return value;
	} else {
		if (ui_persistent_state_ptr_seq_pop(&store->freelist, &value)) {
			const u64 new_generation = value->generation + 1;
			*value = (UI_Persistent_State) {
				.exists = true,
				.id = id,
				.last_access_frame = frame,
				.generation = new_generation,
			};
		} else {
			value = ui_persistent_state_seq_push_reserve(&store->states);
			*value = (UI_Persistent_State) {
				.exists = true,
				.id = id,
				.last_access_frame = frame,
				.generation = 0,
			};
		}
		ui_persistent_state_map_insert(&store->map, id, value);
		return value;
	}
}
void ui_persistent_store_purge(UI_Persistent_Store* store, u64 min_frame) {
	const u64 state_count = ui_persistent_state_seq_get_count(&store->states);
	for (u64 i = 0; i < state_count; i++) {
		UI_Persistent_State* state = ui_persistent_state_seq_get_ptr(&store->states, i);
		if (state->exists && (state->last_access_frame < min_frame)) {
			if (state->destroy) {
				state->destroy(state->data);
			}

			const u64 new_generation = state->generation + 1;
			*state = (UI_Persistent_State) {
				.exists = false,
				.generation = new_generation,
			};
		}
	}
}
UI_Persistent_State_Handle ui_persistent_state_get_handle(UI_Persistent_State* state) {
	return (UI_Persistent_State_Handle) {
		.state = state,
		.generation = state->generation,
	};
}
UI_Persistent_State* ui_persistent_state_handle_resolve(UI_Persistent_State_Handle handle) {
	return (handle.state->generation == handle.generation) ? handle.state : NULL;
}

// MARK: Touch

typedef struct UI_Touch {
	UI_Touch_ID id;

	UI_Touch_State state;

	vec2_f32 position;

	f64 timestamp;

	u32 rc;

	// Gestures that receive updates relating to the touch.
	// Note: receivers could become null if the gesture no longer exists.
	UI_Persistent_State_Handle_Seq receivers;
} UI_Touch;

u64 ui_touch_id_hash(UI_Touch_ID id) {
	SL_Hasher hasher;
	sl_hasher_init(&hasher);
	sl_hasher_push(&hasher, immutable_buffer_for(id.kind));
	sl_hasher_push(&hasher, immutable_buffer_for(id.external));
	return sl_hasher_finalise(&hasher);
}
bool ui_touch_id_equals(UI_Touch_ID a, UI_Touch_ID b) {
	return (a.kind == b.kind) && (a.external == b.external);
}
sl_seq(UI_Touch, UI_Touch_Seq, ui_touch_seq);
sl_seq(UI_Touch*, UI_Touch_Ptr_Seq, ui_touch_ptr_seq);
sl_hashmap(UI_Touch_ID, UI_Touch*, UI_Touch_Map, ui_touch_map, ui_touch_id_hash, ui_touch_id_equals);

// MARK: Element

typedef struct UI_Element UI_Element;

typedef struct UI_Element_Seq UI_Element_Seq;

typedef struct UI_Element_VTable {
	UI_Element* (*add_child)(UI* ui, UI_Element* self);
	UI_Extent (*get_extent)(UI* ui, UI_Element* self);
	UI_Element_Seq* (*get_children)(UI* ui, UI_Element* self);
	void (*layout)(UI* ui, UI_Element* self);
	void (*render)(UI* ui, UI_Element* self, SL_Blitter* blitter);
} UI_Element_VTable;

typedef struct UI_Element {
	void* data;
	const UI_Element_VTable* vtable;

	bool culled;

	// Derived during layout
	Rect_f32 rect;

	UI_Persistent_State_Ptr_Seq gestures;
} UI_Element;

sl_seq(UI_Element, UI_Element_Seq, ui_element_seq);

#define UI_STACK_SIZE 32

typedef struct UI_Frame {
	Rect_f32 rect;
	UI_Element zstack;
} UI_Frame;
sl_seq(UI_Frame, UI_Frame_Seq, ui_frame_seq);

typedef enum UI_Lifecycle_State {
	UI_Lifecycle_State_Began,
	UI_Lifecycle_State_Ended,
} UI_Lifecycle_State;

#define UI_MAX_DEPTH 64

typedef struct UI {
	Allocator* allocator;

	u64 frame_index;

	UI_Lifecycle_State lifecycle_state;

	UI_Frame_Seq frames;

	UI_Element* stack[UI_MAX_DEPTH];
	u32 stack_length;

	SL_Arena_Allocator* arena;

	// Touch tracking
	UI_Touch_Map active_touch_map;
	UI_Touch_Seq touches;
	UI_Touch_Ptr_Seq touch_freelist;

	UI_Persistent_Store persistent_store;
} UI;

UI* ui_new(Allocator* allocator) {
	UI* ui;
	allocator_new(allocator, ui, 1);
	*ui = (UI) {
		.allocator = allocator,
		.frame_index = 0ull,
		.lifecycle_state = UI_Lifecycle_State_Ended,
		.arena = sl_arena_allocator_new(allocator, 64ull * 1024ull),
		.frames = ui_frame_seq_new(allocator, 0),
		.active_touch_map = ui_touch_map_new(allocator, 32),
		.touches = ui_touch_seq_new(allocator, 32),
		.touch_freelist = ui_touch_ptr_seq_new(allocator, 32),
	};
	ui_persistent_store_init(&ui->persistent_store, allocator);
	return ui;
}
void ui_destroy(UI* ui) {
	Allocator* allocator = ui->allocator;
	ui_touch_map_destroy(&ui->active_touch_map);
	ui_touch_seq_destroy(&ui->touches);
	ui_touch_ptr_seq_destroy(&ui->touch_freelist);
	ui_persistent_store_deinit(&ui->persistent_store);
	*ui = (UI) {0};
	allocator_free(allocator, ui, 1);
}

// MARK: Gesture

UI_Gesture* ui_persistent_state_handle_resolve_gesture(UI_Persistent_State_Handle handle) {
	UI_Persistent_State* state = ui_persistent_state_handle_resolve(handle);
	return state ? state->data : state;
}

// MARK: Touch

UI_Touch* ui_touch_new(UI* ui, UI_Touch_ID id) {
	UI_Touch* touch;
	if (!ui_touch_ptr_seq_pop(&ui->touch_freelist, &touch)) {
		touch = ui_touch_seq_push_reserve(&ui->touches);
		*touch = (UI_Touch) {
			.receivers = ui_persistent_state_handle_seq_new(ui->allocator, 8),
		};
	}

	touch->state = UI_Touch_State_Alive;
	touch->position = (vec2_f32) {};
	touch->id = id;
	touch->rc = 1;

	ui_touch_map_insert(&ui->active_touch_map, id, touch);

	return touch;
}
void ui_touch_retain(UI* ui, UI_Touch* touch) {
	sl_debug_assert(touch->rc > 0, "RC must be > 0 to retain.");
	touch->rc++;
}
void ui_touch_release(UI* ui, UI_Touch* touch) {
	sl_debug_assert(touch->rc > 0, "RC must be > 0 to release.");
	touch->rc--;

	if (touch->rc == 0) {
		touch->id = (UI_Touch_ID) {0};
		ui_persistent_state_handle_seq_clear(&touch->receivers);
		ui_touch_ptr_seq_push(&ui->touch_freelist, touch);
	}
}
void ui_touch_began(UI* ui, UI_Touch* touch) {
	sl_debug_assert(touch->state == UI_Touch_State_Alive, "Only alive touches can begin.");
	for (u32 receiver_idx = 0; receiver_idx < ui_persistent_state_handle_seq_get_count(&touch->receivers); receiver_idx++) {
		const UI_Persistent_State_Handle gesture_handle = ui_persistent_state_handle_seq_get(&touch->receivers, receiver_idx);
		UI_Gesture* gesture = ui_persistent_state_handle_resolve_gesture(gesture_handle);
		if (gesture != NULL) {
			gesture->vtable->touch_began(ui, gesture->ctx, touch);
		}
	}
}
void ui_touch_changed(UI* ui, UI_Touch* touch) {
	sl_debug_assert(touch->state == UI_Touch_State_Alive, "Only alive touches can change.");
	for (u32 receiver_idx = 0; receiver_idx < ui_persistent_state_handle_seq_get_count(&touch->receivers); receiver_idx++) {
		const UI_Persistent_State_Handle gesture_handle = ui_persistent_state_handle_seq_get(&touch->receivers, receiver_idx);
		UI_Gesture* gesture = ui_persistent_state_handle_resolve_gesture(gesture_handle);
		if (gesture != NULL) {
			gesture->vtable->touch_changed(ui, gesture->ctx, touch);
		}
	}
}
void ui_touch_ended(UI* ui, UI_Touch* touch) {
	sl_debug_assert(touch->state == UI_Touch_State_Alive, "Can only end an alive touch.");
	touch->state = UI_Touch_State_Ended;
	for (u32 receiver_idx = 0; receiver_idx < ui_persistent_state_handle_seq_get_count(&touch->receivers); receiver_idx++) {
		const UI_Persistent_State_Handle gesture_handle = ui_persistent_state_handle_seq_get(&touch->receivers, receiver_idx);
		UI_Gesture* gesture = ui_persistent_state_handle_resolve_gesture(gesture_handle);
		if (gesture != NULL) {
			gesture->vtable->touch_ended(ui, gesture->ctx, touch);
		}
	}

	sl_unused bool removed = ui_touch_map_remove(&ui->active_touch_map, touch->id);
	sl_debug_assert(removed, "Should have removed touch from alive map.");

	ui_touch_release(ui, touch);
}
void ui_touch_cancelled(UI* ui, UI_Touch* touch) {
	sl_debug_assert(touch->state == UI_Touch_State_Alive, "Can only end an alive touch.");
	touch->state = UI_Touch_State_Cancelled;
	for (u32 receiver_idx = 0; receiver_idx < ui_persistent_state_handle_seq_get_count(&touch->receivers); receiver_idx++) {
		const UI_Persistent_State_Handle gesture_handle = ui_persistent_state_handle_seq_get(&touch->receivers, receiver_idx);
		UI_Gesture* gesture = ui_persistent_state_handle_resolve_gesture(gesture_handle);
		if (gesture != NULL) {
			gesture->vtable->touch_cancelled(ui, gesture->ctx, touch);
		}
	}

	sl_unused bool removed = ui_touch_map_remove(&ui->active_touch_map, touch->id);
	sl_debug_assert(removed, "Should have removed touch from alive map.");

	ui_touch_release(ui, touch);
}

void ui_draw_image(UI* ui, SL_Blitter* blitter, Gpu_Texture texture, Rect_f32 rect, Rect_f32 atlas_rect, vec4_f32 color) {
	Textured_Quad_f32* quad;
	allocator_new(&ui->arena->allocator, quad, 1);

	const Gpu_Texture_Desc* texture_desc = gpu_get_texture_desc(texture);
	const vec2_f32 texture_size = { (f32)texture_desc->size.x, (f32)texture_desc->size.y };

	*quad = textured_quad_for_sub_region_f32(rect, div_rect_vec_f32(atlas_rect, texture_size), color);
	sl_blitter_draw_textured_quads(blitter, texture, quad, 1);
}

void ui_pop(UI* ui) {
	sl_debug_assert(ui->stack_length > 0, "Nothing to pop.");
	if (ui->stack_length > 0) {
		ui->stack_length--;
	}
}

void ui_draw_nine_patch(UI* ui, SL_Blitter* blitter, Gpu_Texture texture, Rect_f32 rect, UI_Nine_Patch nine_patch, vec4_f32 tint) {
	Textured_Quad_f32* quads;
	allocator_new(&ui->arena->allocator, quads, 9);

	const Gpu_Texture_Desc* texture_desc = gpu_get_texture_desc(texture);
	const vec2_f32 texture_size = { (f32)texture_desc->size.x, (f32)texture_desc->size.y };

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
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, texture_size), tint);
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
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, texture_size), tint);
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
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, texture_size), tint);
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
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, texture_size), tint);
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
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, texture_size), tint);
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
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, texture_size), tint);
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
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, texture_size), tint);
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
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, texture_size), tint);
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
		quads[next_quad++] = textured_quad_for_sub_region_f32(patch_rect, div_rect_vec_f32(patch_uv_rect, texture_size), tint);
	}

	sl_blitter_draw_textured_quads(blitter, texture, quads, next_quad);
}

// MARK: Rect

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

Rect_f32 ui_rect_snap_to_pixels(Rect_f32 rect) {
	return (Rect_f32) {
		.start = { roundf(rect.start.x), roundf(rect.start.y) },
		.end = { roundf(rect.end.x), roundf(rect.end.y) },
	};
}

// MARK: Distribute

typedef struct UI_Distribute_Item {
	f32 value;
	f32 min;
	f32 max;
} UI_Distribute_Item;

void ui_distribute(f32 size, f32 spacing, UI_Distribute_Item* items, u64 item_count) {
	f32 remaining = size;

	if (item_count > 0) {
		remaining -= spacing * (f32)(item_count - 1);
	}

	// 1: Assign min width to everything
	f32 non_fill_remaining = 0.0f;
	u64 fill_count = 0;
	for (u64 i = 0; i < item_count; i++) {
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
		for (u64 i = 0; i < item_count; i++) {
			UI_Distribute_Item* item = &items[i];
			if (item->max == UI_FILL) {
				item->value += amount_per_child;
			}
		}
	}
}

// MARK: Extent

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
		.max_height = extent.max_height + padding.top + padding.bottom,
	};
}

// MARK: Element Common

bool ui_element_get_layout_rect(UI* ui, UI_Element* element, Rect_f32* out_rect) {
	if ((ui->lifecycle_state == UI_Lifecycle_State_Ended) && !element->culled) {
		*out_rect = element->rect;
		return true;
	} else {
		return false;
	}
}

void ui_gesture_destroy(void* ctx) {
	UI_Gesture* gesture = ctx;
	Allocator* allocator = gesture->allocator;
	allocator_free(allocator, gesture, 1);
}
UI_Gesture* ui_gesture_new(UI* ui, UI_ID id, const UI_Gesture_VTable* vtable, UI_Element* element) {
	UI_Persistent_State* persistent_state = ui_persistent_store_acquire(&ui->persistent_store, id, ui->frame_index);
	if (persistent_state->data == NULL) {
		UI_Gesture* gesture;
		allocator_new(ui->allocator, gesture, 1);
		*gesture = (UI_Gesture) {
			.allocator = ui->allocator,
			.ctx = NULL,
			.element = element,
			.vtable = vtable,
			.init_frame_index = ui->frame_index
		};
		persistent_state->data = gesture;
		persistent_state->destroy = ui_gesture_destroy;
	} else {
		UI_Gesture* gesture = persistent_state->data;
		gesture->vtable = vtable;
		gesture->element = element;
	}

	ui_persistent_state_ptr_seq_push(&element->gestures, persistent_state);

	return persistent_state->data;
}

// MARK: Pan Gesture

typedef struct UI_Pan_Gesture_State {
	UI_Pan_Gesture_Desc desc;
	UI_Touch* active_touch;
	vec2_f32 initial_position;
	vec2_f32 previous_position;
	vec2_f32 smoothed_velocity;
	f32 total_movement;
	f64 previous_timestamp;
} UI_Pan_Gesture_State;

UI_Pan_Gesture_Frame ui_pan_gesture_resolve_frame(UI_Pan_Gesture_State* pan, UI_Gesture_State state) {
	const f32 dt = (pan->active_touch->timestamp - pan->previous_timestamp);
	if (dt > 0.0) {
		const f32 tau = 0.03f;
		const f32 alpha = 1.0f - expf(-dt / tau);

		const vec2_f32 raw_velocity = div_vec2_f32(sub_vec2_f32(pan->active_touch->position, pan->previous_position), splat_vec2_f32(dt));

		pan->smoothed_velocity = add_vec2_f32(pan->smoothed_velocity, mul_vec2_f32(sub_vec2_f32(raw_velocity, pan->smoothed_velocity), splat_vec2_f32(alpha)));
	}

	pan->total_movement += dist_vec2_f32(pan->previous_position, pan->active_touch->position);

	return (UI_Pan_Gesture_Frame) {
		.state = state,
		.position = pan->active_touch->position,
		.translation = sub_vec2_f32(pan->active_touch->position, pan->initial_position),
		.velocity = pan->smoothed_velocity,
		.total_movement = pan->total_movement,
	};
}
void ui_pan_gesture_touch_began(UI* ui, void* ctx, UI_Touch* touch) {
	UI_Pan_Gesture_State* state = ctx;
	if (state->active_touch == NULL) {
		ui_touch_retain(ui, touch);
		state->active_touch = touch;
		state->initial_position = touch->position;
		state->previous_position = touch->position;
		state->previous_timestamp = touch->timestamp;
		state->total_movement = 0.0f;

		const UI_Pan_Gesture_Frame frame = ui_pan_gesture_resolve_frame(state, UI_Gesture_State_Began);
		state->desc.callback.func(state->desc.callback.ctx, &frame);
	}
}
void ui_pan_gesture_touch_changed(UI* ui, void* ctx, UI_Touch* touch) {
	UI_Pan_Gesture_State* state = ctx;
	if (state->active_touch == touch) {
		const UI_Pan_Gesture_Frame frame = ui_pan_gesture_resolve_frame(state, UI_Gesture_State_Changed);
		state->previous_position = touch->position;
		state->previous_timestamp = touch->timestamp;
		state->desc.callback.func(state->desc.callback.ctx, &frame);
	}
}
void ui_pan_gesture_touch_ended(UI* ui, void* ctx, UI_Touch* touch) {
	UI_Pan_Gesture_State* state = ctx;
	if (state->active_touch == touch) {
		const UI_Pan_Gesture_Frame frame = ui_pan_gesture_resolve_frame(state, UI_Gesture_State_Ended);
		state->desc.callback.func(state->desc.callback.ctx, &frame);

		ui_touch_release(ui, touch);
		state->active_touch = NULL;
	}
}
void ui_pan_gesture_touch_cancelled(UI* ui, void* ctx, UI_Touch* touch) {
	UI_Pan_Gesture_State* state = ctx;
	if (state->active_touch == touch) {
		const UI_Pan_Gesture_Frame frame = ui_pan_gesture_resolve_frame(state, UI_Gesture_State_Cancelled);
		state->desc.callback.func(state->desc.callback.ctx, &frame);

		ui_touch_release(ui, touch);
		state->active_touch = NULL;
	}
}
void ui_pan_gesture_destroy(UI* ui, void* ctx) {
	UI_Pan_Gesture_State* state = ctx;
	if (state->active_touch != NULL) {
		ui_touch_release(ui, state->active_touch);
		state->active_touch = NULL;
	}
	allocator_free(ui->allocator, state, 1);
}
const static UI_Gesture_VTable ui_pan_gesture_vtable = {
	.touch_began = ui_pan_gesture_touch_began,
	.touch_changed = ui_pan_gesture_touch_changed,
	.touch_ended = ui_pan_gesture_touch_ended,
	.touch_cancelled = ui_pan_gesture_touch_cancelled,
	.destroy = ui_pan_gesture_destroy,
};

void ui_pan_gesture(UI* ui, UI_ID id, const UI_Pan_Gesture_Desc* desc, UI_Element* element) {
	UI_Gesture* gesture = ui_gesture_new(ui, id, &ui_pan_gesture_vtable, element);

	if (gesture->ctx == NULL) {
		// Allocate state
		UI_Pan_Gesture_State* state;
		allocator_new(ui->allocator, state, 1);
		*state = (UI_Pan_Gesture_State) {0};
		gesture->ctx = state;
	}

	UI_Pan_Gesture_State* state = gesture->ctx;
	state->desc = *desc;
}

UI_Element* ui_add_leaf(UI* ui) {
	sl_debug_assert(ui->stack_length > 0, "Stack can't be empty.");
	UI_Element* parent = ui->stack[ui->stack_length - 1];
	return parent->vtable->add_child(ui, parent);
}

void ui_trigger_callback(UI_Callback callback) {
	if (callback.func) {
		callback.func(callback.ctx);
	}
}

// MARK: Text Measurements

typedef struct UI_Text_Measurements {
	Range_s32 y_range;
	vec2_s32 size;
} UI_Text_Measurements;

UI_Text_Measurements ui_text_measurements(SL_Font* font, const char* string) {
	const Range_s32 font_y_range = sl_font_get_y_range(font);
	const Rect_s32 label_rect = sl_font_measure_string(font, string);
	const vec2_s32 label_size = rect_size_s32(label_rect);
	return (UI_Text_Measurements) {
		.size = label_size,
		.y_range = font_y_range,
	};
}

UI_Extent ui_text_measurements_get_extent(const UI_Text_Measurements* measurements) {
	return (UI_Extent) {
		.min_width = measurements->size.x,
		.max_width = measurements->size.x,
		.min_height = measurements->y_range.end - measurements->y_range.start,
		.max_height = measurements->y_range.end - measurements->y_range.start,
	};
}

// MARK: HStack

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

	const u64 child_count = ui_element_seq_get_count(&hstack->children);

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
	const u64 child_count = ui_element_seq_get_count(&hstack->children);

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
	const vec2_f32 padded_rect_size = rect_size_f32(padded_rect);
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
		child->rect = ui_rect_snap_to_pixels(child->rect);

		current_x += child_width + hstack->spacing;
	}

	sl_arena_allocator_reset(ui->arena, initial_arena_position);

	// Now layout sub-children
	for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&hstack->children, child_idx);
		if (child->vtable->layout) {
			child->vtable->layout(ui, child);
		}
	}
}
void ui_hstack_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_HStack* hstack = self->data;
	const u64 child_count = ui_element_seq_get_count(&hstack->children);
	for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&hstack->children, child_idx);
		if (child->vtable->render) {
			child->vtable->render(ui, child, blitter);
		}
	}
}
UI_Element_Seq* ui_hstack_get_children(UI* ui, UI_Element* self) {
	UI_HStack* hstack = self->data;
	return &hstack->children;
}

const static UI_Element_VTable ui_hstack_vtable = {
	.add_child = ui_hstack_add_child,
	.get_extent = ui_hstack_get_extent,
	.get_children = ui_hstack_get_children,
	.layout = ui_hstack_layout,
	.render = ui_hstack_render,
};

UI_Element* ui_push_hstack(UI* ui, UI_Extent extent, UI_Padding padding, UI_Vertical_Alignment alignment, f32 spacing) {
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
		.gestures = ui_persistent_state_ptr_seq_new(&ui->arena->allocator, 0),
	};
	ui->stack[ui->stack_length++] = element;
	return element;
}

// MARK: VStack

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

	const u64 child_count = ui_element_seq_get_count(&vstack->children);

	UI_Extent children_extent = {0};
	if (child_count > 0) {
		for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
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
	const u64 child_count = ui_element_seq_get_count(&vstack->children);

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
	const vec2_f32 padded_rect_size = rect_size_f32(padded_rect);
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
		child->rect = ui_rect_snap_to_pixels(child->rect);

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
	const u64 child_count = ui_element_seq_get_count(&vstack->children);
	for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&vstack->children, child_idx);
		if (child->vtable->render) {
			child->vtable->render(ui, child, blitter);
		}
	}
}
UI_Element_Seq* ui_vstack_get_children(UI* ui, UI_Element* self) {
	UI_VStack* vstack = self->data;
	return &vstack->children;
}

const static UI_Element_VTable ui_vstack_vtable = {
	.add_child = ui_vstack_add_child,
	.get_extent = ui_vstack_get_extent,
	.layout = ui_vstack_layout,
	.render = ui_vstack_render,
	.get_children = ui_vstack_get_children,
};

UI_Element* ui_push_vstack(UI* ui, UI_Extent extent, UI_Padding padding, UI_Horizontal_Alignment alignment, f32 spacing) {
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
		.gestures = ui_persistent_state_ptr_seq_new(&ui->arena->allocator, 0),
	};
	ui->stack[ui->stack_length++] = element;
	return element;
}

// MARK: Spacer

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
UI_Element* ui_spacer(UI* ui) {
	UI_Element* element = ui_add_leaf(ui);
	*element = (UI_Element) {
		.data = NULL,
		.vtable = &ui_spacer_vtable,
		.gestures = ui_persistent_state_ptr_seq_new(&ui->arena->allocator, 0),
	};
	return element;
}

// MARK: ZStack

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
	const u64 child_count = ui_element_seq_get_count(&zstack->children);
	for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
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
	const vec2_f32 padded_rect_size = rect_size_f32(padded_rect);
	const vec2_f32 padded_rect_center = centre_rect_f32(padded_rect);

	const u64 child_count = ui_element_seq_get_count(&zstack->children);
	for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
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
		child->rect = ui_rect_snap_to_pixels(child->rect);

		if (child->vtable->layout) {
			child->vtable->layout(ui, child);
		}
	}
}
void ui_zstack_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_ZStack* zstack = self->data;
	const u64 child_count = ui_element_seq_get_count(&zstack->children);
	for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
		UI_Element* child = ui_element_seq_get_ptr(&zstack->children, child_idx);
		if (child->vtable->render) {
			child->vtable->render(ui, child, blitter);
		}
	}
}
UI_Element_Seq* ui_zstack_get_children(UI* ui, UI_Element* self) {
	UI_ZStack* zstack = self->data;
	return &zstack->children;
}

const static UI_Element_VTable ui_zstack_vtable = {
	.add_child = ui_zstack_add_child,
	.get_extent = ui_zstack_get_extent,
	.layout = ui_zstack_layout,
	.render = ui_zstack_render,
	.get_children = ui_zstack_get_children,
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
		.gestures = ui_persistent_state_ptr_seq_new(&ui->arena->allocator, 0),
	};
	return element;
}

UI_Element* ui_push_zstack(UI* ui, UI_Extent extent, UI_Padding padding) {
	UI_Element* element = ui_add_leaf(ui);
	*element = ui_zstack_new(ui, extent, padding);
	ui->stack[ui->stack_length++] = element;
	return element;
}

// MARK: Frame

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

// MARK: Debug Touches

void ui_debug_touches(UI* ui, SL_Font* font, Gpu_Texture texture) {
	const Rect_f32 rect = {
		.start = {
			40.0f,
			190.0f
		},
		.end = {
			600.0f,
			900.0f,
		},
	};
	ui_begin_frame(ui, rect);
	ui_push_zstack(ui, UI_EXTENT_FILL, UI_PADDING_NONE);
	ui_color(ui, UI_EXTENT_FILL, texture, (vec4_f32) { .w = 0.9f });
	ui_push_vstack(ui, UI_EXTENT_FILL, ui_padding_uniform(40.0f), UI_Horizontal_Alignment_Left, 4.0f);

	{
		const UI_Label_Style label_style = {
			.font = font,
			.texture = texture,
			.color = (vec4_f32) { 1.0f, 1.0f, 1.0f, 1.0f, },
		};
		ui_label(ui, UI_EXTENT_NONE, &label_style, "Touches");
	}

//	char buf[128];
//	for (u8 i = 0; i < UI_MAX_TOUCHES; i++) {
//		const UI_Touch* touch = &ui->touch_tracker.touches[i];
//
//		if ((touch->state & UI_Touch_State_Exists) > 0) {
//			const UI_Label_Style label_style = {
//				.font = font,
//				.color = (vec4_f32) { 1.0f, 0.0f, 0.0f, 1.0f, },
//			};
//			sprintf(buf, "%llu: %.2f, %.2f ", touch->internal_id, touch->position.x, touch->position.y);
//			ui_label(ui, UI_EXTENT_NONE, &label_style, buf);
//		}
//	}

	ui_spacer(ui);
	ui_pop(ui);
	ui_pop(ui);
	ui_end_frame(ui);
}

// MARK: Color

typedef struct UI_Color {
	UI_Extent extent;
	Gpu_Texture texture;
	vec4_f32 color;
} UI_Color;

UI_Extent ui_color_get_extent(UI* ui, UI_Element* self) {
	UI_Color* color = self->data;
	return color->extent;
}
void ui_color_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_Color* color = self->data;
	ui_draw_image(ui, blitter, color->texture, self->rect, (Rect_f32){0}, color->color);
}
const static UI_Element_VTable ui_color_vtable = {
	.get_extent = ui_color_get_extent,
	.render = ui_color_render,
};

UI_Element* ui_color(UI* ui, UI_Extent extent, Gpu_Texture texture, vec4_f32 color) {
	UI_Color* color_el;
	allocator_new(&ui->arena->allocator, color_el, 1);
	*color_el = (UI_Color) {
		.extent = extent,
		.color = color,
		.texture = texture,
	};
	UI_Element* element = ui_add_leaf(ui);
	*element = (UI_Element) {
		.data = color_el,
		.vtable = &ui_color_vtable,
		.gestures = ui_persistent_state_ptr_seq_new(&ui->arena->allocator, 0),
	};
	return element;
}

// MARK: Button

typedef struct UI_Button_Persistent_State {
	Allocator* allocator;
	UI_Button_State state;
} UI_Button_Persistent_State;

void ui_button_persistent_state_destroy(void* ctx) {
	UI_Button_Persistent_State* state = ctx;
	Allocator* allocator = state->allocator;
	allocator_free(allocator, state, 1);
}

typedef struct UI_Button {
	UI_ID id;
	UI_Extent extent;
	UI_Button_Style style;
	const char* label;
	u32 label_len;
	UI_Text_Measurements label_measurements;
	UI_Callback on_press;
	UI_Button_Persistent_State* persistent_state;
} UI_Button;

UI_Extent ui_button_get_extent(UI* ui, UI_Element* self) {
	UI_Button* button = self->data;
	const UI_Extent label_extent = ui_text_measurements_get_extent(&button->label_measurements);
	const UI_Extent padded_label_extent = ui_extent_add_padding(label_extent, button->style.text_padding);
	const UI_Extent result = ui_extent_combine(button->extent, padded_label_extent);
	return result;
}
void ui_button_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_Button* button = self->data;

	const Rect_f32 rect = self->rect;
	const vec2_f32 rect_size = rect_size_f32(rect);

	const vec2_f32 text_offset = {
		.x = roundf(rect.start.x + (rect_size.x * 0.5f) - (button->label_measurements.size.x * 0.5f)),
		.y = roundf(rect.start.y + (rect_size.y * 0.5f) + button->label_measurements.y_range.end),
	};

	const UI_Button_State button_state = button->persistent_state->state;
	const UI_Button_State_Style* state_style = &button->style.state[button_state];

	ui_draw_nine_patch(ui, blitter, button->style.texture, rect, state_style->backing, state_style->backing_color);
	sl_blitter_draw_text(blitter, button->style.font, button->style.texture, button->label, add_vec2_f32(text_offset, state_style->label_offset), state_style->label_color);
}
const static UI_Element_VTable ui_button_vtable = {
	.get_extent = ui_button_get_extent,
	.render = ui_button_render,
};

void ui_button_pan_callback(void* ctx, const UI_Pan_Gesture_Frame* frame) {
	UI_Element* element = ctx;
	UI_Button* button = element->data;

	switch (frame->state) {
		case UI_Gesture_State_Began:
		case UI_Gesture_State_Changed: {
			button->persistent_state->state = UI_Button_State_Active;
		} break;

		case UI_Gesture_State_Ended:
		case UI_Gesture_State_Cancelled: {
			button->persistent_state->state = UI_Button_State_Normal;
		} break;
	}

	if (frame->state == UI_Gesture_State_Ended && rect_contains_f32(element->rect, frame->position)) {
		ui_trigger_callback(button->on_press);
	}
}

UI_Element* ui_button(UI* ui, UI_ID id, UI_Extent extent, const UI_Button_Style* style, const char* label, UI_Callback on_press) {
	const u32 label_len = (u32)strlen(label);
	char* label_copy;
	allocator_new(&ui->arena->allocator, label_copy, label_len + 1);
	strcpy(label_copy, label);

	const UI_ID persistent_state_id = ui_id_internal(id, 0, 0, true);
	UI_Persistent_State* persistent_state = ui_persistent_store_acquire(&ui->persistent_store, persistent_state_id, ui->frame_index);
	if (persistent_state->data == NULL) {
		UI_Button_Persistent_State* button_persistent_state;
		allocator_new(ui->allocator, button_persistent_state, 1);
		*button_persistent_state = (UI_Button_Persistent_State) {
			.allocator = ui->allocator,
			.state = UI_Button_State_Normal,
		};
		persistent_state->data = button_persistent_state;
		persistent_state->destroy = ui_button_persistent_state_destroy;
	}

	UI_Button* button;
	allocator_new(&ui->arena->allocator, button, 1);
	*button = (UI_Button) {
		.id = id,
		.extent = extent,
		.style = *style,
		.label = label_copy,
		.label_len = label_len,
		.label_measurements = ui_text_measurements(style->font, label),
		.on_press = on_press,
		.persistent_state = persistent_state->data,
	};
	UI_Element* element = ui_add_leaf(ui);
	*element = (UI_Element) {
		.data = button,
		.vtable = &ui_button_vtable,
		.gestures = ui_persistent_state_ptr_seq_new(&ui->arena->allocator, 0),
	};

	const UI_ID pan_gesture_id = ui_id_internal(id, 1, 0, true);
	const UI_Pan_Gesture_Desc pan_desc = {
		.minimum_touches = 1,
		.maximum_touches = 1,
		.callback = {
			.ctx = element,
			.func = ui_button_pan_callback,
		},
	};
	ui_pan_gesture(ui, pan_gesture_id, &pan_desc, element);

	return element;
}

// MARK: Slider

typedef struct UI_Slider {
	UI_ID id;
	UI_Extent extent;
	UI_Slider_Style style;
	f32* value;
	Range_f32 range;
	UI_Callback on_change;
} UI_Slider;

Rect_f32 ui_slider_get_track_rect(UI_Element* self) {
	UI_Slider* slider = self->data;
	const UI_Slider_Style* style = &slider->style;
	const Rect_f32 rect = self->rect;
	const f32 center_y = (rect.end.y + rect.start.y) * 0.5f;
	const vec2_f32 needle_image_size = cvt_vec2_u32_f32(rect_size_u32(style->needle_image));
	const f32 track_lr_padding = ceil(needle_image_size.x * 0.5);
	const vec2_f32 track_rect_start = {
		rect.start.x + track_lr_padding,
		round(center_y - (style->track_height * 0.5f)),
	};
	return (Rect_f32) {
		.start = track_rect_start,
		.end = {
			rect.end.x - track_lr_padding,
			.y = track_rect_start.y + style->track_height,
		},
	};
}
UI_Extent ui_slider_get_extent(UI* ui, UI_Element* self) {
	UI_Slider* slider = self->data;

	const UI_Slider_Style* style = &slider->style;
	const vec2_f32 needle_image_size = cvt_vec2_u32_f32(rect_size_u32(style->needle_image));

	const UI_Extent slider_extent = {
		.min_width = needle_image_size.x,
		.max_width = needle_image_size.x,
		.min_height = needle_image_size.y,
		.max_height = needle_image_size.y,
	};
	const UI_Extent result = ui_extent_combine(slider->extent, slider_extent);

	return result;
}
void ui_slider_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_Slider* slider = self->data;

	const UI_Slider_Style* style = &slider->style;

	// track
	const Rect_f32 track_rect = ui_slider_get_track_rect(self);
	ui_draw_image(ui, blitter, style->texture, track_rect, (Rect_f32) { .start = {} }, style->track_color);

	const Rect_f32 rect = self->rect;
	const f32 center_y = (rect.end.y + rect.start.y) * 0.5f;
	const vec2_f32 needle_image_size = cvt_vec2_u32_f32(rect_size_u32(style->needle_image));

	// needle
	const f32 progress = saturate_f32((*slider->value - slider->range.start) / (slider->range.end - slider->range.start));
	const vec2_f32 needle_offset = {
		.x = round(lerp_f32(track_rect.start.x, track_rect.end.x, progress) - (needle_image_size.x * 0.5f)),
		.y = round(center_y - (needle_image_size.y * 0.5f)),
	};
	const Rect_f32 needle_rect = {
		.start = needle_offset,
		.end = add_vec2_f32(needle_image_size, needle_offset),
	};
	ui_draw_image(ui, blitter, style->texture, needle_rect, cvt_rect_u32_f32(style->needle_image), style->needle_color);
}
//void ui_slider_handle_events(UI* ui, UI_Element* self) {
//	UI_Slider* slider = self->data;
//
//	const bool mouse_over = !ui->mouse_obscured && rect_contains_f32(self->rect, ui->mouse_position);
//	if (mouse_over) {
//		ui->hot_item = slider->id;
//		ui->mouse_obscured = true;
//	}
//
//	if (ui_id_is_null(ui->active_item) && (ui->mouse_button_down || ui->mouse_button_pressed) && mouse_over) {
//		ui->active_item = slider->id;
//	}
//
//	const bool change_value = ui_id_equals(ui->active_item, slider->id) && ui->mouse_button_down;
//	if (change_value) {
//		ui->mouse_button_pressed = false;
//
//		const Rect_f32 track_rect = ui_slider_get_track_rect(self);
//
//		const f32 mouse_progress = saturate_f32((ui->mouse_position.x - track_rect.start.x) / (track_rect.end.x - track_rect.start.x));
//		*slider->value = lerp_f32(slider->range.start, slider->range.end, mouse_progress);
//
//		ui_trigger_callback(slider->on_change);
//	}
//
//	if (ui_id_equals(ui->active_item, slider->id) && !ui->mouse_button_down) {
//		ui->active_item = UI_ID_NULL;
//	}
//}
const static UI_Element_VTable ui_slider_vtable = {
	.get_extent = ui_slider_get_extent,
	.render = ui_slider_render,
};

void ui_slider_pan_callback(void* ctx, const UI_Pan_Gesture_Frame* frame) {
	UI_Element* element = ctx;
	UI_Slider* slider = element->data;

	if (frame->state == UI_Gesture_State_Cancelled) {
		// Restore original slider value?
		return;
	}

	const Rect_f32 track_rect = ui_slider_get_track_rect(element);

	const f32 mouse_progress = saturate_f32((frame->position.x - track_rect.start.x) / (track_rect.end.x - track_rect.start.x));
	const f32 old_value = *slider->value;
	const f32 new_value = lerp_f32(slider->range.start, slider->range.end, mouse_progress);
	*slider->value = new_value;

	if (old_value != new_value) {
		ui_trigger_callback(slider->on_change);
	}
}

UI_Element* ui_slider_f32(UI* ui, UI_ID id, UI_Extent extent, const UI_Slider_Style* style, f32* value, Range_f32 range, UI_Callback on_change) {
	UI_Slider* slider;
	allocator_new(&ui->arena->allocator, slider, 1);
	*slider = (UI_Slider) {
		.id = id,
		.extent = extent,
		.style = *style,
		.value = value,
		.range = range,
		.on_change = on_change,
	};
	UI_Element* element = ui_add_leaf(ui);
	*element = (UI_Element) {
		.data = slider,
		.vtable = &ui_slider_vtable,
		.gestures = ui_persistent_state_ptr_seq_new(&ui->arena->allocator, 0),
	};

	const UI_ID pan_gesture_id = ui_id_internal(id, 1, 0, true);
	const UI_Pan_Gesture_Desc pan_desc = {
		.minimum_touches = 1,
		.maximum_touches = 1,
		.callback = {
			.ctx = element,
			.func = ui_slider_pan_callback,
		},
	};
	ui_pan_gesture(ui, pan_gesture_id, &pan_desc, element);

	return element;
}

// MARK: Label

typedef struct UI_Label {
	UI_Extent extent;
	UI_Label_Style style;
	const char* label;
	u32 label_len;
	UI_Text_Measurements label_measurements;
} UI_Label;

UI_Extent ui_label_get_extent(UI* ui, UI_Element* self) {
	UI_Label* label = self->data;
	const UI_Extent text_extent = ui_text_measurements_get_extent(&label->label_measurements);
	const UI_Extent result = ui_extent_combine(label->extent, text_extent);
	return result;
}
void ui_label_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_Label* label = self->data;

	const Rect_f32 rect = self->rect;
	const vec2_f32 rect_size = rect_size_f32(rect);

	const vec2_f32 text_offset = {
		.x = roundf(rect.start.x + (rect_size.x * 0.5f) - (label->label_measurements.size.x * 0.5f)),
		.y = roundf(rect.start.y + (rect_size.y * 0.5f) + label->label_measurements.y_range.end),
	};

	const UI_Label_Style* style = &label->style;
	sl_blitter_draw_text(blitter, style->font, style->texture, label->label, text_offset, style->color);
}
const static UI_Element_VTable ui_label_vtable = {
	.get_extent = ui_label_get_extent,
	.render = ui_label_render,
};

UI_Element* ui_label(UI* ui, UI_Extent extent, const UI_Label_Style* style, const char* label) {
	const u32 label_len = (u32)strlen(label);
	char* label_copy;
	allocator_new(&ui->arena->allocator, label_copy, label_len + 1);
	strcpy(label_copy, label);

	UI_Label* label_el;
	allocator_new(&ui->arena->allocator, label_el, 1);
	*label_el = (UI_Label) {
		.extent = extent,
		.style = *style,
		.label = label_copy,
		.label_len = label_len,
		.label_measurements = ui_text_measurements(style->font, label),
	};
	UI_Element* element = ui_add_leaf(ui);
	*element = (UI_Element) {
		.data = label_el,
		.vtable = &ui_label_vtable,
		.gestures = ui_persistent_state_ptr_seq_new(&ui->arena->allocator, 0),
	};
	return element;
}

typedef struct UI_Custom {
	UI_Extent extent;
	UI_Render_Callback on_render;
} UI_Custom;
UI_Extent ui_custom_get_extent(UI* ui, UI_Element* self) {
	UI_Custom* custom = self->data;
	return custom->extent;
}
void ui_custom_render(UI* ui, UI_Element* self, SL_Blitter* blitter) {
	UI_Custom* custom = self->data;
	if (custom->on_render.render != NULL) {
		custom->on_render.render(custom->on_render.ctx, self->rect, blitter);
	}
}
const static UI_Element_VTable ui_custom_vtable = {
	.get_extent = ui_custom_get_extent,
	.render = ui_custom_render,
};

UI_Element* ui_custom(UI* ui, UI_Extent extent, UI_Render_Callback on_render) {
	UI_Custom* custom;
	allocator_new(&ui->arena->allocator, custom, 1);
	*custom = (UI_Custom) {
		.extent = extent,
		.on_render = on_render,
	};
	UI_Element* element = ui_add_leaf(ui);
	*element = (UI_Element) {
		.data = custom,
		.vtable = &ui_custom_vtable,
		.gestures = ui_persistent_state_ptr_seq_new(&ui->arena->allocator, 0),
	};
	return element;
}

// MARK: Lifecycle

bool ui_populate_receivers_with_element(UI* ui, UI_Touch* touch, UI_Element* element) {
	if (rect_contains_f32(element->rect, touch->position)) {
		UI_Element_Seq* children = element->vtable->get_children ? element->vtable->get_children(ui, element) : NULL;
		if (children) {
			const s64 child_count = (s64)ui_element_seq_get_count(children);
			for (s64 child_idx = child_count - 1; child_idx >= 0; child_idx--) {
				UI_Element* child = ui_element_seq_get_ptr(children, child_idx);
				const bool consumed = ui_populate_receivers_with_element(ui, touch, child);
				if (consumed) {
					return true;
				}
			}
		}

		const u64 gesture_count = ui_persistent_state_ptr_seq_get_count(&element->gestures);
		for (u64 gesture_idx = 0; gesture_idx < gesture_count; gesture_idx++) {
			UI_Persistent_State* gesture_persistent_state = ui_persistent_state_ptr_seq_get(&element->gestures, gesture_idx);

			// TODO: Filter gestures based on what they're interested in (i.e. pencil, finger, mouse).
			// UI_Gesture* gesture = ui_gesture_for_handle(ui, gesture_handle);
			// ...

			ui_persistent_state_handle_seq_push(&touch->receivers, ui_persistent_state_get_handle(gesture_persistent_state));
		}

		// TODO: Some views may not occlude touches (i.e. passthrough).
		return true;
	} else {
		return false;
	}
}
void ui_populate_receivers(UI* ui, UI_Touch* touch) {
	const s64 frame_count = (s64)ui_frame_seq_get_count(&ui->frames);
	for (s64 frame_idx = frame_count - 1; frame_idx >= 0; frame_idx--) {
		UI_Frame* frame = ui_frame_seq_get_ptr(&ui->frames, frame_idx);
		if (rect_contains_f32(frame->rect, touch->position)) {
			bool consumed = ui_populate_receivers_with_element(ui, touch, &frame->zstack);
			if (consumed) {
				break;
			}
		}
	}
}

void ui_begin(UI* ui) {
	sl_debug_assert(ui->lifecycle_state == UI_Lifecycle_State_Ended, "UI must be ended before begin.");
	ui->lifecycle_state = UI_Lifecycle_State_Began;
	++ui->frame_index;

	sl_arena_allocator_reset(ui->arena, 0);
	ui_frame_seq_clear(&ui->frames);

	ui->stack_length = 0;
}

void ui_layout_frames(UI* ui) {
	const u64 frame_count = ui_frame_seq_get_count(&ui->frames);
	for (u64 i = 0; i < frame_count; i++) {
		UI_Frame* frame = ui_frame_seq_get_ptr(&ui->frames, i);
		frame->zstack.rect = frame->rect;
		frame->zstack.vtable->layout(ui, &frame->zstack);
	}
}

void ui_handle_events(UI* ui, UI_Event_Seq* event_sink) {
	const u64 event_count = ui_event_seq_get_count(event_sink);
	for (u64 i = 0; i < event_count; i++) {
		const UI_Event event = ui_event_seq_get(event_sink, i);
		switch (event.kind) {
			case UI_Event_Kind_Touch_Began: {
				UI_Touch* touch;
				if (ui_touch_map_get(&ui->active_touch_map, event.touch.id, &touch)) {
					// Prior touch with same ID never received an ended/cancelled event.
					// Ending it now for consistency; there can't be two live touches with the same external ID.
					ui_touch_ended(ui, touch);
					touch = NULL;
				}

				touch = ui_touch_new(ui, event.touch.id);
				touch->position = event.touch.position;
				touch->timestamp = event.touch.timestamp;
				ui_populate_receivers(ui, touch);
				ui_touch_began(ui, touch);
			} break;

			case UI_Event_Kind_Touch_Changed: {
				UI_Touch* touch;
				if (ui_touch_map_get(&ui->active_touch_map, event.touch.id, &touch)) {
					touch->position = event.touch.position;
					touch->timestamp = event.touch.timestamp;
					ui_touch_changed(ui, touch);
				}
			} break;

			case UI_Event_Kind_Touch_Ended: {
				UI_Touch* touch;
				if (ui_touch_map_get(&ui->active_touch_map, event.touch.id, &touch)) {
					touch->position = event.touch.position;
					touch->timestamp = event.touch.timestamp;
					ui_touch_ended(ui, touch);
				}
			} break;

			case UI_Event_Kind_Touch_Cancelled: {
				UI_Touch* touch;
				if (ui_touch_map_get(&ui->active_touch_map, event.touch.id, &touch)) {
					touch->position = event.touch.position;
					touch->timestamp = event.touch.timestamp;
					ui_touch_cancelled(ui, touch);
				}
			} break;

			default:
				break;
		}
	}
	ui_event_seq_clear(event_sink);
}

void ui_end(UI* ui, UI_Event_Seq* event_sink) {
	sl_debug_assert(ui->lifecycle_state == UI_Lifecycle_State_Began, "UI must have began before ending.");
	ui->lifecycle_state = UI_Lifecycle_State_Ended;

	ui_layout_frames(ui);
	ui_handle_events(ui, event_sink);
	ui_persistent_store_purge(&ui->persistent_store, ui->frame_index);
}

void ui_render(UI* ui, SL_Blitter* blitter) {
	const u64 frame_count = ui_frame_seq_get_count(&ui->frames);
	for (u64 i = 0; i < frame_count; i++) {
		UI_Frame* frame = ui_frame_seq_get_ptr(&ui->frames, i);
		frame->zstack.vtable->render(ui, &frame->zstack, blitter);
	}
}
