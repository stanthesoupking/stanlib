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
	SL_Font_Atlas* font;
	UI_Padding text_padding;
	UI_Button_State_Style state[UI_BUTTON_STATE_COUNT];
} UI_Button_Style;

typedef struct UI_Slider_Style {
	Rect_f32 needle_image;
	vec4_f32 needle_color;
	vec4_f32 track_color;
	f32 track_height;
} UI_Slider_Style;

typedef struct UI_Label_Style {
	SL_Font_Atlas* font;
	vec4_f32 color;
} UI_Label_Style;

typedef struct UI UI;

typedef struct UI_ID {
	u64 hash;
} UI_ID;
#define UI_ID_NULL ((UI_ID) { .hash = 0ULL, })

UI_ID ui_id(UI_ID parent, u32 item, u32 index);

typedef enum UI_Event_Kind {
	UI_Event_Kind_Mouse_Move,
	UI_Event_Kind_Mouse_Mouse_Down,
	UI_Event_Kind_Mouse_Mouse_Up,
} UI_Event_Kind;

typedef struct UI_Event_Mouse_Move {
	vec2_f32 position;
} UI_Event_Mouse_Move;

typedef struct UI_Event {
	UI_Event_Kind kind;
	union {
		UI_Event_Mouse_Move mouse_move;
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

typedef struct UI_Element UI_Element;

UI* ui_new(Allocator* allocator, Gpu_Texture texture);
void ui_destroy(UI* ui);

void ui_begin(UI* ui, UI_Event_Seq* event_sink);
void ui_end(UI* ui);

void ui_begin_frame(UI* ui, Rect_f32 rect);
void ui_end_frame(UI* ui);

// ill available space in a hstack/vstack
UI_Element* ui_spacer(UI* ui);

UI_Element* ui_push_hstack(UI* ui, UI_Extent extent, UI_Padding padding, UI_Vertical_Alignment alignment, f32 spacing);
UI_Element* ui_push_vstack(UI* ui, UI_Extent extent, UI_Padding padding, UI_Horizontal_Alignment alignment, f32 spacing);
UI_Element* ui_push_zstack(UI* ui, UI_Extent extent, UI_Padding padding);

void ui_pop(UI* ui);

UI_Element* ui_color(UI* ui, UI_Extent extent, vec4_f32 color);
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

void ui_render(UI* ui, SL_Blitter* blitter);
