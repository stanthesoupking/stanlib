// Notes:
//
// - Everything is a window, including the root and context menus.
// - When the parent window is resized, child windows are repositioned proportionally.
// - Window has various modes?
//   - Decorations
//   - Movable
//   - Resizable
// - Context Menu
// 	 - Spawn an additional window, that dismisses the context menu when tapped.

#pragma once
#include "stanlib/gpu.h"
#include "stanlib/string.h"
#include "stanlib/text.h"
#include <stanlib/blitter.h>
#include <stanlib/core.h>
#include <stanlib/input.h>

typedef enum UI_Window_Flags {
	UI_Window_Flags_None,
	UI_Window_Flags_Decorated
} UI_Window_Flags;

typedef enum UI_Axis {
	UI_Axis_Horizontal,
	UI_Axis_Vertical,
} UI_Axis;

typedef enum UI_Alignment {
	UI_Alignment_Bottom,
	UI_Alignment_Center,
	UI_Alignment_Top,

	UI_Alignment_Left = UI_Alignment_Bottom, // Alias
	UI_Alignment_Right = UI_Alignment_Top, // Alias
} UI_Alignment;

typedef struct UI_Padding {
	s32 top, bottom, left, right;
} UI_Padding;

#define UI_PADDING_NONE ((UI_Padding) {})

sl_inline UI_Padding ui_padding_uniform(s32 amount) {
	return (UI_Padding) { amount, amount, amount, amount };
}

sl_inline UI_Padding ui_padding_add(UI_Padding a, UI_Padding b) {
	return (UI_Padding) {
		.left = a.left + b.left,
		.right = a.right + b.right,
		.top = a.top + b.top,
		.bottom = a.bottom + b.bottom,
	};
}
Rect_s32 ui_padded_rect(Rect_s32 rect, UI_Padding padding);

typedef struct UI_Margins {
	u32 top, bottom, left, right;
} UI_Margins;

typedef struct UI_Nine_Patch {
	Rect_u32 atlas_region;
	UI_Margins margins;
} UI_Nine_Patch;

typedef struct UI_ID { u64 hash; } UI_ID;
#define UI_ID_NULL ((UI_ID) { .hash = 0ULL, })
UI_ID ui_id(UI_ID parent, u32 item, u32 index);

// Opaque
typedef struct UI_Window_Data UI_Window_Data;
typedef struct UI_View_Data UI_View_Data;
typedef struct UI_Gesture_Data UI_Gesture_Data;

typedef struct UI_Window { UI_Window_Data* data; u32 lifecycle_index; } UI_Window;
typedef struct UI_View { UI_View_Data* data; u32 lifecycle_index; } UI_View;
typedef struct UI_Gesture { UI_Gesture_Data* data; u32 lifecycle_index; } UI_Gesture;

typedef struct UI_Limits {
	vec2_s32 min;
	vec2_s32 max;
} UI_Limits;

typedef struct UI_View_Callbacks {
	void* ctx;
	void (*layout_subviews)(void* ctx, UI_View view);
	UI_Limits (*get_limits)(void* ctx, UI_View view);
	void (*pre_render)(void* ctx, UI_View view);
	void (*render)(void* ctx, UI_View view, SL_Blitter* blitter);
} UI_View_Callbacks;

typedef struct UI_Label_Style {
	SL_Font* font;
	vec4_f32 color;
} UI_Label_Style;

typedef struct UI_Window_Heading_Style {
	UI_Label_Style label_style;
	s32 height;

	UI_Nine_Patch background_9p;
	vec4_f32 background_tint;
} UI_Window_Heading_Style;

typedef struct UI_Window_Style {
	bool has_heading;
	UI_Window_Heading_Style heading_style;

	UI_Nine_Patch content_background_9p;
	vec4_f32 content_background_tint;
	UI_Padding content_padding;
} UI_Window_Style;

typedef enum UI_Button_State {
	UI_Button_State_Normal,
	UI_Button_State_Hot,
	UI_Button_State_Active,
	UI_Button_State_Count,
} UI_Button_State;

typedef struct UI_Button_State_Style {
	vec4_f32 text_color;
	UI_Nine_Patch backing;
	vec4_f32 backing_tint;
} UI_Button_State_Style;

typedef struct UI_Button_Style {
	bool has_text;
	SL_Font* font;
	UI_Padding padding;
	UI_Button_State_Style state[UI_Button_State_Count];
} UI_Button_Style;

typedef struct UI_Slider_Style {
	vec4_f32 track_color;
	s32 track_height;

	Rect_u32 needle_rect;
	vec4_f32 needle_tint;
	s32 needle_scale;
} UI_Slider_Style;

typedef struct UI_Slider_Callbacks {
	void* ctx;

	void (*set_value)(void* ctx, f32 value);
	f32 (*get_value)(void* ctx);
	void (*on_begin)(void* ctx);
	void (*on_end)(void* ctx);
} UI_Slider_Callbacks;

typedef struct UI_Touch UI_Touch;

