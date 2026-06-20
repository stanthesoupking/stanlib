#pragma once

#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

sl_inline UI_Touch_Event ui_touch_event_from_sdl_mouse_motion_event(SDL_Window* window, const SDL_MouseMotionEvent* event) {
	// TODO: Report position in points
	const f32 window_scale = SDL_GetWindowDisplayScale(window);
	
	return (UI_Touch_Event) {
		.id = {
			.kind = UI_Touch_Kind_Mouse,
			.external = 0,
		},
		.position = {
			.x = event->x * window_scale,
			.y = event->y * window_scale,
		},
		.timestamp = event->timestamp / 1000000000.0,
	};
}

sl_inline UI_Touch_Event ui_touch_event_from_sdl_mouse_button_event(SDL_Window* window, const SDL_MouseButtonEvent* event) {
	// TODO: Report position in points
	const f32 window_scale = SDL_GetWindowDisplayScale(window);
	
	return (UI_Touch_Event) {
		.id = {
			.kind = UI_Touch_Kind_Mouse,
			.external = 0,
		},
		.position = {
			.x = event->x * window_scale,
			.y = event->y * window_scale,
		},
		.timestamp = event->timestamp / 1000000000.0,
	};
}

sl_inline UI_Touch_Event ui_touch_event_from_sdl_touch_event(SDL_Window* window, const SDL_TouchFingerEvent* event) {
	int window_w, window_h;
	SDL_GetWindowSize(window, &window_w, &window_h);
	
	// TODO: Report position in points
	const f32 window_scale = SDL_GetWindowDisplayScale(window);
	
	return (UI_Touch_Event) {
		.id = {
			.kind = UI_Touch_Kind_Finger,
			.external = event->fingerID,
		},
		.position = {
			.x = event->x * (f32)window_w * window_scale,
			.y = event->y * (f32)window_h * window_scale,
		},
		.timestamp = event->timestamp / 1000000000.0,
	};
}

sl_inline bool ui_event_from_sdl_event(SDL_Window* window, const SDL_Event* event, UI_Event* out_event) {
	switch (event->type) {
		case SDL_EVENT_MOUSE_MOTION: {
			// Handled via touch
			if (event->motion.which == SDL_TOUCH_MOUSEID) {
				return false;
			}
			
			*out_event = (UI_Event) {
				.kind = UI_Event_Kind_Touch_Changed,
				.touch = ui_touch_event_from_sdl_mouse_motion_event(window, &event->motion),
			};
			return true;
		} break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN: {
			// Handled via touch
			if (event->button.which == SDL_TOUCH_MOUSEID) {
				return false;
			}
			
			*out_event = (UI_Event) {
				.kind = UI_Event_Kind_Touch_Began,
				.touch = ui_touch_event_from_sdl_mouse_button_event(window, &event->button),
			};
			return true;
		} break;

		case SDL_EVENT_MOUSE_BUTTON_UP: {
			// Handled via touch
			if (event->button.which == SDL_TOUCH_MOUSEID) {
				return false;
			}
			
			*out_event = (UI_Event) {
				.kind = UI_Event_Kind_Touch_Ended,
				.touch = ui_touch_event_from_sdl_mouse_button_event(window, &event->button),
			};
			return true;
		} break;
			
		case SDL_EVENT_FINGER_DOWN: {
			*out_event = (UI_Event) {
				.kind = UI_Event_Kind_Touch_Began,
				.touch = ui_touch_event_from_sdl_touch_event(window, &event->tfinger),
			};
			return true;
		} break;
			
		case SDL_EVENT_FINGER_UP: {
			*out_event = (UI_Event) {
				.kind = UI_Event_Kind_Touch_Ended,
				.touch = ui_touch_event_from_sdl_touch_event(window, &event->tfinger),
			};
			return true;
		} break;
			
		case SDL_EVENT_FINGER_MOTION: {
			*out_event = (UI_Event) {
				.kind = UI_Event_Kind_Touch_Changed,
				.touch = ui_touch_event_from_sdl_touch_event(window, &event->tfinger),
			};
			return true;
		} break;
			
		case SDL_EVENT_FINGER_CANCELED: {
			*out_event = (UI_Event) {
				.kind = UI_Event_Kind_Touch_Cancelled,
				.touch = ui_touch_event_from_sdl_touch_event(window, &event->tfinger),
			};
			return true;
		} break;

		default: {
			// Unhandled
		} break;
	}

	return false;
}
