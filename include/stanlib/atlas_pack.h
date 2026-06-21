#pragma once

#include <stanlib/core.h>
#include <stanlib/image.h>

typedef struct SL_Atlas_Packer SL_Atlas_Packer;

typedef enum SL_Atlas_Packer_Flags {
	SL_Atlas_Packer_Flags_None = 0,
	
	// Each entry in the atlas gets a 1 pixel gutter of clear pixels.
	SL_Atlas_Packer_Flags_Gutter = 1 << 0,
	
	// Write an opaque white pixel to 0,0 corner of the atlas.
	SL_Atlas_Packer_Flags_White_Pixel = 1 << 1,
} SL_Atlas_Packer_Flags;

SL_Atlas_Packer* sl_atlas_packer_new(Allocator* allocator, SL_Atlas_Packer_Flags flags);
void sl_atlas_packer_destroy(SL_Atlas_Packer* packer);

void sl_atlas_packer_add(SL_Atlas_Packer* packer, SL_Image* unowned_image, Rect_u32* out_atlas_rect);

bool sl_atlas_packer_finalise(SL_Atlas_Packer* packer, u32 max_width, vec2_u32* out_required_size);

void sl_atlas_packer_write(SL_Atlas_Packer* packer, SL_Image* image);
