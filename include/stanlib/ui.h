#pragma once

#include <stanlib/core.h>
#include <stanlib/text.h>
#include <stanlib/blitter.h>

typedef struct UI_Margins {
	u32 top, bottom, left, right;
} UI_Margins;

typedef struct UI_Nine_Patch {
	Rect_u32 atlas_region;
	UI_Margins margins;
} UI_Nine_Patch;

typedef struct UI_Padding {
	f32 top, bottom, left, right;
} UI_Padding;

#define UI_PADDING_NONE ((UI_Padding) {})

sl_inline UI_Padding ui_padding_uniform(f32 amount) {
	return (UI_Padding) { amount, amount, amount, amount };
}

typedef enum UI_Button_State {
	UI_Button_State_Normal,
	UI_Button_State_Hot,
	UI_Button_State_Active
} UI_Button_State;
#define UI_BUTTON_STATE_COUNT 3

typedef struct UI_Button_State_Style {
	vec2_f32 label_offset;
	vec4_f32 label_color;
	UI_Nine_Patch backing;
	vec4_f32 backing_color;
} UI_Button_State_Style;

typedef struct UI_Button_Style {
	SL_Font* font;
	Gpu_Texture texture;
	UI_Padding text_padding;
	UI_Button_State_Style state[UI_BUTTON_STATE_COUNT];
} UI_Button_Style;

typedef struct UI_Slider_Style {
	Gpu_Texture texture;
	Rect_u32 needle_image;
	vec4_f32 needle_color;
	vec4_f32 track_color;
	f32 track_height;
} UI_Slider_Style;

typedef struct UI_Label_Style {
	SL_Font* font;
	Gpu_Texture texture;
	vec4_f32 color;
} UI_Label_Style;

typedef struct UI UI;

typedef struct UI_ID {
	u64 hash;
} UI_ID;
#define UI_ID_NULL ((UI_ID) { .hash = 0ULL, })

UI_ID ui_id(UI_ID parent, u32 item, u32 index);

typedef enum UI_Touch_Kind {
	UI_Touch_Kind_Mouse,
	UI_Touch_Kind_Finger,
	UI_Touch_Kind_Pencil,
} UI_Touch_Kind;

typedef enum UI_Touch_Kind_Mask {
	UI_Touch_Kind_Mask_None = 0,
	UI_Touch_Kind_Mask_Mouse = 1 << UI_Touch_Kind_Mouse,
	UI_Touch_Kind_Mask_Finger = 1 << UI_Touch_Kind_Finger,
	UI_Touch_Kind_Mask_Pencil = 1 << UI_Touch_Kind_Pencil,
	UI_Touch_Kind_Mask_All = UI_Touch_Kind_Mask_Mouse | UI_Touch_Kind_Mask_Finger | UI_Touch_Kind_Mask_Pencil,
} UI_Touch_Kind_Mask;

typedef struct UI_Touch_ID {
	UI_Touch_Kind kind;
	u64 external;
} UI_Touch_ID;

typedef enum UI_Touch_State : u8 {
	UI_Touch_State_Alive,
	UI_Touch_State_Ended,
	UI_Touch_State_Cancelled,
} UI_Touch_State;

typedef struct UI_Touch UI_Touch;

typedef struct UI_Touch_Event {
	UI_Touch_ID id;
	vec2_f32 position;
	f64 timestamp;
} UI_Touch_Event;

typedef enum UI_Event_Kind {
	UI_Event_Kind_Touch_Began,
	UI_Event_Kind_Touch_Changed,
	UI_Event_Kind_Touch_Ended,
	UI_Event_Kind_Touch_Cancelled,
} UI_Event_Kind;

typedef struct UI_Event_Mouse_Move {
	vec2_f32 position;
} UI_Event_Mouse_Move;

typedef struct UI_Event {
	UI_Event_Kind kind;
	union {
		UI_Event_Mouse_Move mouse_move;
		UI_Touch_Event touch;
	};
} UI_Event;
sl_seq(UI_Event, UI_Event_Seq, ui_event_seq);

typedef enum UI_Direction {
	UI_Direction_Horizontal,
	UI_Direction_Vertical,
} UI_Direction;

typedef enum UI_Horizontal_Alignment {
	UI_Horizontal_Alignment_Center,
	UI_Horizontal_Alignment_Left,
	UI_Horizontal_Alignment_Right
} UI_Horizontal_Alignment;

typedef enum UI_Vertical_Alignment {
	UI_Vertical_Alignment_Center,
	UI_Vertical_Alignment_Top,
	UI_Vertical_Alignment_Bottom
} UI_Vertical_Alignment;

#define UI_EXTENT_NONE ((UI_Extent) {})

typedef struct UI_Extent {
	f32 min_width;
	f32 max_width;

	f32 min_height;
	f32 max_height;
} UI_Extent;

