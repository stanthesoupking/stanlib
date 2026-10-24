#include "stanlib/blitter.h"
#include "stanlib/core.h"
#include "stanlib/input.h"
#include "stanlib/string.h"
#include "stanlib/text.h"
#include <stanlib/ui.h>

// MARK: ID

#define ui_id_internal_i(parent, name, index) (_ui_id_internal((parent), sl_string_view_const(name), (index), true))
#define ui_id_internal(parent, name) ui_id_internal_i((parent), (name))

#define ui_id_internal_auto_i(parent, index) (ui_id_internal_i((parent), __BASE_FILE__ ":" sl_stringize(__COUNTER__), (index)))
#define ui_id_internal_auto(parent) ui_id_internal_auto_i((parent), 0)

UI_ID _ui_id_internal(UI_ID parent, SL_String_View name, u32 index, bool internal) {
	SL_Hasher hasher;
	sl_hasher_init(&hasher);
	sl_hasher_push(&hasher, immutable_buffer_for(internal));
	sl_hasher_push(&hasher, immutable_buffer_for(parent.hash));
	sl_hasher_push(&hasher, sl_string_view_get_immutable_buffer(name));
	sl_hasher_push(&hasher, immutable_buffer_for(index));
	return (UI_ID) {
		.hash = sl_hasher_finalise(&hasher),
	};
}
UI_ID _ui_id(UI_ID parent, SL_String_View name, u32 index) {
	return _ui_id_internal(parent, name, index, false);
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

sl_seq(UI_View, UI_View_Seq, ui_view_seq);
sl_seq(UI_Window, UI_Window_Seq, ui_window_seq);
sl_seq(UI_Gesture, UI_Gesture_Seq, ui_gesture_seq);

sl_hashmap(UI_ID, UI_Window, UI_Window_Map, ui_window_map, ui_id_get_hash, ui_id_equals);

typedef struct UI_View_Data {
	UI_ID id;
	UI_View_Flags flags;

	bool needs_layout;

	bool has_custom_implicit_size;
	vec2_s32 custom_implicit_size;

	UI_View_Callbacks callbacks;
	Rect_s32 frame;

	// In z-order; bottom to top.
	UI_View_Seq children;

	UI_Gesture_Seq gestures;
} UI_View_Data;
sl_seq(UI_View_Data, UI_View_Data_Seq, ui_view_data_seq);

typedef struct UI_Window_Data {
	UI_ID id;
	bool accessed;
	bool appearing;
	Rect_s32 frame;

	UI_Window_Style style;

	// Root view.
	// Contains label, buttons and menu bar.
	UI_View root;
	UI_View content;

	UI_Gesture drag_gesture;

	vec2_f32 pan_initial_location;
	Rect_s32 pan_initial_frame;
} UI_Window_Data;

typedef struct UI_Storage_Data {
	u32 generation;
	void* data;
	void (*destroy)(void* data);
} UI_Storage_Data;
sl_seq(UI_Storage_Data, UI_Storage_Data_Seq, ui_storage_data_seq);

typedef struct UI_Storage_Handle {
	u32 index;
	u32 generation;
} UI_Storage_Handle;
sl_seq(UI_Storage_Handle, UI_Storage_Handle_Seq, ui_storage_handle_seq);

sl_hashmap(UI_ID, UI_Storage_Handle, UI_Storage_Map, ui_storage_map, ui_id_get_hash, ui_id_equals);

typedef struct UI_Gesture_Storage_Handle {
	UI_Storage_Handle storage_handle;
} UI_Gesture_Storage_Handle;
sl_seq(UI_Gesture_Storage_Handle, UI_Gesture_Storage_Handle_Seq, ui_gesture_storage_handle_seq);

typedef struct UI_Gesture_Storage {
	void* ctx;
	const UI_Gesture_VTable* vtable;
} UI_Gesture_Storage;

// MARK: Touch

typedef enum UI_Touch_State : u8 {
	UI_Touch_State_Alive,
	UI_Touch_State_Ended,
	UI_Touch_State_Cancelled,
} UI_Touch_State;

typedef struct UI_Touch {
	Input_Touch_ID id;
	UI_Touch_State state;
	vec2_f32 position;
	f64 timestamp;
	u32 rc;

	// Gestures that receive updates relating to the touch.
	// Note: receivers could become null if the gesture no longer exists.
	UI_Gesture_Storage_Handle_Seq receivers;
} UI_Touch;

u64 input_touch_id_hash(Input_Touch_ID id) {
	SL_Hasher hasher;
	sl_hasher_init(&hasher);
	sl_hasher_push(&hasher, immutable_buffer_for(id.kind));
	sl_hasher_push(&hasher, immutable_buffer_for(id.external));
	return sl_hasher_finalise(&hasher);
}
bool input_touch_id_equals(Input_Touch_ID a, Input_Touch_ID b) {
	return (a.kind == b.kind) && (a.external == b.external);
}
sl_seq(UI_Touch, UI_Touch_Seq, ui_touch_seq);
sl_seq(UI_Touch*, UI_Touch_Ptr_Seq, ui_touch_ptr_seq);
sl_hashmap(Input_Touch_ID, UI_Touch*, UI_Touch_Map, ui_touch_map, input_touch_id_hash, input_touch_id_equals);

Input_Touch_Kind ui_touch_get_kind(UI_Touch* touch) {
	return touch->id.kind;
}

vec2_f32 ui_touch_get_position(UI_Touch* touch) {
	return touch->position;
}

bool ui_touch_equals(UI_Touch* a, UI_Touch* b) {
	return input_touch_id_equals(a->id, b->id);
}

typedef struct UI_Lifecycle {
	Gpu_Texture texture;

	// In z-order; bottom to top.
	UI_Window_Seq windows;

	Rect_s32 frame;
} UI_Lifecycle;

typedef struct UI {
	Allocator* allocator;
	u32 lifecycle_index;

	UI_Storage_Data_Seq storage_data;
	Seq_u32 storage_data_freelist;
	UI_Storage_Map storage_map;

	// Touch tracking
	UI_Touch_Map active_touch_map;
	UI_Touch_Seq touches;
	UI_Touch_Ptr_Seq touch_freelist;

	SL_Arena_Allocator* arena[2];
	SL_Arena_Allocator* scratch_arena;

	UI_Lifecycle* prev_lifecycle;
	UI_Lifecycle* current_lifecycle;
} UI;

static UI ui;

void ui_init(const UI_Desc* desc) {
	ui = (UI) {
		.allocator = desc->allocator,
		.storage_data = ui_storage_data_seq_new(desc->allocator, 64),
		.storage_data_freelist = seq_u32_new(desc->allocator, 64),
		.storage_map = ui_storage_map_new(desc->allocator, 512),
		.active_touch_map = ui_touch_map_new(desc->allocator, 32),
		.touches = ui_touch_seq_new(desc->allocator, 32),
		.touch_freelist = ui_touch_ptr_seq_new(desc->allocator, 32),
		.arena = {
			sl_arena_allocator_new(desc->allocator),
			sl_arena_allocator_new(desc->allocator),
		},
		.scratch_arena = sl_arena_allocator_new(desc->allocator),
	};
}
void ui_deinit(void) {
	// todo
}

vec2_s32 ui_padding_size(UI_Padding padding) {
	return (vec2_s32) {
		.x = padding.left + padding.right,
		.y = padding.top + padding.bottom,
	};
}

Rect_s32 ui_padded_rect(Rect_s32 rect, UI_Padding padding) {
	return (Rect_s32) {
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

// MARK: Storage

UI_Storage_Data* ui_storage_handle_resolve(UI_Storage_Handle handle) {
	UI_Storage_Data* data = ui_storage_data_seq_get_ptr(&ui.storage_data, handle.index);
	if (data->generation == handle.generation) {
		return data;
	} else {
		return NULL;
	}
}

bool ui_storage_get_handle(UI_ID id, UI_Storage_Handle* out_handle) {
	return ui_storage_map_get(&ui.storage_map, id, out_handle);
}

void* ui_storage_get(UI_ID id) {
	UI_Storage_Handle handle;
	if (ui_storage_map_get(&ui.storage_map, id, &handle)) {
		UI_Storage_Data* data = ui_storage_handle_resolve(handle);
		return data ? data->data : NULL;
	} else {
		return NULL;
	}
}

UI_Storage_Handle ui_storage_set_and_return_handle(UI_ID id, void* data, void (*destroy)(void* data)) {
	// Acquire storage data
	u32 storage_data_index;
	if (!seq_u32_pop(&ui.storage_data_freelist, &storage_data_index)) {
		storage_data_index = ui_storage_data_seq_get_count(&ui.storage_data);
		ui_storage_data_seq_push(&ui.storage_data, (UI_Storage_Data) {0});
	}

	UI_Storage_Data* storage_data = ui_storage_data_seq_get_ptr(&ui.storage_data, storage_data_index);
	storage_data->generation++;
	storage_data->data = data;
	storage_data->destroy = destroy;

	const UI_Storage_Handle storage_handle = {
		.generation = storage_data->generation,
		.index = storage_data_index,
	};

	UI_Storage_Handle* existing_handle;
	if (ui_storage_map_get_ptr(&ui.storage_map, id, &existing_handle)) {
		UI_Storage_Data* existing_data = ui_storage_data_seq_get_ptr(&ui.storage_data, existing_handle->index);
		if (existing_data->destroy) {
			existing_data->destroy(existing_data->data);
		}
		const u32 new_generation = existing_data->generation++;
		*existing_data = (UI_Storage_Data) {
			.generation = new_generation,
		};
		*existing_handle = storage_handle;
	} else {
		ui_storage_map_insert(&ui.storage_map, id, storage_handle);
	}

	return storage_handle;
}

void ui_storage_set(UI_ID id, void* data, void (*destroy)(void* data)) {
	ui_storage_set_and_return_handle(id, data, destroy);
}

UI_Gesture_Storage* ui_gesture_storage_handle_resolve(UI_Gesture_Storage_Handle handle) {
	UI_Storage_Data* storage_data = ui_storage_handle_resolve(handle.storage_handle);
	return storage_data ? storage_data->data : NULL;
}

// MARK: Touch

UI_Touch* ui_touch_new(Input_Touch_ID id) {
	UI_Touch* touch;
	if (!ui_touch_ptr_seq_pop(&ui.touch_freelist, &touch)) {
		touch = ui_touch_seq_push_reserve(&ui.touches);
		*touch = (UI_Touch) {
			.receivers = ui_gesture_storage_handle_seq_new(ui.allocator, 8),
		};
	}

	touch->state = UI_Touch_State_Alive;
	touch->position = (vec2_f32) {};
	touch->id = id;
	touch->rc = 1;

	ui_touch_map_insert(&ui.active_touch_map, id, touch);

	return touch;
}
void ui_touch_retain(UI_Touch* touch) {
	sl_debug_assert(touch->rc > 0, "RC must be > 0 to retain.");
	touch->rc++;
}
void ui_touch_release(UI_Touch* touch) {
	sl_debug_assert(touch->rc > 0, "RC must be > 0 to release.");
	touch->rc--;

	if (touch->rc == 0) {
		touch->id = (Input_Touch_ID) {0};
		ui_gesture_storage_handle_seq_clear(&touch->receivers);
		ui_touch_ptr_seq_push(&ui.touch_freelist, touch);
	}
}
void ui_touch_began(UI_Touch* touch) {
	sl_debug_assert(touch->state == UI_Touch_State_Alive, "Only alive touches can begin.");
	for (u32 receiver_idx = 0; receiver_idx < ui_gesture_storage_handle_seq_get_count(&touch->receivers); receiver_idx++) {
		const UI_Gesture_Storage_Handle gesture_handle = ui_gesture_storage_handle_seq_get(&touch->receivers, receiver_idx);
		UI_Gesture_Storage* gesture_storage = ui_gesture_storage_handle_resolve(gesture_handle);
		if (gesture_storage) {
			gesture_storage->vtable->touch_began(gesture_storage->ctx, touch);
		}
	}
}
void ui_touch_changed(UI_Touch* touch) {
	sl_debug_assert(touch->state == UI_Touch_State_Alive, "Only alive touches can change.");
	for (u32 receiver_idx = 0; receiver_idx < ui_gesture_storage_handle_seq_get_count(&touch->receivers); receiver_idx++) {
		const UI_Gesture_Storage_Handle gesture_handle = ui_gesture_storage_handle_seq_get(&touch->receivers, receiver_idx);
		UI_Gesture_Storage* gesture_storage = ui_gesture_storage_handle_resolve(gesture_handle);
		if (gesture_storage) {
			gesture_storage->vtable->touch_changed(gesture_storage->ctx, touch);
		}
	}
}
void ui_touch_ended(UI_Touch* touch) {
	sl_debug_assert(touch->state == UI_Touch_State_Alive, "Can only end an alive touch.");
	touch->state = UI_Touch_State_Ended;
	for (u32 receiver_idx = 0; receiver_idx < ui_gesture_storage_handle_seq_get_count(&touch->receivers); receiver_idx++) {
		const UI_Gesture_Storage_Handle gesture_handle = ui_gesture_storage_handle_seq_get(&touch->receivers, receiver_idx);
		UI_Gesture_Storage* gesture_storage = ui_gesture_storage_handle_resolve(gesture_handle);
		if (gesture_storage) {
			gesture_storage->vtable->touch_ended(gesture_storage->ctx, touch);
		}
	}

	sl_unused bool removed = ui_touch_map_remove(&ui.active_touch_map, touch->id);
	sl_debug_assert(removed, "Should have removed touch from alive map.");

	ui_touch_release(touch);
}
void ui_touch_cancelled(UI_Touch* touch) {
	sl_debug_assert(touch->state == UI_Touch_State_Alive, "Can only end an alive touch.");
	touch->state = UI_Touch_State_Cancelled;
	for (u32 receiver_idx = 0; receiver_idx < ui_gesture_storage_handle_seq_get_count(&touch->receivers); receiver_idx++) {
		const UI_Gesture_Storage_Handle gesture_handle = ui_gesture_storage_handle_seq_get(&touch->receivers, receiver_idx);
		UI_Gesture_Storage* gesture_storage = ui_gesture_storage_handle_resolve(gesture_handle);
		if (gesture_storage) {
			gesture_storage->vtable->touch_cancelled(gesture_storage->ctx, touch);
		}
	}

	sl_unused bool removed = ui_touch_map_remove(&ui.active_touch_map, touch->id);
	sl_debug_assert(removed, "Should have removed touch from alive map.");

	ui_touch_release(touch);
}

SL_Arena_Allocator* ui_current_arena(void) {
	return ui.arena[ui.lifecycle_index % 2];
}

Allocator* ui_lifecycle_allocator(void) {
	if (ui.current_lifecycle) {
		return &ui_current_arena()->allocator;
	} else {
		return NULL;
	}
}

void ui_lifecycle_begin(Rect_s32 frame, Gpu_Texture texture) {
	ui.lifecycle_index++;

	SL_Arena_Allocator* arena = ui_current_arena();
	sl_arena_allocator_reset(arena, 0);
	allocator_new(&arena->allocator, ui.current_lifecycle, 1);
	*ui.current_lifecycle = (UI_Lifecycle) {
		.frame = frame,
		.texture = texture,
		.windows = ui_window_seq_new(&arena->allocator, 0),
	};

	// Copy across windows from the previous lifecycle.
	if (ui.prev_lifecycle) {
		for (u64 i = 0; i < ui_window_seq_get_count(&ui.prev_lifecycle->windows); i++) {
			const UI_Window prev_handle = ui_window_seq_get(&ui.prev_lifecycle->windows, i);

			UI_Window_Data* current_data;
			allocator_new(&arena->allocator, current_data, 1);
			*current_data = *prev_handle.data;
			current_data->accessed = false;
			current_data->appearing = false;

			const UI_Window current_handle = {
				.data = current_data,
				.lifecycle_index = ui.lifecycle_index,
			};
			ui_window_seq_push(&ui.current_lifecycle->windows, current_handle);
		}
	}
}

typedef struct UI_Gesture_Data {
	UI_Gesture_Storage_Handle storage_handle;
} UI_Gesture_Data;

bool ui_populate_receivers_recurse(UI_Touch* touch, UI_View view) {
	const Rect_s32 frame = ui_view_get_frame(view);
	if (rect_contains_f32(cvt_rect_s32_f32(frame), touch->position)) {
		const bool passthrough_gestures = (view.data->flags & UI_View_Flags_Passthrough_Gestures) > 0;
		const bool consume_touch = !passthrough_gestures;

		const s64 child_count = ui_view_seq_get_count(&view.data->children);
		for (s64 child_idx = child_count - 1; child_idx >= 0; child_idx--) {
			const UI_View child = ui_view_seq_get(&view.data->children, child_idx);
			if (ui_populate_receivers_recurse(touch, child)) {
				return true;
			}
		}

		const u64 gesture_count = ui_gesture_seq_get_count(&view.data->gestures);
		for (u64 gesture_idx = 0; gesture_idx < gesture_count; gesture_idx++) {
			UI_Gesture gesture = ui_gesture_seq_get(&view.data->gestures, gesture_idx);
			ui_gesture_storage_handle_seq_push(&touch->receivers, gesture.data->storage_handle);
		}

		return consume_touch;
	}

	return false;
}

void ui_populate_receivers(UI_Touch* touch) {
	const s64 window_count = ui_window_seq_get_count(&ui.current_lifecycle->windows);
	for (s64 window_idx = window_count - 1; window_idx >= 0; window_idx--) {
		const UI_Window window = ui_window_seq_get(&ui.current_lifecycle->windows, window_idx);
		if (!window.data->accessed) {
			continue;
		}

		ui_view_layout(window.data->root);
		if (ui_populate_receivers_recurse(touch, window.data->root)) {
			break;
		}
	}
}

void ui_layout_windows(void) {
	const u64 window_count = ui_window_seq_get_count(&ui.current_lifecycle->windows);
	for (u64 window_idx = 0; window_idx < window_count; window_idx++) {
		const UI_Window window = ui_window_seq_get(&ui.current_lifecycle->windows, window_idx);
		if (!window.data->accessed) {
			continue;
		}
		ui_view_layout(window.data->root);
	}
}

void ui_lifecycle_interact(Input_Tracker* input_tracker) {
	if (!ui.current_lifecycle) {
		return;
	}

	ui_layout_windows();

	Input_Event_Iterator event_iterator = input_tracker_get_event_iterator(input_tracker);

	Input_Event event;
	while (input_event_iterator_get_next(&event_iterator, &event)) {
		switch (event.kind) {
			case Input_Event_Kind_Touch_Began: {
				UI_Touch* touch;
				if (ui_touch_map_get(&ui.active_touch_map, event.touch.id, &touch)) {
					// Prior touch with same ID never received an ended/cancelled event.
					// Ending it now for consistency; there can't be two live touches with the same external ID.
					ui_touch_ended(touch);
					touch = NULL;
				}

				touch = ui_touch_new(event.touch.id);
				touch->position = event.touch.position;
				touch->timestamp = event.touch.timestamp;
				ui_populate_receivers(touch);
				ui_touch_began(touch);
			} break;

			case Input_Event_Kind_Touch_Changed: {
				UI_Touch* touch;
				if (ui_touch_map_get(&ui.active_touch_map, event.touch.id, &touch)) {
					touch->position = event.touch.position;
					touch->timestamp = event.touch.timestamp;
					ui_touch_changed(touch);
				}
			} break;

			case Input_Event_Kind_Touch_Ended: {
				UI_Touch* touch;
				if (ui_touch_map_get(&ui.active_touch_map, event.touch.id, &touch)) {
					touch->position = event.touch.position;
					touch->timestamp = event.touch.timestamp;
					ui_touch_ended(touch);
				}
			} break;

			case Input_Event_Kind_Touch_Cancelled: {
				UI_Touch* touch;
				if (ui_touch_map_get(&ui.active_touch_map, event.touch.id, &touch)) {
					touch->position = event.touch.position;
					touch->timestamp = event.touch.timestamp;
					ui_touch_cancelled(touch);
				}
			} break;

			default:
				break;
		}
	}
}

void ui_view_pre_render(UI_View view);
void ui_pre_render_windows(void) {
	const u64 window_count = ui_window_seq_get_count(&ui.current_lifecycle->windows);
	for (u64 window_idx = 0; window_idx < window_count; window_idx++) {
		const UI_Window window = ui_window_seq_get(&ui.current_lifecycle->windows, window_idx);
		if (!window.data->accessed) {
			continue;
		}
		ui_view_pre_render(window.data->root);
	}
}

void ui_view_render(UI_View view, SL_Blitter* blitter);
void ui_render_windows(SL_Blitter* blitter) {
	const u64 window_count = ui_window_seq_get_count(&ui.current_lifecycle->windows);
	for (u64 window_idx = 0; window_idx < window_count; window_idx++) {
		const UI_Window window = ui_window_seq_get(&ui.current_lifecycle->windows, window_idx);
		if (!window.data->accessed) {
			continue;
		}

		const UI_Window_Render_Fn render_fn = window.data->style.render;
		if (render_fn) {
			const UI_Window_Render_Desc render_desc = {
				.ctx = window.data->style.ctx,
				.blitter = blitter,
				.frame = ui_window_get_frame(window),
				.window_state = UI_Window_State_Active,
			};
			render_fn(&render_desc);
		}

		ui_view_render(window.data->root, blitter);
	}
}

void ui_lifecycle_pre_render(void) {
	if (!ui.current_lifecycle) {
		return;
	}

	ui_layout_windows();
	ui_pre_render_windows();
}

void ui_lifecycle_render(SL_Blitter* blitter) {
	if (!ui.current_lifecycle) {
		return;
	}

	ui_layout_windows();
	ui_render_windows(blitter);
}

void ui_cull_unused_windows(void) {
	u64 i = 0;
	while (i < ui_window_seq_get_count(&ui.current_lifecycle->windows)) {
		const UI_Window window = ui_window_seq_get(&ui.current_lifecycle->windows, i);
		if (!window.data->accessed) {
			ui_window_seq_remove(&ui.current_lifecycle->windows, i);
		} else {
			i++;
		}
	}
}

void ui_lifecycle_end(void) {
	ui_cull_unused_windows();

	ui.prev_lifecycle = ui.current_lifecycle;
	ui.current_lifecycle = NULL;
}

// MARK: Drawing

void ui_render_util_draw_image(SL_Blitter* blitter, Gpu_Texture texture, Rect_f32 rect, Rect_f32 atlas_rect, vec4_f32 color) {
	const Gpu_Texture_Desc* texture_desc = gpu_texture_get_desc(texture);
	const vec2_f32 texture_size = { (f32)texture_desc->size.x, (f32)texture_desc->size.y };

	const Textured_Quad_f32 quad = textured_quad_for_sub_region_f32(rect, div_rect_vec_f32(atlas_rect, texture_size), color);
	sl_blitter_draw_textured_quads(blitter, texture, &quad, 1, SL_Blitter_Sampler_Nearest);
}

void ui_render_util_draw_color(SL_Blitter* blitter, Rect_f32 rect, vec4_f32 color) {
	ui_render_util_draw_image(blitter, ui.current_lifecycle->texture, rect, (Rect_f32){0}, color);
}

void ui_render_util_draw_nine_patch(SL_Blitter* blitter, Gpu_Texture texture, Rect_f32 rect, UI_Nine_Patch nine_patch, vec4_f32 tint) {
	Textured_Quad_f32 quads[9];

	const Gpu_Texture_Desc* texture_desc = gpu_texture_get_desc(texture);
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

	sl_blitter_draw_textured_quads(blitter, texture, quads, next_quad, SL_Blitter_Sampler_Nearest);
}

// MARK: Window

void ui_window_validate(UI_Window window) {
	sl_debug_assert(window.lifecycle_index == ui.lifecycle_index, "Invalid window.");
}

void ui_window_root_layout_subviews(void* ctx, UI_View view) {
	UI_Window_Data* window_data = ctx;

	Rect_s32 root_frame = ui_view_get_frame(window_data->root);
	const Rect_s32 content_frame = root_frame;
	ui_view_set_frame(window_data->content, content_frame);
}

void ui_window_pan(void* ctx, const UI_Pan_Gesture_Frame* frame) {
	UI_Window* window = ctx;
	ui_window_validate(*window);

	ui_window_send_to_front(*window);

	if (frame->state == UI_Gesture_State_Began) {
		window->data->pan_initial_location = frame->position;
		window->data->pan_initial_frame = window->data->frame;
	}

	const Rect_s32 new_frame = rect_offset_s32(window->data->pan_initial_frame, cvt_vec2_f32_s32(frame->translation));
	ui_window_set_frame(*window, new_frame);
}

UI_Window ui_window(UI_ID id, UI_View content, UI_Window_Style style) {
	bool appearing = false;
	UI_Window_Data* current_data = NULL;

	// Find existing window.
	{
		const u64 window_count = ui_window_seq_get_count(&ui.current_lifecycle->windows);
		for (u64 window_idx = 0; window_idx < window_count; window_idx++) {
			const UI_Window window = ui_window_seq_get(&ui.current_lifecycle->windows, window_idx);
			if (ui_id_equals(window.data->id, id)) {
				sl_debug_assert(!window.data->accessed, "Window with the same ID already exists.");
				current_data = window.data;
			}
		}
	}

	// Create new window.
	if (!current_data) {
		appearing = true;
		allocator_new(ui_lifecycle_allocator(), current_data, 1);
		*current_data = (UI_Window_Data) {0};
	}

	const UI_View_Callbacks root_callbacks = {
		.ctx = current_data,
		.layout_subviews = ui_window_root_layout_subviews,
	};
	UI_View root = ui_view(ui_id_internal_auto(id), &root_callbacks);
	ui_view_set_frame(root, current_data->frame);

	const UI_Window window = {
		.data = current_data,
		.lifecycle_index = ui.lifecycle_index,
	};

	UI_Window* pan_ctx;
	allocator_new(ui_lifecycle_allocator(), pan_ctx, 1);
	*pan_ctx = window;
	const UI_Pan_Gesture_Desc pan_desc = {
		.touch_mask = Input_Touch_Kind_Mask_All,
		.minimum_touches = 1,
		.maximum_touches = 1,
		.callback = {
			.ctx = pan_ctx,
			.func = ui_window_pan,
		},
	};
	UI_Gesture drag_gesture = ui_pan_gesture(ui_id_internal_auto(id), &pan_desc);

	ui_view_add_subview(root, content);

	current_data->id = id;
	current_data->accessed = true;
	current_data->root = root;
	current_data->content = content;
	current_data->style = style;
	current_data->appearing = appearing;
	current_data->drag_gesture = drag_gesture;

	if (appearing) {
		ui_window_seq_push(&ui.current_lifecycle->windows, window);

		const vec2_s32 root_frame_centre = rect_centre_s32(ui.current_lifecycle->frame);

		// Apply default frame
		const vec2_s32 window_size = ui_window_get_intrinsic_size(window);
		const vec2_s32 window_offset = sub_vec2_s32(root_frame_centre, div_vec2_s32(window_size, splat_vec2_s32(2)));
		const Rect_s32 window_frame = {
			.start = window_offset,
			.end = add_vec2_s32(window_offset, window_size),
		};
		ui_window_set_frame(window, window_frame);
	}

	return window;
}

UI_Gesture ui_window_get_drag_gesture(UI_Window window) {
	ui_window_validate(window);
	return window.data->drag_gesture;
}

vec2_s32 ui_window_get_intrinsic_size(UI_Window window) {
	ui_window_validate(window);
	return ui_view_get_intrinsic_size(window.data->content);
}

bool ui_window_is_appearing(UI_Window window) {
	ui_window_validate(window);

	return window.data->appearing;
}

Rect_s32 ui_window_get_frame(UI_Window window) {
	ui_window_validate(window);

	return window.data->frame;
}

void ui_window_set_frame(UI_Window window, Rect_s32 frame) {
	ui_window_validate(window);

	window.data->frame = frame;
	ui_view_set_frame(window.data->root, frame);
}

void ui_window_send_to_front(UI_Window window) {
	ui_window_validate(window);

	// Find index in window array.
	u64 current_idx;
	const u64 window_count = ui_window_seq_get_count(&ui.current_lifecycle->windows);
	for (current_idx = 0; current_idx < window_count; current_idx++) {
		const UI_Window window_at_idx = ui_window_seq_get(&ui.current_lifecycle->windows, current_idx);
		if (window_at_idx.data == window.data) {
			break;
		}
	}

	sl_debug_assert(current_idx < window_count, "Failed to find window in list.");
	if (current_idx >= window_count) {
		return;
	}

	ui_window_seq_remove(&ui.current_lifecycle->windows, current_idx);
	ui_window_seq_push(&ui.current_lifecycle->windows, window);
}

UI_View ui_view(UI_ID id, const UI_View_Callbacks* callbacks) {
	Allocator* allocator = ui_lifecycle_allocator();

	UI_View_Data* current_data;
	allocator_new(allocator, current_data, 1);
	*current_data = (UI_View_Data) {
		.id = id,
		.callbacks = callbacks ? *callbacks : (UI_View_Callbacks) {},
		.needs_layout = true,
		.children = ui_view_seq_new(allocator, 0),
		.gestures = ui_gesture_seq_new(allocator, 0),
	};

	return (UI_View) {
		.data = current_data,
		.lifecycle_index = ui.lifecycle_index,
	};
}

void ui_view_validate(UI_View view) {
	sl_debug_assert(view.lifecycle_index == ui.lifecycle_index, "Invalid view.");
}

UI_View_Flags ui_view_get_flags(UI_View view) {
	ui_view_validate(view);
	return view.data->flags;
}

void ui_view_set_flags(UI_View view, UI_View_Flags flags) {
	ui_view_validate(view);
	view.data->flags = flags;
}

void ui_view_add_flags(UI_View view, UI_View_Flags flags) {
	ui_view_validate(view);
	view.data->flags |= flags;
}

Rect_s32 ui_view_get_frame(UI_View view) {
	ui_view_validate(view);

	return view.data->frame;
}

void ui_view_set_frame(UI_View view, Rect_s32 frame) {
	ui_view_validate(view);

	view.data->needs_layout |= !rect_s32_equals(view.data->frame, frame);
	view.data->frame = frame;
}

void ui_view_layout(UI_View view) {
	ui_view_validate(view);

	if (!view.data->needs_layout) {
		return;
	}

	if (view.data->callbacks.layout_subviews) {
		view.data->callbacks.layout_subviews(view.data->callbacks.ctx, view);
	}

	const u64 child_count = ui_view_seq_get_count(&view.data->children);
	for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
		const UI_View child = ui_view_seq_get(&view.data->children, child_idx);
		ui_view_layout(child);
	}
}

void ui_view_pre_render(UI_View view) {
	ui_view_validate(view);

	if (view.data->callbacks.pre_render) {
		view.data->callbacks.pre_render(view.data->callbacks.ctx, view);
	}

	const u64 child_count = ui_view_seq_get_count(&view.data->children);
	for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
		const UI_View child = ui_view_seq_get(&view.data->children, child_idx);
		ui_view_pre_render(child);
	}
}

void ui_view_render(UI_View view, SL_Blitter* blitter) {
	ui_view_validate(view);

	if (view.data->callbacks.render) {
		view.data->callbacks.render(view.data->callbacks.ctx, view, blitter);
	}

	const u64 child_count = ui_view_seq_get_count(&view.data->children);
	for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
		const UI_View child = ui_view_seq_get(&view.data->children, child_idx);
		ui_view_render(child, blitter);
	}
}

