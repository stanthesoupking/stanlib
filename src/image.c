#include <stanlib/image.h>
#include <string.h>

#include "vendor/stb_image.h"

SL_Image* sl_image_new(Allocator* allocator, vec2_u32 size, u32 row_length) {
	SL_Image* image;
	allocator_new(allocator, image, 1);

	const u64 buffer_size = row_length * size.y;
	u8* buffer_data;
	allocator_new(allocator, buffer_data, buffer_size);
	memset(buffer_data, 0, buffer_size);

	*image = (SL_Image) {
		.allocator = allocator,
		.buffer = {
			.data = buffer_data,
			.size = buffer_size,
		},
		.size = size,
		.row_length = row_length,
	};

	return image;
}
void sl_image_destroy(SL_Image* image) {
	Allocator* allocator = image->allocator;
	allocator_free(allocator, image->buffer.data, image->buffer.size);
	allocator_free(allocator, image, 1);
}

SL_Image* sl_image_load(Allocator* allocator, const char* path) {
	int img_w, img_h, img_channels;
	u8* img_bytes = stbi_load(path, &img_w, &img_h, &img_channels, 4);
	if (img_bytes == NULL) {
		return NULL;
	}

	SL_Image* result = sl_image_new(allocator, (vec2_u32) { (u32)img_w, (u32)img_h }, (u32)img_w * 4);
	memcpy(result->buffer.data, img_bytes, (u64)img_w * (u64)img_h * (u64)4);
	stbi_image_free(img_bytes);

	return result;
}

void sl_image_blit(SL_Image* dst, SL_Image* src, vec2_u32 offset) {
	sl_assert(((src->size.x + offset.x) <= dst->size.x) && ((src->size.y + offset.y) <= dst->size.y), "Destination size exceeded.");

	u8* dst_pixels = dst->buffer.data;
	const u8* src_pixels = src->buffer.data;
	for (u32 src_y = 0; src_y < src->size.y; src_y++) {
		const u32 dst_row_offset = ((src_y + offset.y) * dst->row_length) + (offset.x * 4);
		const u32 src_row_offset = src_y * src->row_length;
		memcpy(dst_pixels + dst_row_offset, src_pixels + src_row_offset, src->row_length);
	}
}
