#include <stanlib/input.h>

sl_seq(Input_Event, Input_Event_Seq, input_event_seq);

typedef struct Input_Tracker {
	Allocator* allocator;
	bool key_down[Input_Key_Count];
	Input_Event_Seq events;
} Input_Tracker;

Input_Tracker* input_tracker_new(Allocator* allocator) {
	Input_Tracker* tracker;
	allocator_new(allocator, tracker, 1);
	*tracker = (Input_Tracker) {
		.allocator = allocator,
		.events = input_event_seq_new(allocator, 8),
	};
	return tracker;
}
void input_tracker_destroy(Input_Tracker* tracker) {
	Allocator* allocator = tracker->allocator;
	allocator_free(allocator, tracker, 1);
}

void input_tracker_push_event(Input_Tracker* tracker, Input_Event event) {
	switch (event.kind) {
		case Input_Event_Kind_Key_Down: {
			tracker->key_down[event.key] = true;
		} break;

		case Input_Event_Kind_Key_Up: {
			tracker->key_down[event.key] = false;
		} break;

		default:
			break;
	}

	input_event_seq_push(&tracker->events, event);
}
void input_tracker_clear_events(Input_Tracker* tracker) {
	input_event_seq_clear(&tracker->events);
}

bool input_tracker_get_key_down(Input_Tracker* tracker, Input_Key key) {
	return tracker->key_down[key];
}

Input_Event_Iterator input_tracker_get_event_iterator(Input_Tracker* tracker) {
	return (Input_Event_Iterator) {
		.tracker = tracker,
		.index = 0,
	};
}
bool input_event_iterator_get_next(Input_Event_Iterator* iterator, Input_Event* event) {
	if (iterator->index < input_event_seq_get_count(&iterator->tracker->events)) {
		*event = input_event_seq_get(&iterator->tracker->events, iterator->index++);
		return true;
	} else {
		return false;
	}
}
