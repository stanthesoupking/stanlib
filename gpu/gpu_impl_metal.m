#include "core.h"
#include "gpu.h"

#include "gpu_impl_common.c"

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

sl_seq(Gpu_Texture, Gpu_Texture_Seq, gpu_texture_seq);

typedef struct Gpu_Semaphore_Data {
	u32 generation;
	id<MTLSharedEvent> event;
	MTLSharedEventListener* listener;
} Gpu_Semaphore_Data;
sl_threadsafe_pool(Gpu_Semaphore_Data, Gpu_Semaphore_Pool, gpu_semaphore_pool);

typedef enum Gpu_Command_Kind {
	Gpu_Command_Kind_Transition_Texture_Layouts,
	Gpu_Command_Kind_Begin_Render,
	Gpu_Command_Kind_End_Render,
	Gpu_Command_Kind_Draw,
	Gpu_Command_Kind_Dispatch,
	Gpu_Command_Kind_Copy_Texture,
	Gpu_Command_Kind_Copy_Slice,
	Gpu_Command_Kind_Copy_Slice_To_Texture,
	Gpu_Command_Kind_Barrier,
	Gpu_Command_Kind_Wait,
	Gpu_Command_Kind_Signal,
} Gpu_Command_Kind;

typedef struct Gpu_Command_Begin_Render {
	Gpu_Render_Pass_Layout layout;
	Gpu_Render_Pass_Values values;
} Gpu_Command_Begin_Render;

typedef struct Gpu_Command_Draw {
	Gpu_Draw_Desc desc;
} Gpu_Command_Draw;

typedef struct Gpu_Command_Dispatch {
	Gpu_Compute_Pipeline pipeline;
	const Gpu_Binding* bindings;
	u32 binding_count;
	vec3_u32 group_count;
} Gpu_Command_Dispatch;

typedef struct Gpu_Command_Transition_Texture_Layouts {
	const Gpu_Texture* textures;
	const Gpu_Texture_Layout* layouts;
	u32 count;
} Gpu_Command_Transition_Texture_Layouts;

typedef struct Gpu_Command_Copy_Texture {
	Gpu_Copy_Texture_Desc desc;
} Gpu_Command_Copy_Texture;

typedef struct Gpu_Command_Copy_Slice {
	Gpu_Slice src;
	Gpu_Slice dst;
} Gpu_Command_Copy_Slice;

typedef struct Gpu_Command_Copy_Slice_To_Texture {
	Gpu_Copy_Slice_To_Texture_Desc desc;
} Gpu_Command_Copy_Slice_To_Texture;

typedef struct Gpu_Command_Wait {
	Gpu_Semaphore semaphore;
	u64 value;
} Gpu_Command_Wait;

typedef struct Gpu_Command_Signal {
	Gpu_Semaphore semaphore;
	u64 value;
} Gpu_Command_Signal;

typedef struct Gpu_Command {
	Gpu_Command_Kind kind;

	union {
		Gpu_Command_Transition_Texture_Layouts* transition_texture_layouts;
		Gpu_Command_Begin_Render* begin_render;
		Gpu_Command_Draw* draw;
		Gpu_Command_Dispatch* dispatch;
		Gpu_Command_Copy_Texture* copy_texture;
		Gpu_Command_Copy_Slice* copy_slice;
		Gpu_Command_Copy_Slice_To_Texture* copy_slice_to_texture;
		Gpu_Command_Wait* wait;
		Gpu_Command_Signal* signal;
	} data;
} Gpu_Command;
sl_seq(Gpu_Command, Gpu_Command_Seq, gpu_command_seq);

typedef struct Gpu_Swapchain_Present {
	void* drawable; // id<CAMetalDrawable>
} Gpu_Swapchain_Present;
sl_seq(Gpu_Swapchain_Present, Gpu_Swapchain_Present_Seq, gpu_swapchain_present_seq);

typedef struct Gpu_Command_Buffer_Data {
	Gpu_Command_Buffer_State state;
	Gpu_Swapchain_Present_Seq swapchain_presents;
	Gpu_Callback_Seq on_complete_callbacks;
	Gpu_Texture_Seq cleanup_textures;
	SL_Fence completion_fence;
	SL_Arena_Allocator* arena;
	Gpu_Command_Seq commands;
} Gpu_Command_Buffer_Data;

