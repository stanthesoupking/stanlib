#pragma once

#include "../core.h"
#include "../text/text.h"
#include "../blitter/blitter.h"

// UI is processed front to back

typedef struct UI_Margins {
	u32 top, bottom, left, right;
} UI_Margins;

typedef struct UI_Nine_Patch {
	Rect_u32 atlas_region;
	UI_Margins margins;
} UI_Nine_Patch;

typedef enum UI_Button_State {
	UI_Button_State_Normal,
	UI_Button_State_Hot,
	UI_Button_State_Active
} UI_Button_State;
#define UI_BUTTON_STATE_COUNT 3

typedef struct UI_Button_State_Style {
	vec2_f32 label_offset;
	vec4_f32 label_color;
	UI_Nine_Patch nine_patch;
} UI_Button_State_Style;

typedef struct UI_Button_Style {
	SL_Font_Atlas* font;
	UI_Button_State_Style state[UI_BUTTON_STATE_COUNT];
} UI_Button_Style;

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

UI* ui_new(Allocator* allocator, Gpu_Texture texture);
void ui_destroy(UI* ui);

void ui_begin(UI* ui, UI_Event_Seq* event_sink);
void ui_end(UI* ui);

void ui_begin_panel(UI* ui, Rect_f32 rect, vec4_f32 color);
void ui_end_panel(UI* ui);

bool ui_button(UI* ui, UI_ID id, const char* label, UI_Button_Style style, Rect_f32 rect);

void ui_render(UI* ui, SL_Blitter* blitter);