void ui_view_set_intrinsic_size(UI_View view, vec2_s32 size) {
	ui_view_validate(view);

	view.data->has_custom_implicit_size = true;
	view.data->custom_implicit_size = size;
}

vec2_s32 ui_view_get_intrinsic_size(UI_View view) {
	ui_view_validate(view);

	if (view.data->has_custom_implicit_size) {
		// Size has been overridden via `ui_view_set_intrinsic_size()`.
		return view.data->custom_implicit_size;
	}

	const UI_View_Callbacks* callbacks = &view.data->callbacks;
	return callbacks->get_intrinsic_size ? callbacks->get_intrinsic_size(callbacks->ctx, view) : (vec2_s32) { 0, 0 };
}

void ui_view_add_subview(UI_View parent, UI_View child) {
	ui_view_validate(parent);
	ui_view_validate(child);

	parent.data->needs_layout = true;
	ui_view_seq_push(&parent.data->children, child);
}

void ui_view_add_gesture(UI_View view, UI_Gesture gesture) {
	ui_view_validate(view);

	ui_gesture_seq_push(&view.data->gestures, gesture);
}

// MARK: Text Measurements

typedef struct UI_Text_Measurements {
	Range_s32 y_range;
	vec2_s32 size;
} UI_Text_Measurements;