typedef struct Gpu_Command_Buffer_Pool_Data {
	u32 generation;
	
	u32 next_command_buffer;
	
	Gpu_Command_Buffer_Data* command_buffers;
	u32 command_buffer_count;
} Gpu_Command_Buffer_Pool_Data;
sl_threadsafe_pool(Gpu_Command_Buffer_Pool_Data, Gpu_Command_Buffer_Pool_Pool, gpu_command_buffer_pool_pool);

typedef struct Gpu_Texture_Data {
	u32 generation;
	id<MTLTexture> texture;
	Gpu_Texture_Desc desc;
} Gpu_Texture_Data;
sl_threadsafe_pool(Gpu_Texture_Data, Gpu_Texture_Pool, gpu_texture_pool);

typedef struct Gpu_Swapchain_Data {
	u32 generation;
	SL_Mutex mutex;
	CAMetalLayer* metal_layer;
} Gpu_Swapchain_Data;
sl_threadsafe_pool(Gpu_Swapchain_Data, Gpu_Swapchain_Pool, gpu_swapchain_pool);

// Metal Conversions
sl_inline MTLPixelFormat gpu_format_to_mtl_pixel_format(Gpu_Format format) {
	switch (format) {
		case Gpu_Format_RGBA8_Unorm: return MTLPixelFormatRGBA8Unorm;
		case Gpu_Format_RGBA8_sRGB: return MTLPixelFormatRGBA8Unorm_sRGB;
		case Gpu_Format_BGRA8_Unorm: return MTLPixelFormatBGRA8Unorm;
		case Gpu_Format_BGRA8_sRGB: return MTLPixelFormatBGRA8Unorm_sRGB;
		case Gpu_Format_RGBA16_Float: return MTLPixelFormatRGBA16Float;
		case Gpu_Format_RGBA32_Float: return MTLPixelFormatRGBA32Float;
	}
}

typedef struct Gpu {
	Allocator* allocator;
	id<MTLDevice> device;
	id<MTLCommandQueue> queue;
	id<MTLFence> fence;
	
	Gpu_Semaphore_Pool semaphore_pool;
	Gpu_Swapchain_Pool swapchain_pool;
	Gpu_Texture_Pool texture_pool;
	Gpu_Command_Buffer_Pool_Pool command_pool_pool;
	
	Gpu_Semaphore global_semaphore;
} Gpu;

Gpu gpu = {0};

void gpu_init(const Gpu_Desc* desc) {
	gpu.allocator = desc->allocator;
	
	gpu.device = MTLCreateSystemDefaultDevice();
	gpu.queue = [gpu.device newCommandQueue];
	gpu.fence = [gpu.device newFence];
	gpu.texture_pool = gpu_texture_pool_new(gpu.allocator);
	gpu.semaphore_pool = gpu_semaphore_pool_new(gpu.allocator);
	gpu.swapchain_pool = gpu_swapchain_pool_new(gpu.allocator);
	gpu.command_pool_pool = gpu_command_buffer_pool_pool_new(gpu.allocator);
	gpu.global_semaphore = gpu_new_semaphore();
}

void gpu_init_command_buffer(Gpu_Command_Buffer_Data* command_buffer) {
	*command_buffer = (Gpu_Command_Buffer_Data) {
		.state = Gpu_Command_Buffer_State_Idle,
		.arena = sl_arena_allocator_new(gpu.allocator, 256 << 10),
		.commands = gpu_command_seq_new(gpu.allocator, 8),
		.swapchain_presents = gpu_swapchain_present_seq_new(gpu.allocator, 1),
		.cleanup_textures = gpu_texture_seq_new(gpu.allocator, 1),
		.on_complete_callbacks = gpu_callback_seq_new(gpu.allocator, 1),
		.completion_fence = sl_fence_new(true),
	};
}
void gpu_deinit_command_buffer(Gpu_Command_Buffer_Data* command_buffer) {
	sl_arena_allocator_destroy(command_buffer->arena);
	gpu_command_seq_destroy(&command_buffer->commands);
	gpu_swapchain_present_seq_destroy(&command_buffer->swapchain_presents);
	gpu_texture_seq_destroy(&command_buffer->cleanup_textures);
	gpu_callback_seq_destroy(&command_buffer->on_complete_callbacks);
	sl_fence_destroy(&command_buffer->completion_fence);
	*command_buffer = (Gpu_Command_Buffer_Data) {0};
}

