#pragma once

#include <stanlib/core.h>

typedef enum Input_Touch_Kind {
	Input_Touch_Kind_Mouse,
	Input_Touch_Kind_Finger,
	Input_Touch_Kind_Pencil,
} Input_Touch_Kind;

typedef enum Input_Touch_Kind_Mask {
	Input_Touch_Kind_Mask_None = 0,
	Input_Touch_Kind_Mask_Mouse = 1 << Input_Touch_Kind_Mouse,
	Input_Touch_Kind_Mask_Finger = 1 << Input_Touch_Kind_Finger,
	Input_Touch_Kind_Mask_Pencil = 1 << Input_Touch_Kind_Pencil,
	Input_Touch_Kind_Mask_All = Input_Touch_Kind_Mask_Mouse | Input_Touch_Kind_Mask_Finger | Input_Touch_Kind_Mask_Pencil,
} Input_Touch_Kind_Mask;

typedef struct Input_Touch_ID {
	Input_Touch_Kind kind;
	u64 external;
} Input_Touch_ID;

typedef struct Input_Touch_Event {
	Input_Touch_ID id;
	vec2_f32 position;
	f64 timestamp;
} Input_Touch_Event;

typedef enum Input_Event_Kind {
	Input_Event_Kind_Touch_Began,
	Input_Event_Kind_Touch_Changed,
	Input_Event_Kind_Touch_Ended,
	Input_Event_Kind_Touch_Cancelled,
	Input_Event_Kind_Key_Down,
	Input_Event_Kind_Key_Up,
} Input_Event_Kind;

typedef enum Input_Key {
	Input_Key_Unknown,

	Input_Key_A, Input_Key_B, Input_Key_C, Input_Key_D, Input_Key_E, Input_Key_F, Input_Key_G, Input_Key_H,
	Input_Key_I, Input_Key_J, Input_Key_K, Input_Key_L, Input_Key_M, Input_Key_N, Input_Key_O, Input_Key_P,
	Input_Key_Q, Input_Key_R, Input_Key_S, Input_Key_T, Input_Key_U, Input_Key_V, Input_Key_W, Input_Key_X,
	Input_Key_Y, Input_Key_Z, Input_Key_0, Input_Key_1, Input_Key_2, Input_Key_3, Input_Key_4, Input_Key_5,
	Input_Key_6, Input_Key_7, Input_Key_8, Input_Key_9, Input_Key_Space, Input_Key_LCtrl, Input_Key_RCtrl, Input_Key_LAlt,
	Input_Key_RAlt, Input_Key_LShift, Input_Key_RShift, Input_Key_Enter, Input_Key_Tab, Input_Key_Up, Input_Key_Down, Input_Key_Left,
	Input_Key_Right, Input_Key_Backspace, Input_Key_Delete,

	Input_Key_Count,
} Input_Key;

typedef struct Input_Event {
	Input_Event_Kind kind;
	union {
		Input_Touch_Event touch;
		Input_Key key;
	};
} Input_Event;

typedef struct Input_Tracker Input_Tracker;

typedef struct Input_Event_Iterator {
	Input_Tracker* tracker;
	u32 index;
} Input_Event_Iterator;

Input_Tracker* input_tracker_new(Allocator* allocator);
void input_tracker_destroy(Input_Tracker* tracker);

void input_tracker_push_event(Input_Tracker* tracker, Input_Event event);
void input_tracker_clear_events(Input_Tracker* tracker);

bool input_tracker_get_key_down(Input_Tracker* tracker, Input_Key key);

Input_Event_Iterator input_tracker_get_event_iterator(Input_Tracker* tracker);
bool input_event_iterator_get_next(Input_Event_Iterator* iterator, Input_Event* event);