UI_Text_Measurements ui_text_measurements(SL_Font* font, SL_String_View string) {
	const Range_s32 font_y_range = sl_font_get_y_range(font);

	if (string.length == 0) {
		return (UI_Text_Measurements) {
			.size = {},
			.y_range = font_y_range,
		};
	}

	const Rect_s32 label_rect = sl_font_measure_string(font, string.str);
	const vec2_s32 label_size = rect_size_s32(label_rect);
	return (UI_Text_Measurements) {
		.size = label_size,
		.y_range = font_y_range,
	};
}

// MARK: Distribute

// MARK: Distribute

typedef struct UI_Distribute_Item {
	f32 value;
	f32 min;
	f32 max;
	bool fill;
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

		if (item->fill) {
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
			if (!item->fill) {
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
			if (item->fill) {
				item->value += amount_per_child;
			}
		}
	}
}

// MARK: Stack

typedef struct UI_Stack_Data {
	UI_Padding padding;
	UI_Axis axis;
	UI_Alignment alignment;
	s32 spacing;
} UI_Stack_Data;

vec2_s32 ui_stack_get_intrinsic_size(void* ctx, UI_View view) {
	ui_view_validate(view);
	UI_Stack_Data* stack_data = ctx;

	vec2_s32 size = {0};

	const u64 child_count = ui_view_seq_get_count(&view.data->children);
	for (u64 i = 0; i < child_count; i++) {
		const UI_View child = ui_view_seq_get(&view.data->children, i);
		const vec2_s32 child_size = ui_view_get_intrinsic_size(child);

		switch (stack_data->axis) {
			case UI_Axis_Vertical: {
				size.x = sl_max(size.x, child_size.x);
				size.y += child_size.y;
			} break;

			case UI_Axis_Horizontal: {
				size.x += child_size.x;
				size.y = sl_max(size.y, child_size.y);
			} break;
		}
	}

	const s32 spacing_total = (child_count > 0) ? ((s32)child_count - 1) : 0;
	switch (stack_data->axis) {
		case UI_Axis_Vertical: {
			size.y += spacing_total * stack_data->spacing;
		} break;

		case UI_Axis_Horizontal: {
			size.x += spacing_total * stack_data->spacing;
		} break;
	}

	size.x += stack_data->padding.left + stack_data->padding.right;
	size.y += stack_data->padding.top + stack_data->padding.bottom;

	return size;
}

