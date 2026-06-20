#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

sl_seq(Gpu_Texture, Gpu_Texture_Seq, gpu_texture_seq);

typedef struct Gpu_Sampler_Data {
	u32 generation;
	id<MTLSamplerState> sampler;
} Gpu_Sampler_Data;
sl_threadsafe_pool(Gpu_Sampler_Data, Gpu_Sampler_Pool, gpu_sampler_pool);

typedef struct Gpu_Heap_Data {
	u32 generation;
	u64 size;
	Gpu_Memory_Type memory_type;
	id<MTLHeap> heap;
	id<MTLBuffer> buffer;
	void* host_ptr;
} Gpu_Heap_Data;
sl_threadsafe_pool(Gpu_Heap_Data, Gpu_Heap_Pool, gpu_heap_pool);

typedef struct Gpu_Shader_Blob_Data {
	u32 generation;
	id<MTLLibrary> library;
} Gpu_Shader_Blob_Data;
sl_threadsafe_pool(Gpu_Shader_Blob_Data, Gpu_Shader_Blob_Pool, gpu_shader_blob_pool);

typedef struct Gpu_Render_Pipeline_Data {
	u32 generation;
	id<MTLRenderPipelineState> pipeline_state;
	Gpu_Primitive_Kind primitive_kind;
	Gpu_Cull_Mode cull_mode;
} Gpu_Render_Pipeline_Data;
sl_threadsafe_pool(Gpu_Render_Pipeline_Data, Gpu_Render_Pipeline_Pool, gpu_render_pipeline_pool);

typedef struct Gpu_Compute_Pipeline_Data {
	u32 generation;
	id<MTLComputePipelineState> pipeline_state;
	vec3_u32 group_size;
} Gpu_Compute_Pipeline_Data;
sl_threadsafe_pool(Gpu_Compute_Pipeline_Data, Gpu_Compute_Pipeline_Pool, gpu_compute_pipeline_pool);

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
		case Gpu_Format_R8_Unorm: return MTLPixelFormatR8Unorm;
		case Gpu_Format_RGBA8_Unorm: return MTLPixelFormatRGBA8Unorm;
		case Gpu_Format_RGBA8_sRGB: return MTLPixelFormatRGBA8Unorm_sRGB;
		case Gpu_Format_BGRA8_Unorm: return MTLPixelFormatBGRA8Unorm;
		case Gpu_Format_BGRA8_sRGB: return MTLPixelFormatBGRA8Unorm_sRGB;
		case Gpu_Format_RGBA16_Float: return MTLPixelFormatRGBA16Float;
		case Gpu_Format_RGBA32_Float: return MTLPixelFormatRGBA32Float;
	}
}
sl_inline Gpu_Format gpu_mtl_pixel_format_to_format(MTLPixelFormat format) {
	switch (format) {
		case MTLPixelFormatR8Unorm: return Gpu_Format_R8_Unorm;
		case MTLPixelFormatRGBA8Unorm: return Gpu_Format_RGBA8_Unorm;
		case MTLPixelFormatRGBA8Unorm_sRGB: return Gpu_Format_RGBA8_sRGB;
		case MTLPixelFormatBGRA8Unorm: return Gpu_Format_BGRA8_Unorm;
		case MTLPixelFormatBGRA8Unorm_sRGB: return Gpu_Format_BGRA8_sRGB;
		case MTLPixelFormatRGBA16Float: return Gpu_Format_RGBA16_Float;
		case MTLPixelFormatRGBA32Float: return Gpu_Format_RGBA32_Float;

		default:
			sl_abort("Unhandled format.");
			return 0;
	}
}
sl_inline MTLLoadAction gpu_load_op_to_mtl_load_action(Gpu_Load_Op op) {
	switch (op) {
		case Gpu_Load_Op_Load: return MTLLoadActionLoad;
		case Gpu_Load_Op_Clear: return MTLLoadActionClear;
		case Gpu_Load_Op_Dont_Care: return MTLLoadActionDontCare;
	}
}
sl_inline MTLStoreAction gpu_store_op_to_mtl_store_action(Gpu_Store_Op op) {
	switch (op) {
		case Gpu_Store_Op_Store: return MTLStoreActionStore;
		case Gpu_Store_Op_Dont_Care: return MTLStoreActionDontCare;
	}
}
sl_inline Gpu_Texture_Kind gpu_mtl_texture_type_to_texture_kind(MTLTextureType kind) {
	switch (kind) {
		case MTLTextureType1D: return Gpu_Texture_Kind_1D;
		case MTLTextureType2D: return Gpu_Texture_Kind_2D;
		case MTLTextureType3D: return Gpu_Texture_Kind_3D;

		default:
			sl_abort("Unhandled type.");
			return 0;
	}
}
sl_inline MTLTextureType gpu_texture_kind_to_mtl_texture_type(Gpu_Texture_Kind kind) {
	switch (kind) {
		case Gpu_Texture_Kind_1D: return MTLTextureType1D;
		case Gpu_Texture_Kind_2D: return MTLTextureType2D;
		case Gpu_Texture_Kind_3D: return MTLTextureType3D;
	}
}
sl_inline MTLTextureUsage gpu_texture_usage_to_mtl_texture_usage(Gpu_Texture_Usage usage) {
	MTLTextureUsage result = 0;
	if ((usage & Gpu_Texture_Usage_Shader_Read) > 0) {
		result |= MTLTextureUsageShaderRead;
	}
	if ((usage & Gpu_Texture_Usage_Shader_Write) > 0) {
		result |= MTLTextureUsageShaderWrite;
	}
	if ((usage & Gpu_Texture_Usage_Render_Attachment) > 0) {
		result |= MTLTextureUsageRenderTarget;
	}
	return result;
}
sl_inline Gpu_Texture_Usage gpu_mtl_texture_usage_to_texture_usage(MTLTextureUsage usage) {
	Gpu_Texture_Usage result = 0;
	if ((usage & MTLTextureUsageShaderRead) > 0) {
		result |= Gpu_Texture_Usage_Shader_Read;
	}
	if ((usage & MTLTextureUsageShaderWrite) > 0) {
		result |= Gpu_Texture_Usage_Shader_Write;
	}
	if ((usage & MTLTextureUsageRenderTarget) > 0) {
		result |= Gpu_Texture_Usage_Render_Attachment;
	}
	return result;
}
sl_inline MTLSamplerMinMagFilter gpu_filter_to_mtl_sampler_min_mag_filter(Gpu_Filter filter) {
	switch (filter) {
		case Gpu_Filter_Nearest: return MTLSamplerMinMagFilterNearest;
		case Gpu_Filter_Linear: return MTLSamplerMinMagFilterLinear;
	}
}
sl_inline MTLSamplerMipFilter gpu_filter_to_mtl_sampler_mip_filter(Gpu_Filter filter) {
	switch (filter) {
		case Gpu_Filter_Nearest: return MTLSamplerMipFilterNearest;
		case Gpu_Filter_Linear: return MTLSamplerMipFilterLinear;
	}
}
sl_inline MTLSamplerAddressMode gpu_sampler_address_mode_to_mtl_sampler_address_mode(Gpu_Sampler_Address_Mode mode) {
	switch (mode) {
		case Gpu_Sampler_Address_Mode_Clamp_To_Edge: return MTLSamplerAddressModeClampToEdge;
		case Gpu_Sampler_Address_Mode_Repeat: return MTLSamplerAddressModeRepeat;
		case Gpu_Sampler_Address_Mode_Mirrored_Repeat: return MTLSamplerAddressModeMirrorRepeat;
	}
}
sl_inline MTLPrimitiveType gpu_primitive_kind_to_mtl_primitive_type(Gpu_Primitive_Kind kind) {
	switch (kind) {
		case Gpu_Primitive_Kind_Triangle: return MTLPrimitiveTypeTriangle;
		case Gpu_Primitive_Kind_Triangle_Strip: return MTLPrimitiveTypeTriangleStrip;
	}
}
sl_inline MTLPrimitiveTopologyClass gpu_primitive_kind_to_mtl_primitive_topology_class(Gpu_Primitive_Kind kind) {
	switch (kind) {
		case Gpu_Primitive_Kind_Triangle:
		case Gpu_Primitive_Kind_Triangle_Strip:
			return MTLPrimitiveTopologyClassTriangle;
	}
}
sl_inline MTLCullMode gpu_cull_mode_to_mtl_cull_mode(Gpu_Cull_Mode mode) {
	switch (mode) {
		case Gpu_Cull_Mode_None: return MTLCullModeNone;
		case Gpu_Cull_Mode_Front: return MTLCullModeFront;
		case Gpu_Cull_Mode_Back: return MTLCullModeBack;
	}
}

