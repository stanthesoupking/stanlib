#pragma once

#include <stanlib/core.h>

typedef struct SL_Image {
	Allocator* allocator;
	Mutable_Buffer buffer;
	vec2_u32 size;
	u32 row_length;
} SL_Image;

SL_Image* sl_image_new(Allocator* allocator, vec2_u32 size, u32 row_length);
void sl_image_destroy(SL_Image* image);

SL_Image* sl_image_load(Allocator* allocator, const char* path);

void sl_image_blit(SL_Image* dst, SL_Image* src, vec2_u32 offset);