void ui_stack_layout_subviews(void* ctx, UI_View view) {
	ui_view_validate(view);
	UI_Stack_Data* stack_data = ctx;

	const u64 child_count = ui_view_seq_get_count(&view.data->children);

	const u64 initial_arena_position = sl_arena_allocator_get_position(ui.scratch_arena);

	UI_Distribute_Item* distribute_items;
	allocator_new(&ui.scratch_arena->allocator, distribute_items, child_count);

	for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
		const UI_View child = ui_view_seq_get(&view.data->children, child_idx);
		const vec2_s32 child_intrinsic_size = ui_view_get_intrinsic_size(child);

		switch (stack_data->axis) {
			case UI_Axis_Vertical: {
				distribute_items[child_idx] = (UI_Distribute_Item) {
					.value = 0.0f,
					.min = child_intrinsic_size.y,
					.max = child_intrinsic_size.y,
					.fill = ((child.data->flags & UI_View_Flags_Fill_Available_Space) > 0),
				};
			} break;

			case UI_Axis_Horizontal: {
				distribute_items[child_idx] = (UI_Distribute_Item) {
					.value = 0.0f,
					.min = child_intrinsic_size.x,
					.max = child_intrinsic_size.x,
					.fill = ((child.data->flags & UI_View_Flags_Fill_Available_Space) > 0),
				};
			} break;
		}
	}

	const Rect_s32 stack_frame = ui_view_get_frame(view);
	const Rect_s32 stack_frame_padded = ui_padded_rect(stack_frame, stack_data->padding);
	const vec2_s32 padded_rect_size = rect_size_s32(stack_frame_padded);

	switch (stack_data->axis) {
		case UI_Axis_Vertical: {
			ui_distribute(padded_rect_size.y, stack_data->spacing, distribute_items, child_count);
			f32 current_y = stack_frame_padded.start.y;
			for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
				const UI_View child = ui_view_seq_get(&view.data->children, child_idx);
				const vec2_s32 child_intrinsic_size = ui_view_get_intrinsic_size(child);
				const UI_Distribute_Item* distribute_item = &distribute_items[child_idx];
				const bool is_fill = (stack_data->alignment == UI_Alignment_Fill);
				const s32 child_width = is_fill ? padded_rect_size.x : sl_min(padded_rect_size.x, child_intrinsic_size.x);

				f32 offset_x;
				switch (stack_data->alignment) {
					case UI_Alignment_Fill:
					case UI_Alignment_Left: {
						offset_x = stack_frame_padded.start.x;
					} break;

					case UI_Alignment_Center: {
						offset_x = stack_frame_padded.start.x + (padded_rect_size.x * 0.5f) - (child_width * 0.5f);
					} break;

					case UI_Alignment_Right: {
						offset_x = stack_frame_padded.start.x + padded_rect_size.x - child_width;
					} break;
				}

				const f32 child_height = distribute_item->value;

				const Rect_s32 child_frame = {
					.start = {
						offset_x,
						current_y,
					},
					.end = {
						offset_x + child_width,
						current_y + child_height,
					},
				};
				ui_view_set_frame(child, child_frame);
				current_y += child_height + stack_data->spacing;
			}
		} break;

		case UI_Axis_Horizontal: {
			ui_distribute(padded_rect_size.x, stack_data->spacing, distribute_items, child_count);
			f32 current_x = stack_frame_padded.start.x;
			for (u64 child_idx = 0; child_idx < child_count; child_idx++) {
				const UI_View child = ui_view_seq_get(&view.data->children, child_idx);
				const vec2_s32 child_intrinsic_size = ui_view_get_intrinsic_size(child);
				const UI_Distribute_Item* distribute_item = &distribute_items[child_idx];
				const bool is_fill = (stack_data->alignment == UI_Alignment_Fill);
				const s32 child_height = is_fill ? padded_rect_size.y : sl_min(padded_rect_size.y, child_intrinsic_size.y);

				f32 offset_y;
				switch (stack_data->alignment) {
					case UI_Alignment_Fill:
					case UI_Alignment_Top: {
						offset_y = stack_frame_padded.start.y;
					} break;

					case UI_Alignment_Center: {
						offset_y = stack_frame_padded.start.y + (padded_rect_size.y * 0.5f) - (child_height * 0.5f);
					} break;

					case UI_Alignment_Bottom: {
						offset_y = stack_frame_padded.start.y + padded_rect_size.y - child_height;
					} break;
				}

				const f32 child_width = distribute_item->value;

				const Rect_s32 child_frame = {
					.start = {
						current_x,
						offset_y,
					},
					.end = {
						current_x + child_width,
						offset_y + child_height,
					},
				};
				ui_view_set_frame(child, child_frame);
				current_x += child_width + stack_data->spacing;
			}
		} break;
	}

	distribute_items = NULL;
	sl_arena_allocator_reset(ui.scratch_arena, initial_arena_position);
}

