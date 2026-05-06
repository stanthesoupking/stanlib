#pragma once

#include "gpu3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>

sl_inline Gpu_Surface gpu_surface_for_sdl_window(SDL_Window* window) {
	SDL_PropertiesID props = SDL_GetWindowProperties(window);

	void* wayland_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
	void* wayland_surface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
	if ((wayland_display != NULL) && (wayland_surface != NULL)) {
		return (Gpu_Surface) {
			.kind = Gpu_Surface_Kind_Wayland,
			.wayland = {
				.display = wayland_display,
				.surface = wayland_surface,
			},
		};
	}

	void* x11_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
	u64 x11_window = (u64)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
	if ((x11_display != NULL)) {
		return (Gpu_Surface) {
			.kind = Gpu_Surface_Kind_X11,
			.x11 = {
				.display = x11_display,
				.window = x11_window,
			},
		};
	}

	SDL_MetalView metal_view = SDL_Metal_CreateView(window);
	if (metal_view != NULL) {
		return (Gpu_Surface) {
			.kind = Gpu_Surface_Kind_Metal_Layer,
			.metal_layer = {
				.metal_layer = SDL_Metal_GetLayer(metal_view),
			},
		};
	}

	void* win32_hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	void* win32_instance = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, NULL);
	if ((win32_hwnd != NULL) && (win32_instance != NULL)) {
		return (Gpu_Surface) {
			.kind = Gpu_Surface_Kind_Win32,
			.win32 = {
				.hwnd = win32_hwnd,
				.instance = win32_instance,
			},
		};
	}

	return (Gpu_Surface) {
		.kind = Gpu_Surface_Kind_None,
	};
}