typedef struct Gpu {
	Allocator* allocator;
	id<MTLDevice> device;
	id<MTLCommandQueue> queue;
	id<MTLFence> fence;

	Gpu_Sampler_Pool sampler_pool;
	Gpu_Heap_Pool heap_pool;
	Gpu_Shader_Blob_Pool shader_blob_pool;
	Gpu_Render_Pipeline_Pool render_pipeline_pool;
	Gpu_Compute_Pipeline_Pool compute_pipeline_pool;
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
	gpu.sampler_pool = gpu_sampler_pool_new(gpu.allocator);
	gpu.heap_pool = gpu_heap_pool_new(gpu.allocator);
	gpu.shader_blob_pool = gpu_shader_blob_pool_new(gpu.allocator);
	gpu.render_pipeline_pool = gpu_render_pipeline_pool_new(gpu.allocator);
	gpu.compute_pipeline_pool = gpu_compute_pipeline_pool_new(gpu.allocator);
	gpu.texture_pool = gpu_texture_pool_new(gpu.allocator);
	gpu.semaphore_pool = gpu_semaphore_pool_new(gpu.allocator);
	gpu.swapchain_pool = gpu_swapchain_pool_new(gpu.allocator);
	gpu.command_pool_pool = gpu_command_buffer_pool_pool_new(gpu.allocator);
	gpu.global_semaphore = gpu_new_semaphore();

	sl_unused const char* device_name = [gpu.device.name cStringUsingEncoding:NSUTF8StringEncoding];
	gpu_log("Initialised with device '%s', using Metal backend.", device_name);
}

void gpu_deinit(void) {
	// todo
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

	const u64 callback_count = gpu_callback_seq_get_count(&cb_data->on_complete_callbacks);
	for (u64 i = 0; i < callback_count; i++) {
		Gpu_Callback callback = gpu_callback_seq_get(&cb_data->on_complete_callbacks, i);
		callback.fn(callback.ctx);
	}
	gpu_callback_seq_clear(&cb_data->on_complete_callbacks);

	sl_fence_signal(&cb_data->completion_fence);
}

typedef enum Gpu_Encoder_Kind {
	Gpu_Encoder_Kind_None,
	Gpu_Encoder_Kind_Render,
	Gpu_Encoder_Kind_Compute,
	Gpu_Encoder_Kind_Blit
} Gpu_Encoder_Kind;