UI_View ui_stack(UI_ID id, UI_Axis axis, UI_Alignment alignment, s32 spacing, UI_Padding padding) {
	UI_Stack_Data* stack_data;
	allocator_new(ui_lifecycle_allocator(), stack_data, 1);
	*stack_data = (UI_Stack_Data) {
		.axis = axis,
		.alignment = alignment,
		.spacing = spacing,
		.padding = padding,
	};

	const UI_View_Callbacks callbacks = {
		.ctx = stack_data,
		.get_intrinsic_size = ui_stack_get_intrinsic_size,
		.layout_subviews = ui_stack_layout_subviews,
	};
	return ui_view(id, &callbacks);
}

// MARK: Z Stack

typedef struct UI_ZStack_Data {
	UI_Padding padding;
} UI_ZStack_Data;

vec2_s32 ui_zstack_get_intrinsic_size(void* ctx, UI_View view) {
	UI_ZStack_Data* zstack_data = ctx;

	vec2_s32 size = {0};

	const vec2_s32 padding_size = ui_padding_size(zstack_data->padding);
	const u64 child_count = ui_view_seq_get_count(&view.data->children);
	for (u64 i = 0; i < child_count; i++) {
		const UI_View child = ui_view_seq_get(&view.data->children, i);
		const vec2_s32 child_size = add_vec2_s32(ui_view_get_intrinsic_size(child), padding_size);
		size = max_vec2_s32(size, child_size);
	}

	return size;
}

void ui_zstack_layout_subviews(void* ctx, UI_View view) {
	UI_ZStack_Data* zstack_data = ctx;

	const Rect_s32 view_frame = ui_view_get_frame(view);
	const Rect_s32 padded_frame = ui_padded_rect(view_frame, zstack_data->padding);

	const u64 child_count = ui_view_seq_get_count(&view.data->children);
	for (u64 i = 0; i < child_count; i++) {
		const UI_View child = ui_view_seq_get(&view.data->children, i);
		ui_view_set_frame(child, padded_frame);
	}
}

UI_View ui_zstack(UI_ID id, UI_Padding padding) {
	UI_ZStack_Data* zstack_data;
	allocator_new(ui_lifecycle_allocator(), zstack_data, 1);
	*zstack_data = (UI_ZStack_Data) {
		.padding = padding,
	};

	const UI_View_Callbacks callbacks = {
		.ctx = zstack_data,
		.get_intrinsic_size = ui_zstack_get_intrinsic_size,
		.layout_subviews = ui_zstack_layout_subviews,
	};
	return ui_view(id, &callbacks);
}

// MARK: Label

typedef struct UI_Label_Data {
	SL_String* text;
	UI_Label_Style style;
	UI_Text_Measurements measurements;
	UI_Padding padding;
} UI_Label_Data;

vec2_s32 ui_label_get_intrinsic_size(void* ctx, UI_View view) {
	UI_Label_Data* data = ctx;
	return add_vec2_s32(data->measurements.size, ui_padding_size(data->padding));
}

void ui_label_render(void* ctx, UI_View view, SL_Blitter* blitter) {
	UI_Label_Data* data = ctx;

	Rect_s32 padded_frame = ui_padded_rect(ui_view_get_frame(view), data->padding);
	const Rect_f32 frame_f32 = cvt_rect_s32_f32(padded_frame);
	const vec2_f32 frame_size = rect_size_f32(frame_f32);

	const vec2_f32 text_offset = {
		.x = roundf(frame_f32.start.x + (frame_size.x * 0.5f) - (data->measurements.size.x * 0.5f)),
		.y = roundf(frame_f32.start.y + (frame_size.y * 0.5f) + data->measurements.y_range.end),
	};

	const UI_Label_Style* style = &data->style;
	sl_blitter_draw_text(blitter, style->font, ui.current_lifecycle->texture, sl_string_view(data->text).str, text_offset, style->color);
}

