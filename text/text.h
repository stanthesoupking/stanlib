#include "../core.h"
#include "../gpu/gpu.h"

typedef struct SL_Font_Atlas SL_Font_Atlas;

typedef struct SL_Font_Atlas_Desc {
	Allocator* allocator;
	Immutable_Buffer font;
	f32 font_size;
	u32 atlas_size;
	Gpu_Slice* atlas_allocator;
	Gpu_Slice* staging_allocator;
	Gpu_Command_Buffer command_buffer;
} SL_Font_Atlas_Desc;
SL_Font_Atlas* sl_new_font_atlas(const SL_Font_Atlas_Desc* desc);
void sl_destroy_font_atlas(SL_Font_Atlas* atlas);

Gpu_Texture sl_get_font_atlas_texture(SL_Font_Atlas* atlas);