typedef struct Gpu_Encoder_State {
	Gpu_Encoder_Kind current;
	id<MTLCommandBuffer> command_buffer;
	id<MTLRenderCommandEncoder> render_encoder;
	id<MTLComputeCommandEncoder> compute_encoder;
	id<MTLBlitCommandEncoder> blit_encoder;
} Gpu_Encoder_State;

Gpu_Encoder_State gpu_new_encoder_state(id<MTLCommandBuffer> command_buffer) {
	return (Gpu_Encoder_State) {
		.current = Gpu_Encoder_Kind_None,
		.command_buffer = command_buffer,
	};
}
void gpu_destroy_encoder_state(Gpu_Encoder_State* state) {
	*state = (Gpu_Encoder_State) {0};
}
void gpu_end_current_encoder(Gpu_Encoder_State* state) {
	switch (state->current) {
		case Gpu_Encoder_Kind_None: {
			// Nothing to end.
		} break;

		case Gpu_Encoder_Kind_Render: {
			[state->render_encoder endEncoding];
			state->render_encoder = nil;
		} break;

		case Gpu_Encoder_Kind_Compute: {
			[state->compute_encoder endEncoding];
			state->compute_encoder = nil;
		} break;

		case Gpu_Encoder_Kind_Blit: {
			[state->blit_encoder endEncoding];
			state->blit_encoder = nil;
		} break;
	}
	state->current = Gpu_Encoder_Kind_None;
}
void gpu_start_render_encoder(Gpu_Encoder_State* state, const Gpu_Render_Pass_Layout* layout, const Gpu_Render_Pass_Values* values) {
	gpu_end_current_encoder(state);
	state->current = Gpu_Encoder_Kind_Render;

	MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor new];
	for (u32 i = 0; i < values->attachment_count; i++) {
		const Gpu_Render_Pass_Layout_Attachment* layout_att = &layout->attachments[i];
		const Gpu_Render_Pass_Values_Attachment* values_att = &values->attachments[i];

		const Gpu_Texture_Data* texture_data = gpu_texture_pool_resolve(&gpu.texture_pool, values_att->texture);
		pass_desc.colorAttachments[i].texture = texture_data->texture;
		pass_desc.colorAttachments[i].loadAction = gpu_load_op_to_mtl_load_action(layout_att->load_op);
		pass_desc.colorAttachments[i].storeAction = gpu_store_op_to_mtl_store_action(layout_att->store_op);
		pass_desc.colorAttachments[i].clearColor = MTLClearColorMake(values_att->clear_value.x, values_att->clear_value.y, values_att->clear_value.z, values_att->clear_value.w);
	}

	state->render_encoder = [state->command_buffer renderCommandEncoderWithDescriptor:pass_desc];
	[state->render_encoder waitForFence:gpu.fence beforeStages:(MTLRenderStageVertex | MTLRenderStageFragment)];
	[state->render_encoder updateFence:gpu.fence afterStages:(MTLRenderStageVertex | MTLRenderStageFragment)];

	const Gpu_Texture_Desc* texture_desc = gpu_get_texture_desc(values->attachments[0].texture);

	// flip viewport for consistency with Vulkan.
	const MTLViewport vp = {
		.originX = 0,
		.originY = texture_desc->size.y,
		.width   = texture_desc->size.x,
		.height  = -(s32)texture_desc->size.y,
		.znear   = 0.0,
		.zfar    = 1.0
	};
	[state->render_encoder setViewport:vp];
}
void gpu_start_compute_encoder(Gpu_Encoder_State* state) {
	if (state->current == Gpu_Encoder_Kind_Compute) {
		return;
	}
	gpu_end_current_encoder(state);
	state->current = Gpu_Encoder_Kind_Compute;

	state->compute_encoder = [state->command_buffer computeCommandEncoderWithDispatchType:MTLDispatchTypeConcurrent];
	[state->compute_encoder waitForFence:gpu.fence];
	[state->compute_encoder updateFence:gpu.fence];
}
void gpu_start_blit_encoder(Gpu_Encoder_State* state) {
	if (state->current == Gpu_Encoder_Kind_Blit) {
		return;
	}
	gpu_end_current_encoder(state);
	state->current = Gpu_Encoder_Kind_Blit;

	state->blit_encoder = [state->command_buffer blitCommandEncoder];
	[state->blit_encoder waitForFence:gpu.fence];
	[state->blit_encoder updateFence:gpu.fence];
}