// Command Pool
Gpu_Command_Buffer_Pool gpu_new_command_buffer_pool(u32 size) {
	Gpu_Command_Buffer_Pool pool = gpu_command_buffer_pool_pool_acquire(&gpu.command_pool_pool);
	Gpu_Command_Buffer_Pool_Data* data = gpu_command_buffer_pool_pool_resolve(&gpu.command_pool_pool, pool);
	
	data->command_buffer_count = size;
	allocator_new(gpu.allocator, data->command_buffers, size);
	for (u32 i = 0; i < size; i++) {
		gpu_init_command_buffer(&data->command_buffers[i]);
	}
	
	return pool;
}
void gpu_destroy_command_buffer_pool(Gpu_Command_Buffer_Pool pool) {
	Gpu_Command_Buffer_Pool_Data* data = gpu_command_buffer_pool_pool_resolve(&gpu.command_pool_pool, pool);
	for (u32 i = 0; i < data->command_buffer_count; i++) {
		gpu_deinit_command_buffer(&data->command_buffers[i]);
	}
	allocator_free(gpu.allocator, data->command_buffers, data->command_buffer_count);
	gpu_command_buffer_pool_pool_release(&gpu.command_pool_pool, pool);
}
Gpu_Command_Buffer_Data* gpu_resolve_command_buffer_data(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Pool_Data* pool_data = gpu_command_buffer_pool_pool_resolve(&gpu.command_pool_pool, cb.pool);
	if (pool_data == NULL) {
		return NULL;
	}
	if (cb.index > pool_data->command_buffer_count) {
		return NULL;
	}
	return &pool_data->command_buffers[cb.index];
}

// Command Buffer
bool gpu_new_command_buffer(Gpu_Command_Buffer_Pool pool, Gpu_Command_Buffer* out_cb) {
	Gpu_Command_Buffer_Pool_Data* pool_data = gpu_command_buffer_pool_pool_resolve(&gpu.command_pool_pool, pool);

	Gpu_Command_Buffer cb_handle = {
		.pool = pool,
		.index = pool_data->next_command_buffer,
	};

	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb_handle);
	if (cb_data->state == Gpu_Command_Buffer_State_Recording) {
		// Command pool exceeded.
		return false;
	}

	sl_fence_wait(&cb_data->completion_fence);
	cb_data->state = Gpu_Command_Buffer_State_Recording;
	sl_arena_allocator_reset(cb_data->arena, 0);
	gpu_command_seq_clear(&cb_data->commands);
	gpu_callback_seq_clear(&cb_data->on_complete_callbacks);

	pool_data->next_command_buffer = (pool_data->next_command_buffer + 1) % pool_data->command_buffer_count;

	*out_cb = cb_handle;
	return true;
}
void gpu_cleanup_command_buffer(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	
	const u64 cleanup_count = gpu_texture_seq_get_count(&cb_data->cleanup_textures);
	for (u64 i = 0; i < cleanup_count; i++) {
		Gpu_Texture texture = gpu_texture_seq_get(&cb_data->cleanup_textures, i);
		gpu_destroy_texture(texture);
	}
	gpu_texture_seq_clear(&cb_data->cleanup_textures);
	
	sl_fence_signal(&cb_data->completion_fence);
}
void gpu_enqueue(Gpu_Command_Buffer cb, bool wait_until_completed) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");
	cb_data->state = Gpu_Command_Buffer_State_Enqueued;
	
	id<MTLCommandBuffer> mtl_cb = [gpu.queue commandBufferWithUnretainedReferences];
	
	const u64 command_count = gpu_command_seq_get_count(&cb_data->commands);
	for (u64 command_idx = 0; command_idx < command_count; command_idx++) {
		const Gpu_Command command = gpu_command_seq_get(&cb_data->commands, command_idx);

		switch (command.kind) {
			case Gpu_Command_Kind_Transition_Texture_Layouts: {
				// no-op
			} break;

			case Gpu_Command_Kind_Begin_Render: {
				
			} break;

			case Gpu_Command_Kind_End_Render: {
				
			} break;

			case Gpu_Command_Kind_Draw: {
				
			} break;

			case Gpu_Command_Kind_Dispatch: {
				
			} break;

			case Gpu_Command_Kind_Copy_Texture: {
				
			} break;

			case Gpu_Command_Kind_Copy_Slice: {
			
			} break;

			case Gpu_Command_Kind_Copy_Slice_To_Texture: {

			} break;

			case Gpu_Command_Kind_Barrier: {
				
			} break;

			case Gpu_Command_Kind_Wait: {
				Gpu_Command_Wait* wait = command.data.wait;
				Gpu_Semaphore_Data* semaphore_data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, wait->semaphore);
				[mtl_cb encodeWaitForEvent:semaphore_data->event value:wait->value];
			} break;

			case Gpu_Command_Kind_Signal: {
				Gpu_Command_Signal* signal = command.data.signal;
				Gpu_Semaphore_Data* semaphore_data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, signal->semaphore);
				[mtl_cb encodeSignalEvent:semaphore_data->event value:signal->value];
			} break;
		}
	}
	
	const u64 present_count = gpu_swapchain_present_seq_get_count(&cb_data->swapchain_presents);
	for (u64 present_idx = 0; present_idx < present_count; present_idx++) {
		const Gpu_Swapchain_Present present = gpu_swapchain_present_seq_get(&cb_data->swapchain_presents, present_idx);
		id<CAMetalDrawable> drawable = (__bridge_transfer id<CAMetalDrawable>)present.drawable;
		[drawable present];
	}
	gpu_swapchain_present_seq_clear(&cb_data->swapchain_presents);
	
	[mtl_cb addCompletedHandler:^(id<MTLCommandBuffer> mtl_cb1) {
		gpu_cleanup_command_buffer(cb);
	}];

	[mtl_cb commit];
	
	if (wait_until_completed) {
		[mtl_cb waitUntilCompleted];
	}
}

