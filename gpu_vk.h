#pragma once

#include <vulkan/vulkan.h>

typedef VkSurfaceKHR (*Gpu_Vk_Get_Surface_Fn)(void* ctx, VkInstance instance);
typedef VkExtent2D (*Gpu_Vk_Get_Swapchain_Extent_Fn)(void* ctx);

typedef struct Gpu_Vk_Swapchain_Desc {
	void* ctx;
	Gpu_Vk_Get_Surface_Fn get_surface_fn;
	Gpu_Vk_Get_Swapchain_Extent_Fn get_swapchain_extent_fn;
} Gpu_Vk_Swapchain_Desc;

typedef struct Gpu_Vk_Desc {
	Allocator* allocator;

	u32 required_extension_count;
	char const* const* required_extensions;

	Gpu_Vk_Swapchain_Desc swapchain_desc;
} Gpu_Vk_Desc;

void gpu_vk_init(const Gpu_Vk_Desc* desc);
void gpu_vk_deinit();

void gpu_vk_dummy_render();