void gpu_enqueue(Gpu_Command_Buffer cb, bool wait_until_completed) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");
	cb_data->state = Gpu_Command_Buffer_State_Enqueued;

	id<MTLCommandBuffer> mtl_cb = [gpu.queue commandBufferWithUnretainedReferences];

	Gpu_Encoder_State encoder_state = gpu_new_encoder_state(mtl_cb);

	const u64 command_count = gpu_command_seq_get_count(&cb_data->commands);
	for (u64 command_idx = 0; command_idx < command_count; command_idx++) {
		const Gpu_Command command = gpu_command_seq_get(&cb_data->commands, command_idx);

		switch (command.kind) {
			case Gpu_Command_Kind_Transition_Texture_Layouts: {
				// no-op
			} break;

			case Gpu_Command_Kind_Begin_Render: {
				const Gpu_Command_Begin_Render* begin_render = command.data.begin_render;
				gpu_start_render_encoder(&encoder_state, &begin_render->layout, &begin_render->values);
			} break;

			case Gpu_Command_Kind_End_Render: {
				sl_assert(encoder_state.current == Gpu_Encoder_Kind_Render, "Not rendering.");
				gpu_end_current_encoder(&encoder_state);
			} break;

			case Gpu_Command_Kind_Draw: {
				sl_assert(encoder_state.current == Gpu_Encoder_Kind_Render, "Not rendering.");
				id<MTLRenderCommandEncoder> encoder = encoder_state.render_encoder;

				const Gpu_Draw_Desc* draw = &command.data.draw->desc;
				Gpu_Render_Pipeline_Data* pipeline_data = gpu_render_pipeline_pool_resolve(&gpu.render_pipeline_pool, draw->pipeline);

				// apply bindings
				for (u32 i = 0; i < draw->binding_count; i++) {
					const Gpu_Binding binding = draw->bindings[i];
					switch (binding.kind) {
						case Gpu_Binding_Kind_Sampled_Texture:
						case Gpu_Binding_Kind_Storage_Texture: {
							Gpu_Texture_Data* texture_data = gpu_texture_pool_resolve(&gpu.texture_pool, binding.texture);
							[encoder setVertexTexture:texture_data->texture atIndex:i];
							[encoder setFragmentTexture:texture_data->texture atIndex:i];
						} break;

						case Gpu_Binding_Kind_Sampler: {
							Gpu_Sampler_Data* sampler_data = gpu_sampler_pool_resolve(&gpu.sampler_pool, binding.sampler);
							[encoder setVertexSamplerState:sampler_data->sampler atIndex:i];
							[encoder setFragmentSamplerState:sampler_data->sampler atIndex:i];
						} break;

						case Gpu_Binding_Kind_Slice: {
							Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, binding.slice.heap);
							[encoder setVertexBuffer:heap_data->buffer offset:binding.slice.offset atIndex:i];
							[encoder setFragmentBuffer:heap_data->buffer offset:binding.slice.offset atIndex:i];
						} break;
					}
				}

				[encoder setRenderPipelineState:pipeline_data->pipeline_state];
				[encoder setCullMode:gpu_cull_mode_to_mtl_cull_mode(pipeline_data->cull_mode)];

				const MTLPrimitiveType primitive_type = gpu_primitive_kind_to_mtl_primitive_type(pipeline_data->primitive_kind);
				[encoder drawPrimitives:primitive_type vertexStart:0 vertexCount:draw->vertex_count instanceCount:draw->instance_count];
			} break;

			case Gpu_Command_Kind_Dispatch: {
				const Gpu_Command_Dispatch* dispatch = command.data.dispatch;

				sl_assert(encoder_state.current != Gpu_Encoder_Kind_Render, "Must end render before compute dispatch.");
				gpu_start_compute_encoder(&encoder_state);
				id<MTLComputeCommandEncoder> encoder = encoder_state.compute_encoder;

				Gpu_Compute_Pipeline_Data* pipeline_data = gpu_compute_pipeline_pool_resolve(&gpu.compute_pipeline_pool, dispatch->pipeline);
				[encoder setComputePipelineState:pipeline_data->pipeline_state];

				// apply bindings
				for (u32 i = 0; i < dispatch->binding_count; i++) {
					const Gpu_Binding binding = dispatch->bindings[i];
					switch (binding.kind) {
						case Gpu_Binding_Kind_Sampled_Texture:
						case Gpu_Binding_Kind_Storage_Texture: {
							Gpu_Texture_Data* texture_data = gpu_texture_pool_resolve(&gpu.texture_pool, binding.texture);
							[encoder setTexture:texture_data->texture atIndex:i];
						} break;

						case Gpu_Binding_Kind_Sampler: {
							Gpu_Sampler_Data* sampler_data = gpu_sampler_pool_resolve(&gpu.sampler_pool, binding.sampler);
							[encoder setSamplerState:sampler_data->sampler atIndex:i];
						} break;

						case Gpu_Binding_Kind_Slice: {
							Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, binding.slice.heap);
							[encoder setBuffer:heap_data->buffer offset:binding.slice.offset atIndex:i];
						} break;
					}
				}

				const MTLSize groups = MTLSizeMake(dispatch->group_count.x, dispatch->group_count.y, dispatch->group_count.z);
				const MTLSize group_size = MTLSizeMake(pipeline_data->group_size.x, pipeline_data->group_size.y, pipeline_data->group_size.z);
				[encoder dispatchThreadgroups:groups threadsPerThreadgroup:group_size];
			} break;

			case Gpu_Command_Kind_Copy_Texture: {
				sl_assert(encoder_state.current != Gpu_Encoder_Kind_Render, "Must end render before copy.");
				gpu_start_blit_encoder(&encoder_state);

				const Gpu_Copy_Texture_Desc* copy = &command.data.copy_texture->desc;
				Gpu_Texture_Data* src_data = gpu_texture_pool_resolve(&gpu.texture_pool, copy->src);
				Gpu_Texture_Data* dst_data = gpu_texture_pool_resolve(&gpu.texture_pool, copy->dst);

				const MTLSize src_size = MTLSizeMake(copy->src_end.x - copy->src_start.x,
														copy->src_end.y - copy->src_start.y,
														copy->src_end.z - copy->src_start.z);

				[encoder_state.blit_encoder copyFromTexture:src_data->texture
												sourceSlice:copy->src_array_layer
												sourceLevel:copy->src_mip_level
											   sourceOrigin:MTLOriginMake(copy->src_start.x, copy->src_start.y, copy->src_start.z)
												 sourceSize:src_size
												  toTexture:dst_data->texture
										   destinationSlice:copy->dst_array_layer
										   destinationLevel:copy->dst_mip_level
										  destinationOrigin:MTLOriginMake(copy->dst_start.x, copy->dst_start.y, copy->dst_start.z)];
			} break;

			case Gpu_Command_Kind_Copy_Slice: {
				sl_assert(encoder_state.current != Gpu_Encoder_Kind_Render, "Must end render before copy.");
				gpu_start_blit_encoder(&encoder_state);

				const Gpu_Command_Copy_Slice* copy_slice = command.data.copy_slice;
				Gpu_Heap_Data* src_heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, copy_slice->src.heap);
				Gpu_Heap_Data* dst_heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, copy_slice->dst.heap);
				[encoder_state.blit_encoder copyFromBuffer:src_heap_data->buffer sourceOffset:copy_slice->src.offset toBuffer:dst_heap_data->buffer destinationOffset:copy_slice->dst.offset size:copy_slice->src.size];
			} break;

			case Gpu_Command_Kind_Copy_Slice_To_Texture: {
				sl_assert(encoder_state.current != Gpu_Encoder_Kind_Render, "Must end render before copy.");
				gpu_start_blit_encoder(&encoder_state);

				const Gpu_Copy_Slice_To_Texture_Desc* copy = &command.data.copy_slice_to_texture->desc;
				Gpu_Heap_Data* src_heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, copy->src.heap);
				Gpu_Texture_Data* dst_data = gpu_texture_pool_resolve(&gpu.texture_pool, copy->dst);

				const MTLSize dst_size = MTLSizeMake(copy->dst_end.x - copy->dst_start.x,
												 copy->dst_end.y - copy->dst_start.y,
												 copy->dst_end.z - copy->dst_start.z);

				const u64 src_bytes_per_row = copy->src_row_length * gpu_bytes_per_pixel_for_format(dst_data->desc.format);
				const u64 src_bytes_per_image = src_bytes_per_row * dst_size.height;

				[encoder_state.blit_encoder copyFromBuffer:src_heap_data->buffer
											  sourceOffset:copy->src.offset
										 sourceBytesPerRow:src_bytes_per_row
									   sourceBytesPerImage:src_bytes_per_image
												sourceSize:dst_size
												 toTexture:dst_data->texture
										  destinationSlice:copy->dst_array_layer
										  destinationLevel:copy->dst_mip_level
										 destinationOrigin:MTLOriginMake(copy->dst_start.x, copy->dst_start.y, copy->dst_start.z)];
			} break;

			case Gpu_Command_Kind_Barrier: {
				if (encoder_state.current == Gpu_Encoder_Kind_Compute) {
					[encoder_state.compute_encoder memoryBarrierWithScope:(MTLBarrierScopeBuffers | MTLBarrierScopeTextures)];
				}
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

	gpu_end_current_encoder(&encoder_state);
	gpu_destroy_encoder_state(&encoder_state);

	const u64 present_count = gpu_swapchain_present_seq_get_count(&cb_data->swapchain_presents);
	for (u64 present_idx = 0; present_idx < present_count; present_idx++) {
		Gpu_Swapchain_Present* present = gpu_swapchain_present_seq_get_ptr(&cb_data->swapchain_presents, present_idx);
		id<CAMetalDrawable> drawable = (__bridge_transfer id<CAMetalDrawable>)present->drawable;
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

void gpu_add_on_complete_callback(Gpu_Command_Buffer cb, void* ctx, Gpu_Callback_Fn fn) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	const Gpu_Callback callback = {
		.ctx = ctx,
		.fn = fn,
	};
	gpu_callback_seq_push(&cb_data->on_complete_callbacks, callback);
}

void gpu_transition_texture_layouts(Gpu_Command_Buffer cb, const Gpu_Texture* textures, const Gpu_Texture_Layout* layouts, u32 count) {
	// Metal doesn't have the concept of texture layouts.
}

// Heap
Gpu_Heap gpu_new_heap(u64 bytes, Gpu_Memory_Type memory_type) {
	MTLResourceOptions resource_options;
	switch (memory_type) {
		case Gpu_Memory_Type_Device_Local: {
			resource_options = MTLResourceStorageModePrivate;
		} break;

		case Gpu_Memory_Type_Host_Visible: {
			resource_options = MTLResourceStorageModeShared;
		} break;
	}

	MTLHeapDescriptor* descriptor = [MTLHeapDescriptor new];
	descriptor.size = bytes;
	descriptor.hazardTrackingMode = MTLHazardTrackingModeUntracked;
	descriptor.type = MTLHeapTypePlacement;
	descriptor.resourceOptions = resource_options;

	id<MTLHeap> mtl_heap = [gpu.device newHeapWithDescriptor:descriptor];
	id<MTLBuffer> mtl_buffer = [mtl_heap newBufferWithLength:bytes options:resource_options offset:0];

	void* host_ptr;
	switch (memory_type) {
		case Gpu_Memory_Type_Device_Local: {
			host_ptr = NULL;
		} break;

		case Gpu_Memory_Type_Host_Visible: {
			host_ptr = mtl_buffer.contents;
		} break;
	}

	Gpu_Heap result = gpu_heap_pool_acquire(&gpu.heap_pool);
	Gpu_Heap_Data* data = gpu_heap_pool_resolve(&gpu.heap_pool, result);
	data->size = bytes;
	data->memory_type = memory_type;
	data->heap = mtl_heap;
	data->buffer = mtl_buffer;
	data->host_ptr = host_ptr;
	return result;
}
void gpu_destroy_heap(Gpu_Heap heap) {
	Gpu_Heap_Data* data = gpu_heap_pool_resolve(&gpu.heap_pool, heap);
	data->heap = nil;
	data->buffer = nil;
	data->host_ptr = NULL;
	gpu_heap_pool_release(&gpu.heap_pool, heap);
}
u64 gpu_get_heap_size(Gpu_Heap heap) {
	Gpu_Heap_Data* data = gpu_heap_pool_resolve(&gpu.heap_pool, heap);
	return data->size;
}
Gpu_Slice gpu_get_heap_slice(Gpu_Heap heap) {
	return gpu_slice(heap, 0, gpu_get_heap_size(heap));
}

// Slice
void* gpu_get_slice_host_ptr(Gpu_Slice slice) {
	Gpu_Heap_Data* data = gpu_heap_pool_resolve(&gpu.heap_pool, slice.heap);
	switch (data->memory_type) {
		case Gpu_Memory_Type_Host_Visible: {
			return (u8*)data->host_ptr + slice.offset;
		} break;

		case Gpu_Memory_Type_Device_Local: {
			return NULL;
		} break;
	}
}
void gpu_flush_slice_to_gpu(Gpu_Slice slice) {
	// Memory reads/writes are coherent on Apple GPUs.
}
void gpu_flush_slice_from_gpu(Gpu_Slice slice) {
	// Memory reads/writes are coherent on Apple GPUs.
}

// Sampler
MTLSamplerDescriptor* gpu_sampler_desc_to_mtl_sampler_descriptor(const Gpu_Sampler_Desc* desc) {
	MTLSamplerDescriptor* result = [MTLSamplerDescriptor new];
	result.minFilter = gpu_filter_to_mtl_sampler_min_mag_filter(desc->min_filter);
	result.magFilter = gpu_filter_to_mtl_sampler_min_mag_filter(desc->mag_filter);
	result.mipFilter = gpu_filter_to_mtl_sampler_mip_filter(desc->mip_filter);
	result.sAddressMode = gpu_sampler_address_mode_to_mtl_sampler_address_mode(desc->address_mode_x);
	result.tAddressMode = gpu_sampler_address_mode_to_mtl_sampler_address_mode(desc->address_mode_y);
	result.rAddressMode = gpu_sampler_address_mode_to_mtl_sampler_address_mode(desc->address_mode_z);
	result.normalizedCoordinates = (desc->coordinate == Gpu_Sampler_Coordinate_Normalised);
	return result;
}
Gpu_Sampler gpu_new_sampler(const Gpu_Sampler_Desc* desc) {
	MTLSamplerDescriptor* mtl_desc = gpu_sampler_desc_to_mtl_sampler_descriptor(desc);
	id<MTLSamplerState> sampler = [gpu.device newSamplerStateWithDescriptor:mtl_desc];

	Gpu_Sampler result = gpu_sampler_pool_acquire(&gpu.sampler_pool);
	Gpu_Sampler_Data* data = gpu_sampler_pool_resolve(&gpu.sampler_pool, result);
	data->sampler = sampler;
	return result;
}
void gpu_destroy_sampler(Gpu_Sampler sampler) {
	Gpu_Sampler_Data* data = gpu_sampler_pool_resolve(&gpu.sampler_pool, sampler);
	data->sampler = nil;
	gpu_sampler_pool_release(&gpu.sampler_pool, sampler);
}

// Texture
MTLTextureDescriptor* gpu_texture_desc_to_mtl_texture_descriptor(const Gpu_Texture_Desc* desc) {
	MTLTextureDescriptor* result = [MTLTextureDescriptor new];
	result.width = desc->size.x;
	result.height = desc->size.y;
	result.depth = desc->size.z;
	result.mipmapLevelCount = desc->mip_levels;
	result.arrayLength = desc->array_layers;
	result.pixelFormat = gpu_format_to_mtl_pixel_format(desc->format);
	result.textureType = gpu_texture_kind_to_mtl_texture_type(desc->kind);
	result.usage = gpu_texture_usage_to_mtl_texture_usage(desc->usage);
	result.hazardTrackingMode = MTLHazardTrackingModeUntracked;
	return result;
}
Gpu_Texture_Desc gpu_mtl_texture_get_texture_desc(id<MTLTexture> texture) {
	const Gpu_Texture_Desc result = {
		.size = {
			(u32)texture.width,
			(u32)texture.height,
			(u32)texture.depth,
		},
		.mip_levels = (u32)texture.mipmapLevelCount,
		.array_layers = (u32)texture.arrayLength,
		.format = gpu_mtl_pixel_format_to_format(texture.pixelFormat),
		.kind = gpu_mtl_texture_type_to_texture_kind(texture.textureType),
		.usage = gpu_mtl_texture_usage_to_texture_usage(texture.usage),
	};
	return result;
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
	MTLTextureDescriptor* mtl_desc = gpu_texture_desc_to_mtl_texture_descriptor(desc);
	Gpu_Heap_Data* heap_data = gpu_heap_pool_resolve(&gpu.heap_pool, slice.heap);
	mtl_desc.storageMode = heap_data->heap.storageMode;
	id<MTLTexture> mtl_texture = [heap_data->heap newTextureWithDescriptor:mtl_desc offset:slice.offset];

	Gpu_Texture result = gpu_texture_pool_acquire(&gpu.texture_pool);
	Gpu_Texture_Data* data = gpu_texture_pool_resolve(&gpu.texture_pool, result);
	data->texture = mtl_texture;
	data->desc = *desc;
	return result;
}
void gpu_destroy_texture(Gpu_Texture texture) {
	Gpu_Texture_Data* data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
	data->texture = nil;
	gpu_texture_pool_release(&gpu.texture_pool, texture);
}
const Gpu_Texture_Desc* gpu_get_texture_desc(Gpu_Texture texture) {
	Gpu_Texture_Data* data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
	return &data->desc;
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
	swapchain_data->metal_layer.pixelFormat = gpu_format_to_mtl_pixel_format(swapchain_desc.format);
	id<CAMetalDrawable> drawable = [swapchain_data->metal_layer nextDrawable];
	sl_mutex_unlock(&swapchain_data->mutex);

	if (drawable == nil) {
		return false;
	}

	Gpu_Texture texture = gpu_texture_pool_acquire(&gpu.texture_pool);
	Gpu_Texture_Data* texture_data = gpu_texture_pool_resolve(&gpu.texture_pool, texture);
	texture_data->texture = drawable.texture;
	texture_data->desc = gpu_mtl_texture_get_texture_desc(drawable.texture);
	*out_texture = texture;
	gpu_texture_seq_push(&cb_data->cleanup_textures, texture);

	const Gpu_Swapchain_Present present = {
		.drawable = (__bridge_retained void*)drawable,
	};
	gpu_swapchain_present_seq_push(&cb_data->swapchain_presents, present);

	return true;
}

void gpu_begin_render(Gpu_Command_Buffer cb, const Gpu_Render_Pass_Layout* layout, const Gpu_Render_Pass_Values* values) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Command_Begin_Render* begin_render;
	allocator_new(&cb_data->arena->allocator, begin_render, 1);
	*begin_render = (Gpu_Command_Begin_Render) {
		.layout = *layout,
		.values = *values,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Begin_Render,
		.data = {
			.begin_render = begin_render,
		},
	});
}
void gpu_end_render(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_End_Render,
	});
}