// Swapchain
Gpu_Swapchain gpu_new_swapchain(Gpu_Surface surface) {
	Gpu_Swapchain swapchain = gpu_swapchain_pool_acquire(&gpu.swapchain_pool);
	Gpu_Swapchain_Data* swapchain_data = gpu_swapchain_pool_resolve(&gpu.swapchain_pool, swapchain);
	CAMetalLayer* metal_layer = (__bridge CAMetalLayer*)surface.metal_layer.metal_layer;
	metal_layer.device = gpu.device;
	swapchain_data->metal_layer = metal_layer;
	swapchain_data->mutex = sl_mutex_new();
	
	return swapchain;
}
void gpu_destroy_swapchain(Gpu_Swapchain swapchain) {
	Gpu_Swapchain_Data* swapchain_data = gpu_swapchain_pool_resolve(&gpu.swapchain_pool, swapchain);
	swapchain_data->metal_layer = nil;
	sl_mutex_destroy(&swapchain_data->mutex);
	gpu_swapchain_pool_release(&gpu.swapchain_pool, swapchain);
}
bool gpu_fetch_swapchain_texture(Gpu_Swapchain swapchain, Gpu_Command_Buffer cb, Gpu_Swapchain_Desc swapchain_desc, u64 timeout, Gpu_Texture* out_texture) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");
	
	Gpu_Swapchain_Data* swapchain_data = gpu_swapchain_pool_resolve(&gpu.swapchain_pool, swapchain);
	
	sl_mutex_lock(&swapchain_data->mutex);
	swapchain_data->metal_layer.drawableSize = (CGSize) { swapchain_desc.size.x, swapchain_desc.size.y };
	id<CAMetalDrawable> drawable = [swapchain_data->metal_layer nextDrawable];
	sl_mutex_unlock(&swapchain_data->mutex);
	
	if (drawable == nil) {
		return false;
	}
	
	Gpu_Texture texture = gpu_texture_pool_acquire(&gpu.texture_pool);
	Gpu_Texture_Data* texture_data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
	texture_data->texture = drawable.texture;
	gpu_texture_seq_push(&cb_data->cleanup_textures, texture);
	
	const Gpu_Swapchain_Present present = {
		.drawable = (__bridge_retained void*)drawable,
	};
	gpu_swapchain_present_seq_push(&cb_data->swapchain_presents, present);
	
	return true;
}