UI_View ui_label(UI_ID id, SL_String_View text, const UI_Label_Style* style, UI_Padding padding) {
	UI_Label_Data* label_data;
	allocator_new(ui_lifecycle_allocator(), label_data, 1);
	*label_data = (UI_Label_Data) {
		.text = sl_string_new(ui_lifecycle_allocator(), text),
		.measurements = ui_text_measurements(style->font, text),
		.style = *style,
		.padding = padding,
	};

	const UI_View_Callbacks callbacks = {
		.ctx = label_data,
		.get_intrinsic_size = ui_label_get_intrinsic_size,
		.render = ui_label_render,
	};
	return ui_view(id, &callbacks);
}

void ui_trigger_callback(UI_Callback callback) {
	if (callback.func) {
		callback.func(callback.ctx);
	}
}

// MARK: Button

typedef struct UI_Button_Storage {
	UI_Button_State state;
} UI_Button_Storage;

void ui_button_storage_destroy(void* ctx) {
	UI_Button_Storage* state = ctx;
	allocator_free(ui.allocator, state, 1);
}

typedef struct UI_Button_Data {
	UI_Storage_Handle storage_handle;
	UI_Button_Style style;
	UI_Callback_With_View on_press;
	UI_View view;
} UI_Button_Data;

void ui_button_render(void* ctx, UI_View view, SL_Blitter* blitter) {
	const UI_Button_Data* data = ctx;
	const UI_Button_Render_Fn render_fn = data->style.render;

	if (!render_fn) {
		return;
	}

	const UI_Button_Storage* storage = ui_storage_handle_resolve(data->storage_handle)->data;
	const Rect_s32 button_frame = ui_view_get_frame(view);

	const UI_Button_Render_Desc render_desc = {
		.ctx = data->style.ctx,
		.blitter = blitter,
		.frame = button_frame,
		.button_state = storage->state,
	};
	render_fn(&render_desc);
}

typedef struct UI_Button_Pan_Data {
	UI_View view;
	UI_Button_Data* button;
} UI_Button_Pan_Data;

void ui_button_pan(void* ctx, const UI_Pan_Gesture_Frame* frame) {
	UI_Button_Pan_Data* pan_data = ctx;
	UI_Button_Data* button_data = pan_data->button;
	UI_Button_Storage* button_storage = ui_storage_handle_resolve(button_data->storage_handle)->data;

	switch (frame->state) {
		case UI_Gesture_State_Began:
		case UI_Gesture_State_Changed: {
			button_storage->state = UI_Button_State_Active;
		} break;

		case UI_Gesture_State_Ended:
		case UI_Gesture_State_Cancelled: {
			button_storage->state = UI_Button_State_Normal;
		} break;
	}

	const Rect_s32 view_frame = ui_view_get_frame(pan_data->view);
	if (frame->state == UI_Gesture_State_Ended && rect_contains_f32(cvt_rect_s32_f32(view_frame), frame->position)) {
		if (button_data->on_press.func) {
			button_data->on_press.func(button_data->on_press.ctx, button_data->view);
		}
	}
}

vec2_s32 ui_button_get_intrinsic_size(void* ctx, UI_View view) {
	UI_Button_Data* data = ctx;
	return data->style.intrinsic_size;
}

UI_View ui_button(UI_ID id, UI_Button_Style style, UI_Callback_With_View on_press) {
	const UI_ID storage_id = ui_id_internal_auto(id);
	UI_Storage_Handle storage_handle;
	if (!ui_storage_get_handle(storage_id, &storage_handle)) {
		UI_Button_Storage* storage;
		allocator_new(ui.allocator, storage, 1);
		*storage = (UI_Button_Storage) {
			.state = UI_Button_State_Normal,
		};
		storage_handle = ui_storage_set_and_return_handle(storage_id, storage, ui_button_storage_destroy);
	}

	UI_Button_Data* button_data;
	allocator_new(ui_lifecycle_allocator(), button_data, 1);

	const UI_View_Callbacks callbacks = {
		.ctx = button_data,
		.get_intrinsic_size = ui_button_get_intrinsic_size,
		.render = ui_button_render,
	};
	UI_View view = ui_view(id, &callbacks);

	*button_data = (UI_Button_Data) {
		.storage_handle = storage_handle,
		.style = style,
		.on_press = on_press,
		.view = view,
	};

	UI_Button_Pan_Data* pan_data;
	allocator_new(ui_lifecycle_allocator(), pan_data, 1);
	*pan_data = (UI_Button_Pan_Data) {
		.view = view,
		.button = button_data,
	};

	const UI_ID pan_gesture_id = ui_id_internal_auto(id);
	const UI_Pan_Gesture_Desc pan_desc = {
		.touch_mask = Input_Touch_Kind_Mask_All,
		.minimum_touches = 1,
		.maximum_touches = 1,
		.callback = {
			.ctx = pan_data,
			.func = ui_button_pan,
		},
	};
	UI_Gesture pan_gesture = ui_pan_gesture(pan_gesture_id, &pan_desc);
	ui_view_add_gesture(view, pan_gesture);

	return view;
}

// MARK: Slider

void ui_slider_callbacks_basic_f32_set_value(void* ctx, f32 value) {
	f32* ptr = ctx;
	*ptr = value;
}
f32 ui_slider_callbacks_basic_f32_get_value(void* ctx) {
	f32* ptr = ctx;
	return *ptr;
}

UI_Slider_Callbacks ui_slider_callbacks_basic_f32(f32* ptr) {
	return (UI_Slider_Callbacks) {
		.ctx = ptr,
		.set_value = ui_slider_callbacks_basic_f32_set_value,
		.get_value = ui_slider_callbacks_basic_f32_get_value,
	};
}

typedef struct UI_Slider_Data {
	UI_Slider_Style style;
	UI_Slider_Callbacks callbacks;
	Range_f32 range;
} UI_Slider_Data;

typedef struct UI_Slider_Pan_Data {
	UI_View view;
	UI_Slider_Data* slider_data;
} UI_Slider_Pan_Data;

Rect_s32 ui_slider_get_track_rect(UI_View view, UI_Slider_Data* slider_data) {
	const UI_Slider_Style* style = &slider_data->style;
	const Rect_s32 rect = ui_view_get_frame(view);
	const f32 center_y = (rect.end.y + rect.start.y) * 0.5f;
	const vec2_f32 needle_image_size = cvt_vec2_u32_f32(rect_size_u32(style->needle_rect));
	const f32 track_lr_padding = ceil(needle_image_size.x * 0.5);
	const vec2_s32 track_rect_start = {
		rect.start.x + track_lr_padding,
		round(center_y - (style->track_height * 0.5f)),
	};
	return (Rect_s32) {
		.start = track_rect_start,
		.end = {
			rect.end.x - track_lr_padding,
			.y = track_rect_start.y + style->track_height,
		},
	};
}

void ui_slider_pan(void* ctx, const UI_Pan_Gesture_Frame* frame) {
	UI_Slider_Pan_Data* pan_data = ctx;
	UI_Slider_Data* slider = pan_data->slider_data;

	switch (frame->state) {
		case UI_Gesture_State_Began: {
			if (slider->callbacks.on_begin) {
				slider->callbacks.on_begin(slider->callbacks.ctx);
			}
		} break;

		case UI_Gesture_State_Changed: {

		} break;

		case UI_Gesture_State_Ended:
		case UI_Gesture_State_Cancelled: {
			if (slider->callbacks.on_end) {
				slider->callbacks.on_end(slider->callbacks.ctx);
			}
		} break;
	}

	if (frame->state == UI_Gesture_State_Cancelled) {
		// Restore original slider value?
		return;
	}

	const Rect_s32 track_rect = ui_slider_get_track_rect(pan_data->view, slider);

	const f32 mouse_progress = saturate_f32((frame->position.x - track_rect.start.x) / (track_rect.end.x - track_rect.start.x));
	const f32 old_value = slider->callbacks.get_value(slider->callbacks.ctx);
	const f32 new_value = lerp_f32(slider->range.start, slider->range.end, mouse_progress);
	slider->callbacks.set_value(slider->callbacks.ctx, new_value);
}

void ui_slider_render(void* ctx, UI_View view, SL_Blitter* blitter) {
	UI_Slider_Data* slider = ctx;

	const UI_Slider_Style* style = &slider->style;

	// track
	const Rect_s32 track_rect = ui_slider_get_track_rect(view, slider);
	ui_render_util_draw_image(blitter, ui.current_lifecycle->texture, cvt_rect_s32_f32(track_rect), (Rect_f32) { .start = {} }, style->track_color);

	const Rect_s32 rect = ui_view_get_frame(view);
	const f32 center_y = (rect.end.y + rect.start.y) * 0.5f;
	const vec2_f32 needle_image_size = mul_vec2_f32(cvt_vec2_u32_f32(rect_size_u32(style->needle_rect)), splat_vec2_f32(style->needle_scale));

	// needle
	const f32 value = slider->callbacks.get_value(slider->callbacks.ctx);
	const f32 progress = saturate_f32((value - slider->range.start) / (slider->range.end - slider->range.start));
	const vec2_f32 needle_offset = {
		.x = round(lerp_f32(track_rect.start.x, track_rect.end.x, progress) - (needle_image_size.x * 0.5f)),
		.y = round(center_y - (needle_image_size.y * 0.5f)),
	};
	const Rect_f32 needle_rect = {
		.start = needle_offset,
		.end = add_vec2_f32(needle_image_size, needle_offset),
	};
	ui_render_util_draw_image(blitter, ui.current_lifecycle->texture, needle_rect, cvt_rect_u32_f32(style->needle_rect), style->needle_tint);
}