// Render

MTLFunctionConstantValues* gpu_function_constants_to_mtl_function_constant_values(const Gpu_Function_Constant* constants, u32 count) {
	MTLFunctionConstantValues* result = [MTLFunctionConstantValues new];
	
	for (u32 i = 0; i < count; i++) {
		const Gpu_Function_Constant constant = constants[i];
		switch (constant.kind) {
			case Gpu_Function_Constant_Kind_U32: {
				[result setConstantValue:&constant.v_u32 type:MTLDataTypeUInt atIndex:constant.index];
			} break;
				
			case Gpu_Function_Constant_Kind_Bool: {
				[result setConstantValue:&constant.v_bool type:MTLDataTypeBool atIndex:constant.index];
			} break;
		}
	}
	
	return result;
}

Gpu_Render_Pipeline gpu_new_render_pipeline(const Gpu_Render_Pipeline_Desc* desc) {
	Gpu_Shader_Blob_Data* vert_blob_data = gpu_shader_blob_pool_resolve(&gpu.shader_blob_pool, desc->vertex_blob);
	Gpu_Shader_Blob_Data* frag_blob_data = gpu_shader_blob_pool_resolve(&gpu.shader_blob_pool, desc->fragment_blob);

	if ((vert_blob_data == NULL) || (frag_blob_data == NULL)) {
		return SL_HANDLE_NULL;
	}
	
	MTLFunctionConstantValues* constants = gpu_function_constants_to_mtl_function_constant_values(desc->constants, desc->constant_count);

	id<MTLFunction> vert_function = [vert_blob_data->library newFunctionWithName:[NSString stringWithCString:desc->vertex_entry_point encoding:NSUTF8StringEncoding] constantValues:constants error:nil];
	if (!vert_function) {
		return SL_HANDLE_NULL;
	}

	id<MTLFunction> frag_function = [frag_blob_data->library newFunctionWithName:[NSString stringWithCString:desc->fragment_entry_point encoding:NSUTF8StringEncoding] constantValues:constants error:nil];
	if (!frag_function) {
		return SL_HANDLE_NULL;
	}

	MTLRenderPipelineDescriptor* pipeline_desc = [MTLRenderPipelineDescriptor new];
	pipeline_desc.vertexFunction = vert_function;
	pipeline_desc.fragmentFunction = frag_function;
	pipeline_desc.inputPrimitiveTopology = gpu_primitive_kind_to_mtl_primitive_topology_class(desc->primitive_kind);
	for (u8 i = 0; i < desc->render_pass_layout.attachment_count; i++) {
		MTLRenderPipelineColorAttachmentDescriptor* mtl_attachment = pipeline_desc.colorAttachments[i];
		mtl_attachment.pixelFormat = gpu_format_to_mtl_pixel_format(desc->render_pass_layout.attachments[i].format);
		
		if (desc->alpha_blending) {
			mtl_attachment.blendingEnabled = YES;
			mtl_attachment.rgbBlendOperation = MTLBlendOperationAdd;
			mtl_attachment.alphaBlendOperation = MTLBlendOperationAdd;
			mtl_attachment.sourceRGBBlendFactor = MTLBlendFactorOne;
			mtl_attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
			mtl_attachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
			mtl_attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
		} else {
			mtl_attachment.blendingEnabled = NO;
		}
	}

	id<MTLRenderPipelineState> pipeline_state = [gpu.device newRenderPipelineStateWithDescriptor:pipeline_desc error:nil];
	Gpu_Render_Pipeline result = gpu_render_pipeline_pool_acquire(&gpu.render_pipeline_pool);
	Gpu_Render_Pipeline_Data* data = gpu_render_pipeline_pool_resolve(&gpu.render_pipeline_pool, result);
	data->pipeline_state = pipeline_state;
	data->cull_mode = desc->cull_mode;
	data->primitive_kind = desc->primitive_kind;
	return result;
}