#define UI_FILL -1.0f
#define UI_EXTENT_IMPLICIT 0.0f

#define UI_EXTENT_NONE ((UI_Extent) {})
#define UI_EXTENT_FILL ((UI_Extent) { .max_width = UI_FILL, .max_height = UI_FILL })

typedef struct UI_Callback {
	void* ctx;
	void (*func)(void* ctx);
} UI_Callback;
#define UI_CALLBACK_NULL ((UI_Callback) {})

typedef struct UI_Render_Callback {
	void* ctx;
	void (*render)(void* ctx, Rect_f32 rect, SL_Blitter* blitter);
} UI_Render_Callback;

// MARK: Gesture

typedef struct UI_Gesture_VTable {
	void (*touch_began)(UI* ui, void* ctx, UI_Touch* touch);
	void (*touch_changed)(UI* ui, void* ctx, UI_Touch* touch);
	void (*touch_ended)(UI* ui, void* ctx, UI_Touch* touch);
	void (*touch_cancelled)(UI* ui, void* ctx, UI_Touch* touch);
	void (*destroy)(UI* ui, void* ctx);
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
	u32 minimum_touches;
	u32 maximum_touches;
	UI_Pan_Gesture_Callback callback;
} UI_Pan_Gesture_Desc;

typedef struct UI_Atlas UI_Atlas;

typedef enum UI_Atlas_Entry_Kind {
	UI_Atlas_Entry_Kind_Image,
	UI_Atlas_Entry_Kind_Font,
} UI_Atlas_Entry_Kind;

typedef struct UI_Atlas_Entry_Image {
	Immutable_Buffer buffer;
	vec2_u32 size;
	u32 row_length; // bytes
} UI_Atlas_Entry_Image;

typedef struct UI_Atlas_Entry_Font {
	Immutable_Buffer buffer;
	f32 size;
} UI_Atlas_Entry_Font;

typedef struct UI_Atlas_Entry {
	UI_Atlas_Entry_Kind kind;
	union {
		UI_Atlas_Entry_Image image;
		UI_Atlas_Entry_Font font;
	};
} UI_Atlas_Entry;

typedef struct UI_Atlas_Desc {
	Allocator* allocator;

	const UI_Atlas_Entry* entries;
	u32 entry_count;

	Gpu_Command_Buffer command_buffer;
	Gpu_Slice* inout_staging_slice;
	Gpu_Slice* inout_persistent_slice;
} UI_Atlas_Desc;

typedef struct UI_Atlas_Handle {
	UI_Atlas* atlas;
	u32 index;
} UI_Atlas_Handle;

UI_Atlas* ui_atlas_new(const UI_Atlas_Desc* desc);
void ui_atlas_destroy(UI_Atlas* atlas);

typedef struct UI_Element UI_Element;

UI* ui_new(Allocator* allocator);
void ui_destroy(UI* ui);

void ui_begin(UI* ui);
void ui_end(UI* ui, UI_Event_Seq* event_sink);

void ui_begin_frame(UI* ui, Rect_f32 rect);
void ui_end_frame(UI* ui);

void ui_debug_touches(UI* ui, SL_Font* font, Gpu_Texture texture);

// Fill available space in a hstack/vstack
UI_Element* ui_spacer(UI* ui);

UI_Element* ui_push_hstack(UI* ui, UI_Extent extent, UI_Padding padding, UI_Vertical_Alignment alignment, f32 spacing);
UI_Element* ui_push_vstack(UI* ui, UI_Extent extent, UI_Padding padding, UI_Horizontal_Alignment alignment, f32 spacing);
UI_Element* ui_push_zstack(UI* ui, UI_Extent extent, UI_Padding padding);

void ui_pop(UI* ui);

UI_Element* ui_color(UI* ui, UI_Extent extent, Gpu_Texture texture, vec4_f32 color);
UI_Element* ui_button(UI* ui, UI_ID id, UI_Extent extent, const UI_Button_Style* style, const char* label, UI_Callback on_press);
UI_Element* ui_slider_f32(UI* ui, UI_ID id, UI_Extent extent, const UI_Slider_Style* style, f32* value, Range_f32 range, UI_Callback on_change);
UI_Element* ui_label(UI* ui, UI_Extent extent, const UI_Label_Style* style, const char* label);

// Custom element rendered using a callback.
UI_Element* ui_custom(UI* ui, UI_Extent extent, UI_Render_Callback on_render);

// Get the rect after layout for the given element.
//
// Note #1: An element will only have a rect once layout has run (after calling ui_end()).
// Note #2: When the element is culled, this function returns `false`.
bool ui_element_get_layout_rect(UI* ui, UI_Element* element, Rect_f32* out_rect);

void ui_pan_gesture(UI* ui, UI_ID id, const UI_Pan_Gesture_Desc* desc, UI_Element* element);

void ui_render(UI* ui, SL_Blitter* blitter);
