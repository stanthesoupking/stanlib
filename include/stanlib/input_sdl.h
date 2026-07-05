#pragma once

#include <stanlib/input.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

sl_inline Input_Touch_Event input_touch_event_from_sdl_mouse_motion_event(SDL_Window* window, const SDL_MouseMotionEvent* event) {
	// TODO: Report position in points
	const f32 window_scale = SDL_GetWindowDisplayScale(window);

	return (Input_Touch_Event) {
		.id = {
			.kind = Input_Touch_Kind_Mouse,
			.external = 0,
		},
		.position = {
			.x = event->x * window_scale,
			.y = event->y * window_scale,
		},
		.timestamp = event->timestamp / 1000000000.0,
	};
}

sl_inline Input_Touch_Event input_touch_event_from_sdl_mouse_button_event(SDL_Window* window, const SDL_MouseButtonEvent* event) {
	// TODO: Report position in points
	const f32 window_scale = SDL_GetWindowDisplayScale(window);

	return (Input_Touch_Event) {
		.id = {
			.kind = Input_Touch_Kind_Mouse,
			.external = 0,
		},
		.position = {
			.x = event->x * window_scale,
			.y = event->y * window_scale,
		},
		.timestamp = event->timestamp / 1000000000.0,
	};
}

sl_inline Input_Touch_Event input_touch_event_from_sdl_touch_event(SDL_Window* window, const SDL_TouchFingerEvent* event) {
	int window_w, window_h;
	SDL_GetWindowSize(window, &window_w, &window_h);

	// TODO: Report position in points
	const f32 window_scale = SDL_GetWindowDisplayScale(window);

	return (Input_Touch_Event) {
		.id = {
			.kind = Input_Touch_Kind_Finger,
			.external = event->fingerID,
		},
		.position = {
			.x = event->x * (f32)window_w * window_scale,
			.y = event->y * (f32)window_h * window_scale,
		},
		.timestamp = event->timestamp / 1000000000.0,
	};
}

sl_inline Input_Key input_key_from_sdl_keycode(SDL_Keycode keycode) {
	switch (keycode) {
		case SDLK_A: return Input_Key_A;
		case SDLK_B: return Input_Key_B;
		case SDLK_C: return Input_Key_C;
	 	case SDLK_D: return Input_Key_D;
		case SDLK_E: return Input_Key_E;
		case SDLK_F: return Input_Key_F;
		case SDLK_G: return Input_Key_G;
		case SDLK_H: return Input_Key_H;
		case SDLK_I: return Input_Key_I;
		case SDLK_J: return Input_Key_J;
		case SDLK_K: return Input_Key_K;
		case SDLK_L: return Input_Key_L;
		case SDLK_M: return Input_Key_M;
		case SDLK_N: return Input_Key_N;
		case SDLK_O: return Input_Key_O;
	 	case SDLK_P: return Input_Key_P;
		case SDLK_Q: return Input_Key_Q;
		case SDLK_R: return Input_Key_R;
		case SDLK_S: return Input_Key_S;
		case SDLK_T: return Input_Key_T;
		case SDLK_U: return Input_Key_U;
		case SDLK_V: return Input_Key_V;
		case SDLK_W: return Input_Key_W;
		case SDLK_X: return Input_Key_X;
		case SDLK_Y: return Input_Key_Y;
		case SDLK_Z: return Input_Key_Z;
		case SDLK_0: return Input_Key_0;
		case SDLK_1: return Input_Key_1;
		case SDLK_2: return Input_Key_2;
		case SDLK_3: return Input_Key_3;
		case SDLK_4: return Input_Key_4;
		case SDLK_5: return Input_Key_5;
		case SDLK_6: return Input_Key_6;
		case SDLK_7: return Input_Key_7;
		case SDLK_8: return Input_Key_8;
		case SDLK_9: return Input_Key_9;
		case SDLK_SPACE: return Input_Key_Space;
		case SDLK_LCTRL: return Input_Key_LCtrl;
		case SDLK_RCTRL: return Input_Key_RCtrl;
		case SDLK_LALT: return Input_Key_LAlt;
		case SDLK_RALT: return Input_Key_RAlt;
		case SDLK_LSHIFT: return Input_Key_LShift;
		case SDLK_RSHIFT: return Input_Key_RShift;
		case SDLK_RETURN: return Input_Key_Enter;
		case SDLK_TAB: return Input_Key_Tab;
		case SDLK_UP: return Input_Key_Up;
		case SDLK_DOWN: return Input_Key_Down;
		case SDLK_LEFT: return Input_Key_Left;
		case SDLK_RIGHT: return Input_Key_Right;
		case SDLK_BACKSPACE: return Input_Key_Backspace;
		case SDLK_DELETE: return Input_Key_Delete;
		default: return Input_Key_Unknown;
	}
}