// Compute
Gpu_Compute_Pipeline gpu_new_compute_pipeline(const Gpu_Compute_Pipeline_Desc* desc) {
	Gpu_Shader_Blob_Data* blob_data = gpu_shader_blob_pool_resolve(&gpu.shader_blob_pool, desc->blob);
	if (blob_data == NULL) {
		return SL_HANDLE_NULL;
	}

	id<MTLFunction> function = [blob_data->library newFunctionWithName:[NSString stringWithCString:desc->entry_point encoding:NSUTF8StringEncoding]];
	if (!function) {
		return SL_HANDLE_NULL;
	}

	id<MTLComputePipelineState> pipeline_state = [gpu.device newComputePipelineStateWithFunction:function error:nil];
	if (!pipeline_state) {
		return SL_HANDLE_NULL;
	}

	Gpu_Compute_Pipeline result = gpu_compute_pipeline_pool_acquire(&gpu.compute_pipeline_pool);
	Gpu_Compute_Pipeline_Data* data = gpu_compute_pipeline_pool_resolve(&gpu.compute_pipeline_pool, result);
	data->pipeline_state = pipeline_state;
	data->group_size = desc->group_size;
	return result;
}

void gpu_draw(Gpu_Command_Buffer cb, const Gpu_Draw_Desc* desc) {
	gpu_validate(gpu_render_pipeline_pool_resolve(&gpu.render_pipeline_pool, desc->pipeline), "Invalid pipeline.");

	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	gpu_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Command_Draw* draw;
	allocator_new(&cb_data->arena->allocator, draw, 1);
	*draw = (Gpu_Command_Draw) {
		.desc = *desc,
	};

	allocator_new(&cb_data->arena->allocator, draw->desc.bindings, desc->binding_count);
	memcpy((void*)draw->desc.bindings, desc->bindings, sizeof(Gpu_Binding) * desc->binding_count);

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Draw,
		.data.draw = draw,
	});
}