vec2_s32 ui_slider_get_intrinsic_size(void* ctx, UI_View view) {
	UI_Slider_Data* slider_data = ctx;

	const vec2_f32 needle_image_size = mul_vec2_f32(cvt_vec2_u32_f32(rect_size_u32(slider_data->style.needle_rect)), splat_vec2_f32(slider_data->style.needle_scale));

	return (vec2_s32) {
		needle_image_size.x,
		needle_image_size.y,
	};
}

UI_View ui_slider(UI_ID id, const UI_Slider_Style* style, Range_f32 range, UI_Slider_Callbacks callbacks) {
	UI_Slider_Data* slider_data;
	allocator_new(ui_lifecycle_allocator(), slider_data, 1);
	*slider_data = (UI_Slider_Data) {
		.style = *style,
		.callbacks = callbacks,
		.range = range,
	};

	const UI_View_Callbacks view_callbacks = {
		.ctx = slider_data,
		.get_intrinsic_size = ui_slider_get_intrinsic_size,
		.render = ui_slider_render,
	};
	UI_View view = ui_view(id, &view_callbacks);

	UI_Slider_Pan_Data* pan_data;
	allocator_new(ui_lifecycle_allocator(), pan_data, 1);
	*pan_data = (UI_Slider_Pan_Data) {
		.view = view,
		.slider_data = slider_data,
	};

	const UI_ID pan_gesture_id = ui_id_internal_auto(id);
	const UI_Pan_Gesture_Desc pan_desc = {
		.touch_mask = Input_Touch_Kind_Mask_All,
		.minimum_touches = 1,
		.maximum_touches = 1,
		.callback = {
			.ctx = pan_data,
			.func = ui_slider_pan,
		},
	};
	UI_Gesture pan_gesture = ui_pan_gesture(pan_gesture_id, &pan_desc);
	ui_view_add_gesture(view, pan_gesture);

	return view;
}

// MARK: Color

typedef struct UI_Color_Data {
	vec4_f32 color;
} UI_Color_Data;

void ui_color_render(void* ctx, UI_View view, SL_Blitter* blitter) {
	UI_Color_Data* data = ctx;
	ui_render_util_draw_image(blitter, ui.current_lifecycle->texture, cvt_rect_s32_f32(ui_view_get_frame(view)), (Rect_f32) {}, data->color);
}

UI_View ui_color(UI_ID id, vec4_f32 color) {
	UI_Color_Data* color_data;
	allocator_new(ui_lifecycle_allocator(), color_data, 1);
	*color_data = (UI_Color_Data) {
		.color = color,
	};

	const UI_View_Callbacks callbacks = {
		.ctx = color_data,
		.render = ui_color_render,
	};
	return ui_view(id, &callbacks);
}

// MARK: Nine patch

typedef struct UI_Nine_Patch_Data {
	UI_Nine_Patch nine_patch;
	vec4_f32 tint;
} UI_Nine_Patch_Data;

void ui_nine_patch_render(void* ctx, UI_View view, SL_Blitter* blitter) {
	UI_Nine_Patch_Data* data = ctx;
	ui_render_util_draw_nine_patch(blitter, ui.current_lifecycle->texture, cvt_rect_s32_f32(ui_view_get_frame(view)), data->nine_patch, data->tint);
}

UI_View ui_nine_patch(UI_ID id, UI_Nine_Patch nine_patch, vec4_f32 tint) {
	UI_Nine_Patch_Data* nine_patch_data;
	allocator_new(ui_lifecycle_allocator(), nine_patch_data, 1);
	*nine_patch_data = (UI_Nine_Patch_Data) {
		.nine_patch = nine_patch,
		.tint = tint,
	};

	const UI_View_Callbacks callbacks = {
		.ctx = nine_patch_data,
		.render = ui_nine_patch_render,
	};
	return ui_view(id, &callbacks);
}

// MARK: Image

typedef struct UI_Image_Data {
	Gpu_Texture texture;
	Rect_u32 texture_rect;
	s32 scale;
	vec4_f32 tint;
} UI_Image_Data;

vec2_s32 ui_image_get_intrinsic_size(void* ctx, UI_View view) {
	UI_Image_Data* image_data = ctx;
	return mul_vec2_s32(cvt_vec2_u32_s32(rect_size_u32(image_data->texture_rect)), splat_vec2_s32(image_data->scale));
}

void ui_image_render(void* ctx, UI_View view, SL_Blitter* blitter) {
	UI_Image_Data* image_data = ctx;

	const Rect_s32 frame = ui_view_get_frame(view);

	const vec2_s32 scaled_size = mul_vec2_s32(cvt_vec2_u32_s32(rect_size_u32(image_data->texture_rect)), splat_vec2_s32(image_data->scale));
	const vec2_s32 frame_center = rect_centre_s32(frame);
	const vec2_s32 image_offset = sub_vec2_s32(frame_center, div_vec2_s32(scaled_size, splat_vec2_s32(2)));
	const Rect_f32 rect = {
		.start = cvt_vec2_s32_f32(image_offset),
		.end = cvt_vec2_s32_f32(add_vec2_s32(image_offset, scaled_size)),
	};

	ui_render_util_draw_image(blitter, image_data->texture, rect, cvt_rect_u32_f32(image_data->texture_rect), image_data->tint);
}

UI_View ui_image(UI_ID id, Gpu_Texture texture, Rect_u32 texture_rect, s32 scale, vec4_f32 tint) {
	UI_Image_Data* image_data;
	allocator_new(ui_lifecycle_allocator(), image_data, 1);
	*image_data = (UI_Image_Data) {
		.texture = texture,
		.texture_rect = texture_rect,
		.scale = scale,
		.tint = tint,
	};

	const UI_View_Callbacks callbacks = {
		.ctx = image_data,
		.get_intrinsic_size = ui_image_get_intrinsic_size,
		.render = ui_image_render,
	};
	return ui_view(id, &callbacks);
}

UI_View ui_spacer(UI_ID id) {
	const UI_View_Callbacks callbacks = {0};
	UI_View view = ui_view(id, &callbacks);
	view.data->flags |= UI_View_Flags_Fill_Available_Space;
	return view;
}

void ui_gesture_storage_destroy(void* data) {
	UI_Gesture_Storage* data0 = data;
	allocator_free(ui.allocator, data0, 1);
}

UI_Gesture ui_custom_gesture(UI_ID id, void* ctx, const UI_Gesture_VTable* vtable) {
	const UI_ID storage_id = ui_id_internal_auto(id);
	UI_Storage_Handle storage_handle;
	if (!ui_storage_get_handle(storage_id, &storage_handle)) {
		UI_Gesture_Storage* storage;
		allocator_new(ui.allocator, storage, 1);
		storage_handle = ui_storage_set_and_return_handle(storage_id, storage, ui_gesture_storage_destroy);
	}

	UI_Gesture_Storage* storage = ui_storage_handle_resolve(storage_handle)->data;
	*storage = (UI_Gesture_Storage) {
		.ctx = ctx,
		.vtable = vtable,
	};

	UI_Gesture_Data* gesture_data;
	allocator_new(ui_lifecycle_allocator(), gesture_data, 1);
	*gesture_data = (UI_Gesture_Data) {
		.storage_handle = (UI_Gesture_Storage_Handle) {
			.storage_handle = storage_handle,
		},
	};

	return (UI_Gesture) {
		.data = gesture_data,
		.lifecycle_index = ui.lifecycle_index,
	};
}

// MARK: Pan Gesture

typedef struct UI_Pan_Gesture_Data {
	UI_Pan_Gesture_Desc desc;
	UI_Touch* active_touch;
	vec2_f32 initial_position;
	vec2_f32 previous_position;
	vec2_f32 smoothed_velocity;
	f32 total_movement;
	f64 previous_timestamp;
} UI_Pan_Gesture_Data;