// Texture
MTLTextureDescriptor* gpu_texture_desc_to_mtl_texture_descriptor(const Gpu_Texture_Desc* desc) {
	MTLTextureDescriptor* descriptor = [MTLTextureDescriptor new];
	
	descriptor.width = desc->size.x;
	descriptor.height = desc->size.y;
	descriptor.depth = desc->size.z;
	descriptor.mipmapLevelCount = desc->mip_levels;
	descriptor.arrayLength = desc->array_layers;
	descriptor.pixelFormat = gpu_format_to_mtl_pixel_format(desc->format);
	
	switch (desc->kind) {
		case Gpu_Texture_Kind_1D: {
			descriptor.textureType = MTLTextureType1D;
		} break;
			
		case Gpu_Texture_Kind_2D: {
			descriptor.textureType = MTLTextureType2D;
		} break;
			
		case Gpu_Texture_Kind_3D: {
			descriptor.textureType = MTLTextureType3D;
		} break;
	}
	
	MTLTextureUsage usage = 0;
	if ((desc->usage & Gpu_Texture_Usage_Shader_Read) > 0) {
		usage |= MTLTextureUsageShaderRead;
	}
	if ((desc->usage & Gpu_Texture_Usage_Shader_Write) > 0) {
		usage |= MTLTextureUsageShaderWrite;
	}
	if ((desc->usage & Gpu_Texture_Usage_Render_Attachment) > 0) {
		usage |= MTLTextureUsageRenderTarget;
	}
	descriptor.usage = usage;
	
	return descriptor;
}
Gpu_Size_And_Align gpu_size_and_align_for_texture(const Gpu_Texture_Desc* desc) {
	MTLTextureDescriptor* mtl_desc = gpu_texture_desc_to_mtl_texture_descriptor(desc);
	MTLSizeAndAlign mtl_size_and_align = [gpu.device heapTextureSizeAndAlignWithDescriptor:mtl_desc];
	return (Gpu_Size_And_Align) {
		.size = mtl_size_and_align.size,
		.align = mtl_size_and_align.align,
	};
}
Gpu_Texture gpu_new_texture(const Gpu_Texture_Desc* desc, Gpu_Slice slice) {
	return SL_HANDLE_NULL; // todo
}
void gpu_destroy_texture(Gpu_Texture texture) {
	Gpu_Texture_Data* data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
	data->texture = nil;
	gpu_texture_pool_release(&gpu.texture_pool, texture);
}
vec3_u32 gpu_get_texture_size(Gpu_Texture texture) {
	Gpu_Texture_Data* data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
	return data->desc.size;
}

// Semaphore
Gpu_Semaphore gpu_new_semaphore(void) {
	Gpu_Semaphore result = gpu_semaphore_pool_acquire(&gpu.semaphore_pool);
	Gpu_Semaphore_Data* data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, result);
	data->event = [gpu.device newSharedEvent];
	data->listener = [[MTLSharedEventListener alloc] init];
	return result;
}
void gpu_destroy_semaphore(Gpu_Semaphore semaphore) {
	Gpu_Semaphore_Data* data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, semaphore);
	data->event = nil;
	data->listener = nil;
	gpu_semaphore_pool_release(&gpu.semaphore_pool, semaphore);
}
void gpu_notify(Gpu_Semaphore semaphore, u64 value, void* ctx, Gpu_Callback_Fn fn) {
	Gpu_Semaphore_Data* data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, semaphore);
	[data->event notifyListener:data->listener atValue:value block:^(id<MTLSharedEvent> event0, uint64_t value0) {
		fn(ctx);
	}];
}
void gpu_wait_gpu(Gpu_Command_Buffer cb, Gpu_Semaphore semaphore, u64 value) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Command_Wait* wait;
	allocator_new(&cb_data->arena->allocator, wait, 1);
	*wait = (Gpu_Command_Wait) {
		.semaphore = semaphore,
		.value = value,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Wait,
		.data = {
			.wait = wait,
		},
	});
}
void gpu_signal_gpu(Gpu_Command_Buffer cb, Gpu_Semaphore semaphore, u64 value) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Command_Signal* signal;
	allocator_new(&cb_data->arena->allocator, signal, 1);
	*signal = (Gpu_Command_Signal) {
		.semaphore = semaphore,
		.value = value,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Signal,
		.data = {
			.signal = signal,
		},
	});
}
void gpu_wait_cpu(Gpu_Semaphore semaphore, u64 value) {
	Gpu_Semaphore_Data* data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, semaphore);
	[data->event waitUntilSignaledValue:value timeoutMS:u64_max];
}
void gpu_signal_cpu(Gpu_Semaphore semaphore, u64 value) {
	Gpu_Semaphore_Data* data = gpu_semaphore_pool_resolve(&gpu.semaphore_pool, semaphore);
	[data->event setSignaledValue:value];
}