void gpu_dispatch(Gpu_Command_Buffer cb, Gpu_Compute_Pipeline pipeline, const Gpu_Binding* bindings, u32 binding_count, vec3_u32 group_count) {
	gpu_validate(gpu_compute_pipeline_pool_resolve(&gpu.compute_pipeline_pool, pipeline), "Invalid pipeline.");

	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	gpu_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Binding* bindings_copy;
	allocator_new(&cb_data->arena->allocator, bindings_copy, binding_count);
	memcpy(bindings_copy, bindings, sizeof(Gpu_Binding) * binding_count);

	Gpu_Command_Dispatch* dispatch;
	allocator_new(&cb_data->arena->allocator, dispatch, 1);
	*dispatch = (Gpu_Command_Dispatch) {
		.pipeline = pipeline,
		.bindings = bindings_copy,
		.binding_count = binding_count,
		.group_count = group_count,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Dispatch,
		.data.dispatch = dispatch,
	});
}

void gpu_copy_texture(Gpu_Command_Buffer cb, const Gpu_Copy_Texture_Desc* desc) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	gpu_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Command_Copy_Texture* copy;
	allocator_new(&cb_data->arena->allocator, copy, 1);
	*copy = (Gpu_Command_Copy_Texture) {
		.desc = *desc,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Copy_Texture,
		.data.copy_texture = copy,
	});
}