UI_Pan_Gesture_Frame ui_pan_gesture_resolve_frame(UI_Pan_Gesture_Data* pan, UI_Gesture_State state) {
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
void ui_pan_gesture_touch_began(void* ctx, UI_Touch* touch) {
	UI_Pan_Gesture_Data* state = ctx;

	if ((state->desc.touch_mask & (1 << touch->id.kind)) == 0) {
		return;
	}

	if (state->active_touch == NULL) {
		ui_touch_retain(touch);
		state->active_touch = touch;
		state->initial_position = touch->position;
		state->previous_position = touch->position;
		state->previous_timestamp = touch->timestamp;
		state->total_movement = 0.0f;

		const UI_Pan_Gesture_Frame frame = ui_pan_gesture_resolve_frame(state, UI_Gesture_State_Began);
		state->desc.callback.func(state->desc.callback.ctx, &frame);
	}
}
void ui_pan_gesture_touch_changed(void* ctx, UI_Touch* touch) {
	UI_Pan_Gesture_Data* state = ctx;

	if (state->active_touch == touch) {
		const UI_Pan_Gesture_Frame frame = ui_pan_gesture_resolve_frame(state, UI_Gesture_State_Changed);
		state->previous_position = touch->position;
		state->previous_timestamp = touch->timestamp;
		state->desc.callback.func(state->desc.callback.ctx, &frame);
	}
}
void ui_pan_gesture_touch_ended(void* ctx, UI_Touch* touch) {
	UI_Pan_Gesture_Data* state = ctx;

	if (state->active_touch == touch) {
		const UI_Pan_Gesture_Frame frame = ui_pan_gesture_resolve_frame(state, UI_Gesture_State_Ended);
		state->desc.callback.func(state->desc.callback.ctx, &frame);

		ui_touch_release(touch);
		state->active_touch = NULL;
	}
}
void ui_pan_gesture_touch_cancelled(void* ctx, UI_Touch* touch) {
	UI_Pan_Gesture_Data* state = ctx;

	if (state->active_touch == touch) {
		const UI_Pan_Gesture_Frame frame = ui_pan_gesture_resolve_frame(state, UI_Gesture_State_Cancelled);
		state->desc.callback.func(state->desc.callback.ctx, &frame);

		ui_touch_release(touch);
		state->active_touch = NULL;
	}
}
void ui_pan_gesture_destroy(void* ctx) {
	UI_Pan_Gesture_Data* state = ctx;
	if (state->active_touch != NULL) {
		ui_touch_release(state->active_touch);
		state->active_touch = NULL;
	}
	allocator_free(ui.allocator, state, 1);
}
const static UI_Gesture_VTable ui_pan_gesture_vtable = {
	.touch_began = ui_pan_gesture_touch_began,
	.touch_changed = ui_pan_gesture_touch_changed,
	.touch_ended = ui_pan_gesture_touch_ended,
	.touch_cancelled = ui_pan_gesture_touch_cancelled,
	.destroy = ui_pan_gesture_destroy,
};

UI_Gesture ui_pan_gesture(UI_ID id, const UI_Pan_Gesture_Desc* desc) {
	UI_Pan_Gesture_Data* gesture_data = ui_storage_get(id);
	if (!gesture_data) {
		allocator_new(ui.allocator, gesture_data, 1);
		*gesture_data = (UI_Pan_Gesture_Data) {0};
		ui_storage_set(id, gesture_data, ui_pan_gesture_destroy);
	}
	gesture_data->desc = *desc;

	return ui_custom_gesture(id, gesture_data, &ui_pan_gesture_vtable);
}

typedef struct UI_Tap_Gesture_Data {
	UI_Tap_Gesture_Desc desc;
	UI_Touch* active_touch;
	vec2_f32 previous_position;
	f32 total_movement;
} UI_Tap_Gesture_Data;

void ui_tap_gesture_touch_began(void* ctx, UI_Touch* touch) {
	UI_Tap_Gesture_Data* state = ctx;
	if (state->active_touch == NULL) {
		ui_touch_retain(touch);
		state->active_touch = touch;
		state->previous_position = touch->position;
		state->total_movement = 0.0f;
	}
}
void ui_tap_gesture_touch_changed(void* ctx, UI_Touch* touch) {
	UI_Tap_Gesture_Data* state = ctx;
	if (state->active_touch == touch) {
		state->total_movement += dist_vec2_f32(state->previous_position, state->active_touch->position);
		state->previous_position = touch->position;
	}
}
void ui_tap_gesture_touch_ended(void* ctx, UI_Touch* touch) {
	UI_Tap_Gesture_Data* state = ctx;
	if (state->active_touch == touch) {
		state->total_movement += dist_vec2_f32(state->previous_position, state->active_touch->position);

		if (state->total_movement < state->desc.maximum_movement) {
			const UI_Tap_Gesture_Frame frame = {
				.position = state->active_touch->position,
			};
			state->desc.callback.func(state->desc.callback.ctx, &frame);
		}

		ui_touch_release(touch);
		state->active_touch = NULL;
	}
}
void ui_tap_gesture_touch_cancelled(void* ctx, UI_Touch* touch) {
	UI_Tap_Gesture_Data* state = ctx;
	if (state->active_touch == touch) {
		ui_touch_release(touch);
		state->active_touch = NULL;
	}
}
void ui_tap_gesture_destroy(void* ctx) {
	UI_Tap_Gesture_Data* state = ctx;
	if (state->active_touch != NULL) {
		ui_touch_release(state->active_touch);
		state->active_touch = NULL;
	}
	allocator_free(ui.allocator, state, 1);
}
const static UI_Gesture_VTable ui_tap_gesture_vtable = {
	.touch_began = ui_tap_gesture_touch_began,
	.touch_changed = ui_tap_gesture_touch_changed,
	.touch_ended = ui_tap_gesture_touch_ended,
	.touch_cancelled = ui_tap_gesture_touch_cancelled,
	.destroy = ui_tap_gesture_destroy,
};

UI_Gesture ui_tap_gesture(UI_ID id, const UI_Tap_Gesture_Desc* desc) {
	UI_Tap_Gesture_Data* gesture_data = ui_storage_get(id);
	if (!gesture_data) {
		allocator_new(ui.allocator, gesture_data, 1);
		*gesture_data = (UI_Tap_Gesture_Data) {0};
		ui_storage_set(id, gesture_data, ui_tap_gesture_destroy);
	}
	gesture_data->desc = *desc;

	return ui_custom_gesture(id, gesture_data, &ui_tap_gesture_vtable);
}

// MARK: Menu

void ui_menu_stack_push(UI_Menu_Stack* stack, UI_Menu menu) {
	const bool max_menu_depth_exceeded = (stack->menu_depth >= UI_MENU_DEPTH_MAX);
	sl_debug_assert(!max_menu_depth_exceeded, "Exceeded maximum menu depth.");
	if (max_menu_depth_exceeded) {
		return;
	}

	stack->menus[stack->menu_depth++] = menu;
}
void ui_menu_stack_pop(UI_Menu_Stack* stack) {
	sl_debug_assert(stack->menu_depth > 0, "No menus to pop.");
	if (stack->menu_depth > 0) {
		stack->menu_depth--;
	}
}
void ui_menu_stack_clear(UI_Menu_Stack* stack) {
	stack->menu_depth = 0;
}

void ui_menu_stack_on_tap_off(void* ctx, const UI_Tap_Gesture_Frame* frame) {
	UI_Menu_Stack* stack = ctx;
	ui_menu_stack_clear(stack);
}

typedef struct UI_Menu_Stack_Context {

} UI_Menu_Stack_Context;

void ui_menu_stack_create_windows(UI_ID id, UI_Menu_Stack* stack, UI_Menu_Style style, UI_Menu_Callbacks callbacks) {
	if (stack->menu_depth == 0) {
		return;
	}

	const UI_View root = ui_view(ui_id_internal_auto(id), NULL);

	const UI_Tap_Gesture_Desc tap_off_gesture_desc = {
		.callback = {
			.ctx = stack,
			.func = ui_menu_stack_on_tap_off,
		},
		.maximum_movement = UI_TAP_GESTURE_MAXIMUM_MOVEMENT_DEFAULT,
	};
	ui_view_add_gesture(root, ui_tap_gesture(ui_id_internal_auto(id), &tap_off_gesture_desc));

	for (u32 depth = 0; depth < stack->menu_depth; depth++) {
		const UI_Menu_Item_Array menu_items = callbacks.get_menu_items(callbacks.ctx, stack->menus[depth].menu_id, ui_lifecycle_allocator());
		const UI_View view = style.create_view(style.ctx, ui_id_internal_auto_i(id, depth), menu_items);
		ui_view_add_subview(root, view);

		// TODO: Prevent going outside bounds of screen.
		const vec2_s32 position = stack->menus[depth].position;
		const vec2_s32 size = ui_view_get_intrinsic_size(view);
		const Rect_s32 frame = {
			.start = position,
			.end = add_vec2_s32(position, size),
		};
		ui_view_set_frame(view, frame);
	}

	UI_Window window = ui_window(ui_id_internal_auto(id), root, (UI_Window_Style) {0});
	ui_window_set_frame(window, ui.current_lifecycle->frame);
}
