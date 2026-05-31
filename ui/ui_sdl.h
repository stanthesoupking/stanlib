#pragma once

#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

sl_inline bool ui_event_from_sdl_event(SDL_Window* window, const SDL_Event* event, UI_Event* out_event) {
	switch (event->type) {
		case SDL_EVENT_MOUSE_MOTION: {
			const f32 window_scale = SDL_GetWindowDisplayScale(window);
			*out_event = (UI_Event) {
				.kind = UI_Event_Kind_Mouse_Move,
				.mouse_move = {
					.position = { event->motion.x * window_scale, event->motion.y * window_scale },
				},
			};
			return true;
		} break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN: {
			*out_event = (UI_Event) {
				.kind = UI_Event_Kind_Mouse_Mouse_Down,
			};
			return true;
		} break;

		case SDL_EVENT_MOUSE_BUTTON_UP: {
			*out_event = (UI_Event) {
				.kind = UI_Event_Kind_Mouse_Mouse_Up,
			};
			return true;
		} break;

		default: {
			// Unhandled
		} break;
	}

	return false;
}