Input_Touch_Kind ui_touch_get_kind(UI_Touch* touch);
vec2_f32 ui_touch_get_position(UI_Touch* touch);
bool ui_touch_equals(UI_Touch* a, UI_Touch* b);

typedef struct UI_Gesture_VTable {
	void (*touch_began)(void* ctx, UI_Touch* touch);
	void (*touch_changed)(void* ctx, UI_Touch* touch);
	void (*touch_ended)(void* ctx, UI_Touch* touch);
	void (*touch_cancelled)(void* ctx, UI_Touch* touch);
	void (*destroy)(void* ctx);
} UI_Gesture_VTable;

typedef enum UI_Gesture_State {
	UI_Gesture_State_Began,
	UI_Gesture_State_Changed,
	UI_Gesture_State_Ended,
	UI_Gesture_State_Cancelled,
} UI_Gesture_State;

typedef struct UI_Pan_Gesture_Frame {
	UI_Gesture_State state;
	vec2_f32 position;
	vec2_f32 translation;
	vec2_f32 velocity;
	f32 total_movement;
} UI_Pan_Gesture_Frame;

typedef struct UI_Pan_Gesture_Callback {
	void* ctx;
	void (*func)(void* ctx, const UI_Pan_Gesture_Frame* frame);
} UI_Pan_Gesture_Callback;

typedef struct UI_Pan_Gesture_Desc {
	Input_Touch_Kind_Mask touch_mask;
	u32 minimum_touches;
	u32 maximum_touches;
	UI_Pan_Gesture_Callback callback;
} UI_Pan_Gesture_Desc;

typedef struct UI_Tap_Gesture_Frame {
	vec2_f32 position;
} UI_Tap_Gesture_Frame;

typedef struct UI_Tap_Gesture_Callback {
	void* ctx;
	void (*func)(void* ctx, const UI_Tap_Gesture_Frame* frame);
} UI_Tap_Gesture_Callback;

#define UI_TAP_GESTURE_MAXIMUM_MOVEMENT_DEFAULT 10.0f

typedef struct UI_Tap_Gesture_Desc {
	// Default recommended value is UI_TAP_GESTURE_MAXIMUM_MOVEMENT_DEFAULT.
	f32 maximum_movement;

	UI_Tap_Gesture_Callback callback;
} UI_Tap_Gesture_Desc;

typedef struct UI_Callback {
	void* ctx;
	void (*func)(void* ctx);
} UI_Callback;
#define UI_CALLBACK_NULL ((UI_Callback) {})

typedef struct UI_Desc {
	Allocator* allocator;
} UI_Desc;

// MARK: UI
void ui_init(const UI_Desc* desc);
void ui_deinit();

// MARK: Lifecycle
void ui_lifecycle_begin(Rect_s32 frame, Gpu_Texture texture);
void ui_lifecycle_interact(Input_Tracker* input_tracker);
void ui_lifecycle_pre_render(void);
void ui_lifecycle_render(SL_Blitter* blitter);
void ui_lifecycle_end();

// MARK: Window
UI_Window ui_window(UI_ID id, UI_View content, SL_String_View title, const UI_Window_Style* style);
bool ui_window_is_appearing(UI_Window window);
Rect_s32 ui_window_get_frame(UI_Window window);
void ui_window_set_frame(UI_Window window, Rect_s32 frame);
void ui_window_send_to_front(UI_Window window);

// MARK: View
UI_View ui_view(UI_ID id, const UI_View_Callbacks* callbacks);
Rect_s32 ui_view_get_frame(UI_View view);
void ui_view_set_frame(UI_View view, Rect_s32 frame);
void ui_view_layout(UI_View view);
UI_Limits ui_view_get_limits(UI_View view);
void ui_view_add_subview(UI_View parent, UI_View child);
void ui_view_add_gesture(UI_View view, UI_Gesture gesture);

// MARK: View Types
UI_View ui_stack(UI_ID id, UI_Axis axis, UI_Alignment alignment, s32 spacing);
UI_View ui_label(UI_ID id, SL_String_View text, const UI_Label_Style* style);
UI_View ui_button(UI_ID id, SL_String_View text, const UI_Button_Style* style, UI_Callback on_press);
UI_View ui_slider(UI_ID id, const UI_Slider_Style* style, Range_f32 range, UI_Slider_Callbacks callbacks);
UI_View ui_color(UI_ID id, vec4_f32 color);
UI_View ui_nine_patch(UI_ID id, UI_Nine_Patch nine_patch, vec4_f32 tint);
UI_View ui_spacer(UI_ID id);

// MARK: Gesture
UI_Gesture ui_custom_gesture(UI_ID id, void* ctx, const UI_Gesture_VTable* vtable);
UI_Gesture ui_pan_gesture(UI_ID id, const UI_Pan_Gesture_Desc* desc);
UI_Gesture ui_tap_gesture(UI_ID id, const UI_Tap_Gesture_Desc* desc);

// MARK: Storage
// Store persistent data between lifecycles.
// If a data is not accessed in a lifecycle, it is cleaned up.
void* ui_storage_get(UI_ID id);
void ui_storage_set(UI_ID id, void* data, void (*destroy)(void* data));
