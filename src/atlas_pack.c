#include <stanlib/atlas_pack.h>

typedef struct SL_Atlas_Entry {
	SL_Image* image; // unowned
	Rect_u32* out_atlas_rect;
} SL_Atlas_Entry;
sl_seq(SL_Atlas_Entry, SL_Atlas_Entry_Seq, sl_atlas_entry_seq);

typedef struct SL_Atlas_Packer {
	Allocator* allocator;
	SL_Atlas_Packer_Flags flags;
	SL_Atlas_Entry_Seq entries;
	
	bool finalised;
	vec2_u32 required_size;
} SL_Atlas_Packer;

SL_Atlas_Packer* sl_atlas_packer_new(Allocator* allocator, SL_Atlas_Packer_Flags flags) {
	SL_Atlas_Packer* packer;
	allocator_new(allocator, packer, 1);
	*packer = (SL_Atlas_Packer) {
		.allocator = allocator,
		.flags = flags,
		.entries = sl_atlas_entry_seq_new(allocator, 0),
	};
	return packer;
}
void sl_atlas_packer_destroy(SL_Atlas_Packer* packer) {
	Allocator* allocator = packer->allocator;
	sl_atlas_entry_seq_destroy(&packer->entries);
	allocator_free(allocator, packer, 1);
}

void sl_atlas_packer_add(SL_Atlas_Packer* packer, SL_Image* unowned_image, Rect_u32* out_atlas_rect) {
	*out_atlas_rect = (Rect_u32) {0};
	
	sl_atlas_entry_seq_push(&packer->entries, (SL_Atlas_Entry) {
		.image = unowned_image,
		.out_atlas_rect = out_atlas_rect,
	});
}

bool sl_atlas_packer_finalise(SL_Atlas_Packer* packer, u32 max_width, vec2_u32* out_required_size) {
	sl_debug_assert(!packer->finalised, "Can only finalise once.");
	
	const u32 gutter = ((packer->flags & SL_Atlas_Packer_Flags_Gutter) > 0) ? 1 : 0;
	
	u32 row_max_x = 0;
	u32 row_x = 0;
	u32 row_start_y = 0;
	u32 row_end_y = row_start_y + 1;
	
	if ((packer->flags & SL_Atlas_Packer_Flags_White_Pixel) > 0) {
		row_x = 1;
		row_max_x = 1;
	}
	
	const u64 entry_count = sl_atlas_entry_seq_get_count(&packer->entries);
	for (u64 i = 0; i < entry_count; i++) {
		SL_Atlas_Entry* entry = sl_atlas_entry_seq_get_ptr(&packer->entries, i);
		const vec2_u32 entry_size = entry->image->size;
		if (entry_size.x + (gutter * 2) > max_width) {
			return false;
		}
		
		if (row_x + entry_size.x + gutter > max_width - gutter) {
			row_x = 0;
			row_start_y = row_end_y;
			row_end_y = row_start_y + 1;
		}
		
		*entry->out_atlas_rect = (Rect_u32) {
			.start = { row_x + gutter, row_start_y + gutter },
			.end = { row_x + gutter + entry_size.x, row_start_y + gutter + entry_size.y },
		};
		
		row_x += entry_size.x + gutter;
		row_max_x = sl_max(row_max_x, row_x);
		row_end_y = sl_max(row_end_y, row_start_y + gutter + entry_size.y + gutter);
	}
	
	packer->required_size = (vec2_u32) { row_max_x + 1, row_end_y };
	
	packer->finalised = true;
	*out_required_size = packer->required_size;
	return true;
}

vec2_u32 sl_atlas_packer_get_required_size(SL_Atlas_Packer* packer) {
	sl_debug_assert(packer->finalised, "Must be finalised to get required size.");
	return packer->required_size;
}

void sl_atlas_packer_write(SL_Atlas_Packer* packer, SL_Image* image) {
	sl_debug_assert(packer->finalised, "Must be finalised to write.");
	
	if ((packer->flags & SL_Atlas_Packer_Flags_White_Pixel) > 0) {
		u8* pixel = image->buffer.data;
		pixel[0] = 255;
		pixel[1] = 255;
		pixel[2] = 255;
		pixel[3] = 255;
	}
	
	const u64 entry_count = sl_atlas_entry_seq_get_count(&packer->entries);
	for (u64 i = 0; i < entry_count; i++) {
		SL_Atlas_Entry* entry = sl_atlas_entry_seq_get_ptr(&packer->entries, i);
		sl_image_blit(image, entry->image, entry->out_atlas_rect->start);
	}
}