sl_inline bool input_event_from_sdl_event(SDL_Window* window, const SDL_Event* event, Input_Event* out_event) {
	switch (event->type) {
		case SDL_EVENT_MOUSE_MOTION: {
			// Handled via touch
			if (event->motion.which == SDL_TOUCH_MOUSEID) {
				return false;
			}

			*out_event = (Input_Event) {
				.kind = Input_Event_Kind_Touch_Changed,
				.touch = input_touch_event_from_sdl_mouse_motion_event(window, &event->motion),
			};
			return true;
		} break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN: {
			// Handled via touch
			if (event->button.which == SDL_TOUCH_MOUSEID) {
				return false;
			}

			*out_event = (Input_Event) {
				.kind = Input_Event_Kind_Touch_Began,
				.touch = input_touch_event_from_sdl_mouse_button_event(window, &event->button),
			};
			return true;
		} break;

		case SDL_EVENT_MOUSE_BUTTON_UP: {
			// Handled via touch
			if (event->button.which == SDL_TOUCH_MOUSEID) {
				return false;
			}

			*out_event = (Input_Event) {
				.kind = Input_Event_Kind_Touch_Ended,
				.touch = input_touch_event_from_sdl_mouse_button_event(window, &event->button),
			};
			return true;
		} break;

		case SDL_EVENT_FINGER_DOWN: {
			*out_event = (Input_Event) {
				.kind = Input_Event_Kind_Touch_Began,
				.touch = input_touch_event_from_sdl_touch_event(window, &event->tfinger),
			};
			return true;
		} break;

		case SDL_EVENT_FINGER_UP: {
			*out_event = (Input_Event) {
				.kind = Input_Event_Kind_Touch_Ended,
				.touch = input_touch_event_from_sdl_touch_event(window, &event->tfinger),
			};
			return true;
		} break;

		case SDL_EVENT_FINGER_MOTION: {
			*out_event = (Input_Event) {
				.kind = Input_Event_Kind_Touch_Changed,
				.touch = input_touch_event_from_sdl_touch_event(window, &event->tfinger),
			};
			return true;
		} break;

		case SDL_EVENT_FINGER_CANCELED: {
			*out_event = (Input_Event) {
				.kind = Input_Event_Kind_Touch_Cancelled,
				.touch = input_touch_event_from_sdl_touch_event(window, &event->tfinger),
			};
			return true;
		} break;

		case SDL_EVENT_KEY_DOWN: {
			const Input_Key key = input_key_from_sdl_keycode(event->key.key);
			if (key == Input_Key_Unknown) {
				return false;
			}

			*out_event = (Input_Event) {
				.kind = Input_Event_Kind_Key_Down,
				.key = key,
			};
			return true;
		} break;

		case SDL_EVENT_KEY_UP: {
			const Input_Key key = input_key_from_sdl_keycode(event->key.key);
			if (key == Input_Key_Unknown) {
				return false;
			}

			*out_event = (Input_Event) {
				.kind = Input_Event_Kind_Key_Up,
				.key = key,
			};
			return true;
		} break;

		default: {
			// Unhandled
		} break;
	}

	return false;
}
