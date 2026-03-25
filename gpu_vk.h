#pragma once

typedef struct Gpu_Vk_Desc {
	Allocator* allocator;
	u32 required_extension_count;
	char const* const* required_extensions;
} Gpu_Vk_Desc;

void gpu_vk_init(const Gpu_Vk_Desc* desc);
void gpu_vk_deinit();