void gpu_copy_slice(Gpu_Command_Buffer cb, Gpu_Slice src, Gpu_Slice dst) {
	gpu_validate(src.size == dst.size, "Sizes must match");

	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	gpu_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Command_Copy_Slice* copy;
	allocator_new(&cb_data->arena->allocator, copy, 1);
	*copy = (Gpu_Command_Copy_Slice) {
		.src = src,
		.dst = dst,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Copy_Slice,
		.data.copy_slice = copy,
	});
}

void gpu_copy_slice_to_texture(Gpu_Command_Buffer cb, const Gpu_Copy_Slice_To_Texture_Desc* desc) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	gpu_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	Gpu_Command_Copy_Slice_To_Texture* copy;
	allocator_new(&cb_data->arena->allocator, copy, 1);
	*copy = (Gpu_Command_Copy_Slice_To_Texture) {
		.desc = *desc,
	};

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Copy_Slice_To_Texture,
		.data.copy_slice_to_texture = copy,
	});
}

void gpu_barrier(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Data* cb_data = gpu_resolve_command_buffer_data(cb);
	gpu_validate(cb_data, "Invalid command buffer.");
	sl_assert(cb_data->state == Gpu_Command_Buffer_State_Recording, "Command buffer should be in the recording state.");

	gpu_command_seq_push(&cb_data->commands, (Gpu_Command) {
		.kind = Gpu_Command_Kind_Barrier,
	});
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

// Shader Blob
Gpu_Shader_Blob gpu_new_shader_blob(const Gpu_Shader_Blob_Desc* desc) {
    if (desc->metallib.size == 0) {
        return SL_HANDLE_NULL;
    }

	dispatch_data_t buffer_data = dispatch_data_create(desc->metallib.data, desc->metallib.size, nil, ^{});
	
	NSError* err = nil;
	id<MTLLibrary> library = [gpu.device newLibraryWithData:buffer_data error:&err];
	if (library == nil) {
		return SL_HANDLE_NULL;
	}

	Gpu_Shader_Blob blob = gpu_shader_blob_pool_acquire(&gpu.shader_blob_pool);
	Gpu_Shader_Blob_Data* data = gpu_shader_blob_pool_resolve(&gpu.shader_blob_pool, blob);
	data->library = library;
	return blob;
}
void gpu_destroy_shader_blob(Gpu_Shader_Blob blob) {
	Gpu_Shader_Blob_Data* data = gpu_shader_blob_pool_resolve(&gpu.shader_blob_pool, blob);
	data->library = nil;
	gpu_shader_blob_pool_release(&gpu.shader_blob_pool, blob);
}
