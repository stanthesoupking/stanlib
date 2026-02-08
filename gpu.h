/// GPU
///
/// In at least `.c` or `.m` file:
/// ```
/// #define GPU_IMPLEMENTATION 1
/// #include "gpu.h"
/// ```

#include "core.h"

_Pragma("clang assume_nonnull begin")

typedef enum Gpu_Memory {
	Gpu_Memory_Shared,
	Gpu_Memory_Private,
} Gpu_Memory;

typedef struct Gpu_Queue { u64 handle; } Gpu_Queue;
typedef struct Gpu_Command_Buffer { u64 handle; } Gpu_Command_Buffer;
typedef struct Gpu_Compute_Pipeline { u64 handle; } Gpu_Compute_Pipeline;
typedef struct Gpu_Render_Pipeline { u64 handle; } Gpu_Render_Pipeline;
typedef struct Gpu_Heap { u64 handle; } Gpu_Heap;
typedef struct Gpu_Semaphore { u64 handle; } Gpu_Semaphore;
typedef struct Gpu_Texture { u64 handle; } Gpu_Texture;
typedef struct Gpu_Swapchain { u64 handle; } Gpu_Swapchain;
typedef struct Gpu_Swapchain_Image { u64 handle; } Gpu_Swapchain_Image;
typedef u64 Gpu_Address;

#define GPU_TEXTURE_NULL ((GPU_Texture) { .handle = 0 })

typedef struct Gpu_Size_And_Align {
	u64 size;
	u64 align;
} Gpu_Size_And_Align;

typedef enum Gpu_Format {
	Gpu_Format_None,
	
	Gpu_Format_A8_Unorm,
	Gpu_Format_R8_Unorm,
	Gpu_Format_R8_Unorm_sRGB,
	Gpu_Format_R8_Snorm,
	Gpu_Format_R8_Uint,
	Gpu_Format_R8_Sint,
	
	Gpu_Format_R16_Unorm,
	Gpu_Format_R16_Snorm,
	Gpu_Format_R16_Uint,
	Gpu_Format_R16_Sint,
	Gpu_Format_R16_Float,
	
	Gpu_Format_RG8_Unorm,
	Gpu_Format_RG8_Unorm_sRGB,
	Gpu_Format_RG8_Snorm,
	Gpu_Format_RG8_Uint,
	Gpu_Format_RG8_Sint,
	
	Gpu_Format_R32_Uint,
	Gpu_Format_R32_Sint,
	Gpu_Format_R32_Float,
	
	Gpu_Format_RG16_Unorm,
	Gpu_Format_RG16_Snorm,
	Gpu_Format_RG16_Uint,
	Gpu_Format_RG16_Sint,
	Gpu_Format_RG16_Float,
	
	Gpu_Format_RGBA8_Unorm,
	Gpu_Format_RGBA8_Unorm_sRGB,
	Gpu_Format_RGBA8_Snorm,
	Gpu_Format_RGBA8_Uint,
	Gpu_Format_RGBA8_Sint,
	
	Gpu_Format_BGRA8_Unorm,
	Gpu_Format_BGRA8_Unorm_sRGB,
	
	Gpu_Format_RGB10A2_Unorm,
	Gpu_Format_RGB10A2_Uint,
	Gpu_Format_RG11B10_Float,
	Gpu_Format_RGB9E5_Float,
	Gpu_Format_BGR10A2_Unorm,
	Gpu_Format_BGR10_XR,
	Gpu_Format_BGR10_XR_sRGB,
	
	Gpu_Format_RG32_Uint,
	Gpu_Format_RG32_Sint,
	Gpu_Format_RG32_Float,
	
	Gpu_Format_RGBA16_Unorm,
	Gpu_Format_RGBA16_Snorm,
	Gpu_Format_RGBA16_Uint,
	Gpu_Format_RGBA16_Sint,
	Gpu_Format_RGBA16_Float,
	
	Gpu_Format_BGRA10_XR,
	Gpu_Format_BGRA10_XR_sRGB,
	Gpu_Format_RGBA32_Uint,
	Gpu_Format_RGBA32_Sint,
	Gpu_Format_RGBA32_Float,
	Gpu_Format_Depth16_Unorm,
	Gpu_Format_Depth32_Float,
	Gpu_Format_Stencil8,
} Gpu_Format;

typedef enum Gpu_Texture_Usage {
	Gpu_Texture_Usage_None = 0,
	Gpu_Texture_Usage_Shader_Read = 1 << 0,
	Gpu_Texture_Usage_Shader_Write = 1 << 1,
	Gpu_Texture_Usage_Render_Target = 1 << 2,
} Gpu_Texture_Usage;

typedef enum Gpu_Texture_Kind {
	Gpu_Texture_Kind_1D,
	Gpu_Texture_Kind_1D_Array,
	Gpu_Texture_Kind_2D,
	Gpu_Texture_Kind_2D_Array,
	Gpu_Texture_Kind_3D,
	Gpu_Texture_Kind_Cube,
	Gpu_Texture_Kind_Cube_Array,
} Gpu_Texture_Kind;

typedef struct Gpu_Texture_Desc {
	Gpu_Texture_Kind kind;
	Gpu_Format format;
	Gpu_Texture_Usage usage;
	vec3_u32 size;
	u32 array_length;
	u32 mip_count;
} Gpu_Texture_Desc;

typedef enum Gpu_Primitive_Kind {
	Gpu_Primitive_Kind_Triangle,
	Gpu_Primitive_Kind_Triangle_Strip,
} Gpu_Primitive_Kind;

#define GPU_RENDER_PASS_MAXIMUM_COLOR_TARGETS 8

typedef struct Gpu_Render_Pass {
	Gpu_Texture color_targets[GPU_RENDER_PASS_MAXIMUM_COLOR_TARGETS];
	Gpu_Texture depth_target;
} Gpu_Render_Pass;

typedef struct Gpu_Render_Pipeline_Desc_Target {
	Gpu_Format format;
} Gpu_Render_Pipeline_Desc_Target;

typedef struct Gpu_Render_Pipeline_Desc {
	const char*_Nonnull vertex_function_name;
	const char*_Nonnull fragment_function_name;
	
	Gpu_Render_Pipeline_Desc_Target color_targets[GPU_RENDER_PASS_MAXIMUM_COLOR_TARGETS];
	Gpu_Render_Pipeline_Desc_Target depth_target;
} Gpu_Render_Pipeline_Desc;

typedef struct Gpu_Heap_Address {
	Gpu_Heap heap;
	u64 offset;
} Gpu_Heap_Address;

typedef struct Gpu_Slice {
	Gpu_Heap heap;
	u64 offset;
	u64 size;
} Gpu_Slice;

#define GPU_SLICE_NULL ((Gpu_Slice) {})

static inline bool gpu_slice_is_null(Gpu_Slice slice) {
	return slice.heap.handle == 0;
}

typedef struct Gpu_Setup {
	const char*_Nullable metal_library_path;
} Gpu_Setup;

// Init/deinit
void gpu_init(Gpu_Setup setup);
void gpu_deinit(void);

// Heap
Gpu_Heap gpu_heap_new(u64 bytes, Gpu_Memory memory);
void gpu_heap_destroy(Gpu_Heap allocator);

// Slice
static inline Gpu_Slice gpu_slice(Gpu_Heap heap, u64 offset, u64 size) {
	return (Gpu_Slice) {
		.heap = heap,
		.offset = offset,
		.size = size,
	};
}
void* _Nullable gpu_slice_get_cpu_ptr(Gpu_Slice slice);
Gpu_Address gpu_slice_get_gpu_ptr(Gpu_Slice slice);

// Texture
Gpu_Size_And_Align gpu_texture_desc_get_size_and_align(Gpu_Texture_Desc texture_desc, Gpu_Heap heap);

Gpu_Texture gpu_texture_new(Gpu_Slice slice, Gpu_Texture_Desc texture_desc);
Gpu_Address gpu_texture_get_gpu_ptr(Gpu_Texture texture);
void gpu_texture_replace_contents(Gpu_Texture texture, void*_Nonnull bytes, u32 row_length);
void gpu_texture_destroy(Gpu_Texture texture);

// Semaphore
Gpu_Semaphore gpu_semaphore_new(u64 init_value);
void gpu_semaphore_destroy(Gpu_Semaphore semaphore);
void gpu_semaphore_wait(Gpu_Semaphore semaphore, u64 value);
void gpu_semaphore_signal(Gpu_Semaphore semaphore, Gpu_Queue queue, u64 value);

// Queue
Gpu_Queue gpu_queue_new(void);
void gpu_queue_destroy(Gpu_Queue queue);

// Compute Pipeline
Gpu_Compute_Pipeline gpu_compute_pipeline_new(const char*_Nonnull function_name);
void gpu_compute_pipeline_destroy(Gpu_Compute_Pipeline pipeline);

// Render Pipeline
Gpu_Render_Pipeline gpu_render_pipeline_new(Gpu_Render_Pipeline_Desc desc);
void gpu_render_pipeline_destroy(Gpu_Render_Pipeline pipeline);

// Command Buffer
Gpu_Command_Buffer gpu_command_buffer_new(void);
void gpu_submit(Gpu_Command_Buffer cb, Gpu_Queue queue);

void gpu_copy(Gpu_Command_Buffer cb, Gpu_Slice src, Gpu_Slice dst);

void gpu_draw(Gpu_Command_Buffer cb, const Gpu_Render_Pass*_Nonnull pass, Gpu_Render_Pipeline pipeline, Gpu_Slice data, Gpu_Primitive_Kind primitive_kind, u32 vertex_count, u32 instance_count);

void gpu_dispatch(Gpu_Command_Buffer cb, Gpu_Compute_Pipeline pipeline, Gpu_Slice data, vec3_u32 threadsPerThreadgroup, vec3_u32 threadgroupCount);

void gpu_barrier(Gpu_Command_Buffer cb);

// Swapchain
typedef struct Gpu_Swapchain_Desc {
	Gpu_Format format;
	bool vsync;
} Gpu_Swapchain_Desc;

void gpu_swapchain_destroy(Gpu_Swapchain swapchain);
Gpu_Swapchain_Image gpu_swapchain_get_next_image(Gpu_Swapchain swapchain);

// Swapchain Image
Gpu_Texture gpu_swapchain_image_get_texture(Gpu_Swapchain_Image swapchain_image);
void gpu_swapchain_image_wait(Gpu_Swapchain_Image swapchain_image, Gpu_Queue queue);
void gpu_swapchain_image_signal(Gpu_Swapchain_Image swapchain_image, Gpu_Queue queue);
void gpu_swapchain_image_present(Gpu_Swapchain_Image swapchain_image);

// Metal
void* gpu_get_metal_device(void);
Gpu_Texture gpu_texture_new_from_metal_texture(void* metal_texture);
Gpu_Swapchain gpu_swapchain_new_from_metal_layer(void* metal_layer, Gpu_Swapchain_Desc swapchain_desc);

#if GPU_IMPLEMENTATION

_Pragma("clang assume_nonnull end")
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
_Pragma("clang assume_nonnull begin")

static inline void gpu_assert(bool condition, const char*_Nonnull message) {
	if (!condition) {
		printf("gpu_assert: %s\n", message);
		abort();
	}
}

static inline void gpu_abort(const char*_Nonnull message) {
	printf("gpu_abort: %s\n", message);
	abort();
}

typedef struct Gpu_Heap_Data {
	void* heap; // id<MTLHeap>
	void* buffer; // id<MTLBuffer>
	Gpu_Memory memory;
	
	void*_Nullable buffer_cpu_ptr;
	Gpu_Address buffer_gpu_ptr;
	
	u64 position;
	u64 length;
} Gpu_Heap_Data;

typedef struct Gpu_Queue_Data {
	void* queue; // id<MTL4CommandQueue>
} Gpu_Queue_Data;

typedef struct Gpu_Command_Buffer_Data {
	void* cb; // id<MTL4CommandBuffer>
	void* command_allocator; // id<MTL4CommandAllocator>
	void* argument_table; // id<MTL4ArgumentTable>
	
	bool has_current_render_pass;
	Gpu_Render_Pass current_render_pass;
	Gpu_Render_Pipeline current_render_pipeline;
	
	void* _Nullable encoder; // id<MTL4CommandEncoder>
} Gpu_Command_Buffer_Data;

typedef struct Gpu_Compute_Pipeline_Data {
	void* pipeline_state; // id<MTLComputePipelineState>
} Gpu_Compute_Pipeline_Data;

typedef struct Gpu_Render_Pipeline_Data {
	void* pipeline_state; // id<MTLRenderPipelineState>
} Gpu_Render_Pipeline_Data;

typedef struct Gpu_Semaphore_Data {
	void* event; // id<MTLSharedEvent>
} Gpu_Semaphore_Data;

typedef struct Gpu_Texture_Data {
	void* texture; // id<MTLTexture>
	u64 gpu_address;
} Gpu_Texture_Data;

typedef struct Gpu_Swapchain_Data {
	void* metal_layer; // CAMetalLayer*
} Gpu_Swapchain_Data;

typedef struct Gpu_Swapchain_Image_Data {
	void* drawable; // id<CAMetalDrawable>
	Gpu_Texture texture;
} Gpu_Swapchain_Image_Data;

typedef struct Gpu_Pool_Entry {
	union {
		Gpu_Heap_Data heap;
		Gpu_Queue_Data queue;
		Gpu_Command_Buffer_Data command_buffer;
		Gpu_Compute_Pipeline_Data compute_pipeline;
		Gpu_Render_Pipeline_Data render_pipeline;
		Gpu_Texture_Data texture;
		Gpu_Swapchain_Data swapchain;
		Gpu_Swapchain_Image_Data swapchain_image;
		Gpu_Semaphore_Data semaphore;
	};
	
	u32 generation;
} Gpu_Pool_Entry;

#define GPU_POOL_CAPACITY 512

typedef struct Gpu_Pool {
	Gpu_Pool_Entry entries[GPU_POOL_CAPACITY];
	u32 entry_free_list[GPU_POOL_CAPACITY];
	u32 entry_free_list_length;
} Gpu_Pool;

static inline void gpu_pool_init(Gpu_Pool* pool) {
	for (u32 i = 0; i < GPU_POOL_CAPACITY; i++) {
		pool->entries[i].generation = 0;
		pool->entry_free_list[i] = (GPU_POOL_CAPACITY - 1) - i;
	}
	pool->entry_free_list_length = GPU_POOL_CAPACITY;
}
static inline Gpu_Pool_Entry*_Nullable gpu_pool_get_entry(Gpu_Pool* pool, u64 handle) {
	const u32 entry_index = (u32)(handle & u32_max);
	const u32 entry_generation = (u32)((handle >> 32ULL) & u32_max);
	Gpu_Pool_Entry* entry = pool->entries + entry_index;
	if (entry->generation == entry_generation) {
		return entry;
	} else {
		return NULL;
	}
}
static inline u64 gpu_pool_borrow_handle(Gpu_Pool* pool) {
	gpu_assert(pool->entry_free_list_length > 0, "Exhausted pool of GPU objects.");
	u32 entry_index = pool->entry_free_list[--pool->entry_free_list_length];
	u32 entry_generation = ++pool->entries[entry_index].generation;
	return (u64)entry_index | ((u64)entry_generation << 32ULL);
}
static inline void gpu_pool_return_handle(Gpu_Pool* pool, u64 handle) {
	Gpu_Pool_Entry* entry = gpu_pool_get_entry(pool, handle);
	gpu_assert(entry != NULL, "Attempted to return invalid/deallocated handle.");
	++entry->generation;
	
	const u32 entry_index = (u32)(handle & u32_max);
	pool->entry_free_list[pool->entry_free_list_length++] = entry_index;
}

typedef struct Gpu_Context {
	id<MTLDevice> device;
	id<MTLLibrary> library;
	id<MTL4Compiler> compiler;
	id<MTLResidencySet> residency_set;
	id<MTLFence> fence;
	
	Gpu_Pool pool;
} Gpu_Context;

static Gpu_Context _context = {};

void gpu_init(Gpu_Setup setup) {
	_context = (Gpu_Context) {};
	_context.device = MTLCreateSystemDefaultDevice();
	_context.library = [_context.device newLibraryWithURL:[NSURL URLWithString:[NSString stringWithCString:setup.metal_library_path encoding:NSUTF8StringEncoding]] error:nil];
	gpu_assert(_context.library != nil, "Failed to load library.");
	_context.fence = [_context.device newFence];
	
	MTLResidencySetDescriptor* desc = [MTLResidencySetDescriptor new];
	_context.residency_set = [_context.device newResidencySetWithDescriptor:desc error:nil];
	
	MTL4CompilerDescriptor* compilerDescriptor = [MTL4CompilerDescriptor new];
	_context.compiler = [_context.device newCompilerWithDescriptor:compilerDescriptor error:nil];
	
	gpu_pool_init(&_context.pool);
}
void gpu_deinit(void) {
	_context.device = nil;
	_context = (Gpu_Context) {};
}

static inline MTLResourceOptions gpu_memory_get_metal_resource_options(Gpu_Memory memory) {
	switch (memory) {
		case Gpu_Memory_Shared: return MTLResourceStorageModeShared;
		case Gpu_Memory_Private: return MTLResourceStorageModePrivate;
	}
}

Gpu_Heap gpu_heap_new(u64 bytes, Gpu_Memory memory) {
	const Gpu_Heap heap_handle = (Gpu_Heap) {
		.handle = gpu_pool_borrow_handle(&_context.pool),
	};
	Gpu_Heap_Data* heap_data = &gpu_pool_get_entry(&_context.pool, heap_handle.handle)->heap;
	
	MTLHeapDescriptor* heapDescriptor = [MTLHeapDescriptor new];
	heapDescriptor.size = bytes;
	heapDescriptor.type = MTLHeapTypePlacement;
	heapDescriptor.hazardTrackingMode = MTLHazardTrackingModeUntracked;
	
	switch (memory) {
		case Gpu_Memory_Shared: {
			heapDescriptor.storageMode = MTLStorageModeShared;
		} break;
			
		case Gpu_Memory_Private: {
			heapDescriptor.storageMode = MTLStorageModePrivate;
		} break;
	}
	
	id<MTLHeap> heap = [_context.device newHeapWithDescriptor:heapDescriptor];
	id<MTLBuffer> buffer = [heap newBufferWithLength:bytes options:gpu_memory_get_metal_resource_options(memory) offset:0];
	
	heap.label = [NSString stringWithFormat:@"Gpu_Heap <Heap> %llu", heap_handle.handle];
	buffer.label = [NSString stringWithFormat:@"Gpu_Heap <Buffer> %llu", heap_handle.handle];
	
	*heap_data = (Gpu_Heap_Data) {
		.heap = (__bridge_retained void*)heap,
		.buffer = (__bridge_retained void*)buffer,
		.memory = memory,
	};
	
	switch (memory) {
		case Gpu_Memory_Shared: {
			heap_data->buffer_cpu_ptr = buffer.contents;
		} break;
			
		case Gpu_Memory_Private: {
			heap_data->buffer_cpu_ptr = NULL;
		} break;
	}
	
	heap_data->buffer_gpu_ptr = buffer.gpuAddress;
	heap_data->position = 0;
	heap_data->length = bytes;
	
	[_context.residency_set addAllocation:heap];
	[_context.residency_set commit];
	
	return heap_handle;
}
void gpu_heap_destroy(Gpu_Heap heap) {
	Gpu_Heap_Data* heap_data = &gpu_pool_get_entry(&_context.pool, heap.handle)->heap;
	id<MTLHeap> mtl_heap = (__bridge_transfer id<MTLHeap>)heap_data->heap;
	[_context.residency_set removeAllocation:mtl_heap];
	[_context.residency_set commit];
	gpu_pool_return_handle(&_context.pool, heap.handle);
}

void* _Nullable gpu_slice_get_cpu_ptr(Gpu_Slice slice) {
	Gpu_Heap_Data* heap_data = &gpu_pool_get_entry(&_context.pool, slice.heap.handle)->heap;
	switch (heap_data->memory) {
		case Gpu_Memory_Shared:
			return (uint8_t*)heap_data->buffer_cpu_ptr + slice.offset;
			
		case Gpu_Memory_Private:
			return NULL;
	}
	
}
Gpu_Address gpu_slice_get_gpu_ptr(Gpu_Slice slice) {
	Gpu_Heap_Data* heap_data = &gpu_pool_get_entry(&_context.pool, slice.heap.handle)->heap;
	return heap_data->buffer_gpu_ptr + slice.offset;
}

MTLPixelFormat gpu_format_get_metal_format(Gpu_Format format) {
	switch (format) {
		case Gpu_Format_None: return MTLPixelFormatInvalid;
		case Gpu_Format_A8_Unorm: return MTLPixelFormatA8Unorm;
		case Gpu_Format_R8_Unorm: return MTLPixelFormatR8Unorm;
		case Gpu_Format_R8_Unorm_sRGB: return MTLPixelFormatR8Unorm_sRGB;
		case Gpu_Format_R8_Snorm: return MTLPixelFormatR8Snorm;
		case Gpu_Format_R8_Uint: return MTLPixelFormatR8Uint;
		case Gpu_Format_R8_Sint: return MTLPixelFormatR8Sint;
		case Gpu_Format_R16_Unorm: return MTLPixelFormatR16Unorm;
		case Gpu_Format_R16_Snorm: return MTLPixelFormatR16Snorm;
		case Gpu_Format_R16_Uint: return MTLPixelFormatR16Uint;
		case Gpu_Format_R16_Sint: return MTLPixelFormatR16Sint;
		case Gpu_Format_R16_Float: return MTLPixelFormatR16Float;
		case Gpu_Format_RG8_Unorm: return MTLPixelFormatRG8Unorm;
		case Gpu_Format_RG8_Unorm_sRGB: return MTLPixelFormatRG8Unorm_sRGB;
		case Gpu_Format_RG8_Snorm: return MTLPixelFormatRG8Snorm;
		case Gpu_Format_RG8_Uint: return MTLPixelFormatRG8Uint;
		case Gpu_Format_RG8_Sint: return MTLPixelFormatRG8Sint;
		case Gpu_Format_R32_Uint: return MTLPixelFormatR32Uint;
		case Gpu_Format_R32_Sint: return MTLPixelFormatR32Sint;
		case Gpu_Format_R32_Float: return MTLPixelFormatR32Float;
		case Gpu_Format_RG16_Unorm: return MTLPixelFormatRG16Unorm;
		case Gpu_Format_RG16_Snorm: return MTLPixelFormatRG16Snorm;
		case Gpu_Format_RG16_Uint: return MTLPixelFormatRG16Uint;
		case Gpu_Format_RG16_Sint: return MTLPixelFormatRG16Sint;
		case Gpu_Format_RG16_Float: return MTLPixelFormatRG16Float;
		case Gpu_Format_RGBA8_Unorm: return MTLPixelFormatRGBA8Unorm;
		case Gpu_Format_RGBA8_Unorm_sRGB: return MTLPixelFormatRGBA8Unorm_sRGB;
		case Gpu_Format_RGBA8_Snorm: return MTLPixelFormatRGBA8Snorm;
		case Gpu_Format_RGBA8_Uint: return MTLPixelFormatRGBA8Uint;
		case Gpu_Format_RGBA8_Sint: return MTLPixelFormatRGBA8Sint;
		case Gpu_Format_BGRA8_Unorm: return MTLPixelFormatBGRA8Unorm;
		case Gpu_Format_BGRA8_Unorm_sRGB: return MTLPixelFormatBGRA8Unorm_sRGB;
		case Gpu_Format_RGB10A2_Unorm: return MTLPixelFormatRGB10A2Unorm;
		case Gpu_Format_RGB10A2_Uint: return MTLPixelFormatRGB10A2Uint;
		case Gpu_Format_RG11B10_Float: return MTLPixelFormatRG11B10Float;
		case Gpu_Format_RGB9E5_Float: return MTLPixelFormatRGB9E5Float;
		case Gpu_Format_BGR10A2_Unorm: return MTLPixelFormatBGR10A2Unorm;
		case Gpu_Format_BGR10_XR: return MTLPixelFormatBGR10_XR;
		case Gpu_Format_BGR10_XR_sRGB: return MTLPixelFormatBGR10_XR_sRGB;
		case Gpu_Format_RG32_Uint: return MTLPixelFormatRG32Uint;
		case Gpu_Format_RG32_Sint: return MTLPixelFormatRG32Sint;
		case Gpu_Format_RG32_Float: return MTLPixelFormatRG32Float;
		case Gpu_Format_RGBA16_Unorm: return MTLPixelFormatRGBA16Unorm;
		case Gpu_Format_RGBA16_Snorm: return MTLPixelFormatRGBA16Snorm;
		case Gpu_Format_RGBA16_Uint: return MTLPixelFormatRGBA16Uint;
		case Gpu_Format_RGBA16_Sint: return MTLPixelFormatRGBA16Sint;
		case Gpu_Format_RGBA16_Float: return MTLPixelFormatRGBA16Float;
		case Gpu_Format_BGRA10_XR: return MTLPixelFormatBGRA10_XR;
		case Gpu_Format_BGRA10_XR_sRGB: return MTLPixelFormatBGRA10_XR_sRGB;
		case Gpu_Format_RGBA32_Uint: return MTLPixelFormatRGBA32Uint;
		case Gpu_Format_RGBA32_Sint: return MTLPixelFormatRGBA32Sint;
		case Gpu_Format_RGBA32_Float: return MTLPixelFormatRGBA32Float;
		case Gpu_Format_Depth16_Unorm: return MTLPixelFormatDepth16Unorm;
		case Gpu_Format_Depth32_Float: return MTLPixelFormatDepth32Float;
		case Gpu_Format_Stencil8: return MTLPixelFormatStencil8;
	}
}
MTLTextureType gpu_texture_kind_get_metal_texture_type(Gpu_Texture_Kind kind) {
	switch (kind) {
		case Gpu_Texture_Kind_1D: return MTLTextureType1D;
		case Gpu_Texture_Kind_1D_Array: return MTLTextureType1DArray;
		case Gpu_Texture_Kind_2D: return MTLTextureType2D;
		case Gpu_Texture_Kind_2D_Array: return MTLTextureType2DArray;
		case Gpu_Texture_Kind_3D: return MTLTextureType3D;
		case Gpu_Texture_Kind_Cube: return MTLTextureTypeCube;
		case Gpu_Texture_Kind_Cube_Array: return MTLTextureTypeCubeArray;
	}
}
MTLTextureUsage gpu_texture_usage_get_metal_usage(Gpu_Texture_Usage usage) {
	MTLTextureUsage metalUsage = 0;
	if ((usage & Gpu_Texture_Usage_Shader_Read) > 0) {
		metalUsage |= MTLTextureUsageShaderRead;
	}
	if ((usage & Gpu_Texture_Usage_Shader_Write) > 0) {
		metalUsage |= MTLTextureUsageShaderWrite;
	}
	if ((usage & Gpu_Texture_Usage_Render_Target) > 0) {
		metalUsage |= MTLTextureUsageRenderTarget;
	}
	return metalUsage;
}

MTLTextureDescriptor* gpu_texture_desc_get_metal(Gpu_Texture_Desc texture_desc, Gpu_Heap heap) {
	Gpu_Heap_Data* heap_data = &gpu_pool_get_entry(&_context.pool, heap.handle)->heap;
	id<MTLHeap> mtl_heap = (__bridge id<MTLHeap>)heap_data->heap;
	
	MTLTextureDescriptor* metal_texture_desc = [MTLTextureDescriptor new];
	metal_texture_desc.width = texture_desc.size.x;
	metal_texture_desc.height = texture_desc.size.y;
	metal_texture_desc.depth = texture_desc.size.z;
	metal_texture_desc.pixelFormat = gpu_format_get_metal_format(texture_desc.format);
	metal_texture_desc.textureType = gpu_texture_kind_get_metal_texture_type(texture_desc.kind);
	metal_texture_desc.arrayLength = texture_desc.array_length;
	metal_texture_desc.usage = gpu_texture_usage_get_metal_usage(texture_desc.usage);
	metal_texture_desc.storageMode = mtl_heap.storageMode;
	
	return metal_texture_desc;
}

Gpu_Size_And_Align gpu_texture_desc_get_size_and_align(Gpu_Texture_Desc texture_desc, Gpu_Heap heap) {
	MTLTextureDescriptor* metal_texture_desc = gpu_texture_desc_get_metal(texture_desc, heap);
	MTLSizeAndAlign texture_size_and_align = [_context.device heapTextureSizeAndAlignWithDescriptor:metal_texture_desc];
	return (Gpu_Size_And_Align) {
		.size = texture_size_and_align.size,
		.align = texture_size_and_align.align,
	};
}

Gpu_Texture gpu_texture_new(Gpu_Slice slice, Gpu_Texture_Desc texture_desc) {
	Gpu_Texture texture_handle = (Gpu_Texture) {
		.handle = gpu_pool_borrow_handle(&_context.pool),
	};
	Gpu_Texture_Data* texture_data = &gpu_pool_get_entry(&_context.pool, texture_handle.handle)->texture;
	
	Gpu_Heap_Data* heap_data = &gpu_pool_get_entry(&_context.pool, slice.heap.handle)->heap;
	id<MTLHeap> mtl_heap = (__bridge id<MTLHeap>)heap_data->heap;
	
	MTLTextureDescriptor* metal_texture_desc = gpu_texture_desc_get_metal(texture_desc, slice.heap);
	
	id<MTLTexture> texture = [mtl_heap newTextureWithDescriptor:metal_texture_desc offset:slice.offset];
	*texture_data = (Gpu_Texture_Data) {
		.texture = (__bridge_retained void*)texture,
		.gpu_address = texture.gpuResourceID._impl,
	};
	
	return texture_handle;
}
Gpu_Address gpu_texture_get_gpu_ptr(Gpu_Texture texture) {
	Gpu_Texture_Data* texture_data = &gpu_pool_get_entry(&_context.pool, texture.handle)->texture;
	return texture_data->gpu_address;
}
void gpu_texture_replace_contents(Gpu_Texture texture, void* bytes, u32 row_length) {
	Gpu_Texture_Data* texture_data = &gpu_pool_get_entry(&_context.pool, texture.handle)->texture;
	id<MTLTexture> metal_texture = (__bridge id<MTLTexture>)texture_data->texture;
	gpu_assert(metal_texture.storageMode == MTLStorageModeShared, "Can only replace contents of shared textures.");
	[metal_texture replaceRegion:MTLRegionMake2D(0, 0, metal_texture.width, metal_texture.height) mipmapLevel:0 withBytes:bytes bytesPerRow:row_length];
}
void gpu_texture_destroy(Gpu_Texture texture) {
	Gpu_Texture_Data* texture_data = &gpu_pool_get_entry(&_context.pool, texture.handle)->texture;
	(void)(__bridge_transfer id<MTLTexture>)texture_data->texture;
	gpu_pool_return_handle(&_context.pool, texture.handle);
}

Gpu_Semaphore gpu_semaphore_new(u64 init_value) {
	const Gpu_Semaphore semaphore_handle = {
		.handle = gpu_pool_borrow_handle(&_context.pool),
	};
	Gpu_Semaphore_Data* semaphore_data = &gpu_pool_get_entry(&_context.pool, semaphore_handle.handle)->semaphore;
	
	id<MTLSharedEvent> metal_event = [_context.device newSharedEvent];
	metal_event.label = [NSString stringWithFormat:@"Gpu_Semaphore %llu", semaphore_handle.handle];
	
	[metal_event setSignaledValue:init_value];
	semaphore_data->event = (__bridge_retained void*)metal_event;
	
	return semaphore_handle;
}
void gpu_semaphore_wait(Gpu_Semaphore semaphore, u64 value) {
	Gpu_Semaphore_Data* semaphore_data = &gpu_pool_get_entry(&_context.pool, semaphore.handle)->semaphore;
	id<MTLSharedEvent> metal_event = (__bridge id<MTLSharedEvent>)semaphore_data->event;
	[metal_event waitUntilSignaledValue:value timeoutMS:UINT64_MAX];
}
void gpu_semaphore_signal(Gpu_Semaphore semaphore, Gpu_Queue queue, u64 value) {
	Gpu_Semaphore_Data* semaphore_data = &gpu_pool_get_entry(&_context.pool, semaphore.handle)->semaphore;
	Gpu_Queue_Data* queue_data = &gpu_pool_get_entry(&_context.pool, queue.handle)->queue;
	id<MTLSharedEvent> metal_event = (__bridge id<MTLSharedEvent>)semaphore_data->event;
	id<MTL4CommandQueue> metal_queue = (__bridge id<MTL4CommandQueue>)queue_data->queue;
	[metal_queue signalEvent:metal_event value:value];
}
void gpu_semaphore_destroy(Gpu_Semaphore semaphore) {
	Gpu_Semaphore_Data* semaphore_data = &gpu_pool_get_entry(&_context.pool, semaphore.handle)->semaphore;
	(void)(__bridge_transfer id<MTLSharedEvent>)semaphore_data->event;
	gpu_pool_return_handle(&_context.pool, semaphore.handle);
}

Gpu_Queue gpu_queue_new(void) {
	const Gpu_Queue queue_handle = {
		.handle = gpu_pool_borrow_handle(&_context.pool),
	};
	Gpu_Queue_Data* queue_data = &gpu_pool_get_entry(&_context.pool, queue_handle.handle)->queue;
	
	MTL4CommandQueueDescriptor* queue_desc = [MTL4CommandQueueDescriptor new];
	queue_desc.label = [NSString stringWithFormat:@"Gpu_Queue %llu", queue_handle.handle];
	
	id<MTL4CommandQueue> metal_queue = [_context.device newMTL4CommandQueueWithDescriptor:queue_desc error:nil];
	[metal_queue addResidencySet:_context.residency_set];
	
	queue_data->queue = (__bridge_retained void*)metal_queue;
	
	return queue_handle;
}
void gpu_queue_destroy(Gpu_Queue queue) {
	Gpu_Queue_Data* queue_data = &gpu_pool_get_entry(&_context.pool, queue.handle)->queue;
	id<MTL4CommandQueue> metal_queue = (__bridge_transfer id<MTL4CommandQueue>)queue_data->queue;
	[metal_queue removeResidencySet:_context.residency_set];
	gpu_pool_return_handle(&_context.pool, queue.handle);
}

Gpu_Command_Buffer gpu_command_buffer_new(void) {
	const Gpu_Command_Buffer cb_handle = {
		.handle = gpu_pool_borrow_handle(&_context.pool),
	};
	Gpu_Command_Buffer_Data* cb_data = &gpu_pool_get_entry(&_context.pool, cb_handle.handle)->command_buffer;
	
	id<MTL4CommandBuffer> metal_cb = [_context.device newCommandBuffer];
	metal_cb.label = [NSString stringWithFormat:@"Gpu_Command_Buffer %llu", cb_handle.handle];
	
	id<MTL4CommandAllocator> metal_command_allocator = [_context.device newCommandAllocator];
	
	MTL4ArgumentTableDescriptor* argument_table_desc = [MTL4ArgumentTableDescriptor new];
	argument_table_desc.maxBufferBindCount = 1;
	argument_table_desc.maxTextureBindCount = 0;
	argument_table_desc.maxSamplerStateBindCount = 0;
	id<MTL4ArgumentTable> argument_table = [_context.device newArgumentTableWithDescriptor:argument_table_desc error:nil];
	
	*cb_data = (Gpu_Command_Buffer_Data) {
		.cb = (__bridge_retained void*)metal_cb,
		.command_allocator = (__bridge_retained void*)metal_command_allocator,
		.argument_table = (__bridge_retained void*)argument_table,
	};
	
	[metal_cb beginCommandBufferWithAllocator:metal_command_allocator];
	
	return cb_handle;
}

static inline void gpu_end_encoder(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Data* cb_data = &gpu_pool_get_entry(&_context.pool, cb.handle)->command_buffer;
	if (cb_data->encoder != NULL) {
		id<MTL4CommandEncoder> current_encoder = (__bridge_transfer id<MTL4CommandEncoder>)cb_data->encoder;
		[current_encoder endEncoding];
		cb_data->encoder = NULL;
		cb_data->has_current_render_pass = false;
		cb_data->current_render_pipeline = (Gpu_Render_Pipeline) {};
	}
}
static inline void gpu_begin_compute_encoder(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Data* cb_data = &gpu_pool_get_entry(&_context.pool, cb.handle)->command_buffer;
	
	if ((cb_data->encoder != NULL) && [((__bridge id)cb_data->encoder) conformsToProtocol:@protocol(MTL4ComputeCommandEncoder)]) {
		// Already encoding compute.
		return;
	}
	
	gpu_end_encoder(cb);
	
	id<MTL4CommandBuffer> metal_cb = (__bridge id<MTL4CommandBuffer>)cb_data->cb;
	id<MTL4ComputeCommandEncoder> metal_compute_encoder = [metal_cb computeCommandEncoder];
	[metal_compute_encoder waitForFence:_context.fence beforeEncoderStages:(MTLStageDispatch | MTLStageBlit)];
	[metal_compute_encoder updateFence:_context.fence afterEncoderStages:(MTLStageDispatch | MTLStageBlit)];
	[metal_compute_encoder setArgumentTable:(__bridge id<MTL4ArgumentTable>)cb_data->argument_table];
	cb_data->encoder = (__bridge_retained void*)metal_compute_encoder;
}
static inline void gpu_begin_render_encoder(Gpu_Command_Buffer cb, const Gpu_Render_Pass* pass) {
	Gpu_Command_Buffer_Data* cb_data = &gpu_pool_get_entry(&_context.pool, cb.handle)->command_buffer;
	
	gpu_end_encoder(cb);
	
	MTL4RenderPassDescriptor* metal_pass_desc = [MTL4RenderPassDescriptor new];
	for (u32 i = 0; i < GPU_RENDER_PASS_MAXIMUM_COLOR_TARGETS; i++) {
		Gpu_Texture target = pass->color_targets[i];
		if (target.handle > 0) {
			Gpu_Texture_Data* texture_data = &gpu_pool_get_entry(&_context.pool, target.handle)->texture;
			metal_pass_desc.colorAttachments[i].texture = (__bridge id<MTLTexture>)texture_data->texture;
			metal_pass_desc.colorAttachments[i].loadAction = MTLLoadActionLoad;
			metal_pass_desc.colorAttachments[i].storeAction = MTLStoreActionStore;
		}
	}
	
	id<MTL4CommandBuffer> metal_cb = (__bridge id<MTL4CommandBuffer>)cb_data->cb;
	id<MTL4RenderCommandEncoder> metal_encoder = [metal_cb renderCommandEncoderWithDescriptor:metal_pass_desc];
	[metal_encoder setArgumentTable:(__bridge id<MTL4ArgumentTable>)cb_data->argument_table atStages:(MTLRenderStageVertex | MTLRenderStageFragment)];
	[metal_encoder waitForFence:_context.fence beforeEncoderStages:(MTLRenderStageVertex | MTLRenderStageFragment)];
	[metal_encoder updateFence:_context.fence afterEncoderStages:(MTLRenderStageVertex | MTLRenderStageFragment)];
	cb_data->encoder = (__bridge_retained void*)metal_encoder;
	
	cb_data->has_current_render_pass = pass;
	cb_data->current_render_pass = *pass;
	cb_data->current_render_pipeline = (Gpu_Render_Pipeline) {};
}


void gpu_submit(Gpu_Command_Buffer cb, Gpu_Queue queue) {
	Gpu_Command_Buffer_Data* cb_data = &gpu_pool_get_entry(&_context.pool, cb.handle)->command_buffer;
	Gpu_Queue_Data* queue_data = &gpu_pool_get_entry(&_context.pool, queue.handle)->queue;
	
	gpu_end_encoder(cb);
	
	id<MTL4CommandBuffer> metal_cb = (__bridge_transfer id<MTL4CommandBuffer>)cb_data->cb;
	id<MTL4CommandQueue> metal_queue = (__bridge id<MTL4CommandQueue>)queue_data->queue;
	
	[metal_cb endCommandBuffer];
	[metal_queue commit:&metal_cb count:1];
	
	(void)(__bridge_transfer id<MTL4CommandAllocator>)cb_data->command_allocator;
	(void)(__bridge_transfer id<MTL4ArgumentTable>)cb_data->argument_table;
	gpu_pool_return_handle(&_context.pool, cb.handle);
}

Gpu_Compute_Pipeline gpu_compute_pipeline_new(const char* function_name) {
	const Gpu_Compute_Pipeline pipeline_handle = {
		.handle = gpu_pool_borrow_handle(&_context.pool),
	};
	Gpu_Compute_Pipeline_Data* pipeline_data = &gpu_pool_get_entry(&_context.pool, pipeline_handle.handle)->compute_pipeline;
	
	MTL4LibraryFunctionDescriptor* function_desc = [MTL4LibraryFunctionDescriptor new];
	function_desc.name = [NSString stringWithCString:function_name encoding:NSUTF8StringEncoding];
	function_desc.library = _context.library;
	
	MTL4ComputePipelineDescriptor* pipeline_state_desc = [MTL4ComputePipelineDescriptor new];
	pipeline_state_desc.computeFunctionDescriptor = function_desc;
	
	id<MTLComputePipelineState> metal_pipeline_state = [_context.compiler newComputePipelineStateWithDescriptor:pipeline_state_desc compilerTaskOptions:nil error:nil];
	gpu_assert(metal_pipeline_state != nil, "Failed to create compute pipeline.");
	
	pipeline_data->pipeline_state = (__bridge_retained void*)metal_pipeline_state;
	
	return pipeline_handle;
}
void gpu_compute_pipeline_destroy(Gpu_Compute_Pipeline pipeline) {
	Gpu_Compute_Pipeline_Data* pipeline_data = &gpu_pool_get_entry(&_context.pool, pipeline.handle)->compute_pipeline;
	(void)(__bridge_transfer id<MTLComputePipelineState>)pipeline_data->pipeline_state;
	gpu_pool_return_handle(&_context.pool, pipeline.handle);
}

Gpu_Render_Pipeline gpu_render_pipeline_new(Gpu_Render_Pipeline_Desc desc) {
	const Gpu_Render_Pipeline pipeline_handle = {
		.handle = gpu_pool_borrow_handle(&_context.pool),
	};
	
	MTL4LibraryFunctionDescriptor* vertex_function_desc = [MTL4LibraryFunctionDescriptor new];
	vertex_function_desc.name = [NSString stringWithCString:desc.vertex_function_name encoding:NSUTF8StringEncoding];
	vertex_function_desc.library = _context.library;
	
	MTL4LibraryFunctionDescriptor* fragment_function_desc = [MTL4LibraryFunctionDescriptor new];
	fragment_function_desc.name = [NSString stringWithCString:desc.fragment_function_name encoding:NSUTF8StringEncoding];
	fragment_function_desc.library = _context.library;
	
	MTL4RenderPipelineDescriptor* metal_pipeline_desc = [MTL4RenderPipelineDescriptor new];
	metal_pipeline_desc.vertexFunctionDescriptor = vertex_function_desc;
	metal_pipeline_desc.fragmentFunctionDescriptor = fragment_function_desc;
	
	for (u32 i = 0; i < GPU_RENDER_PASS_MAXIMUM_COLOR_TARGETS; i++) {
		metal_pipeline_desc.colorAttachments[i].pixelFormat = gpu_format_get_metal_format(desc.color_targets[i].format);
	}
	
	id<MTLRenderPipelineState> metal_pipeline = [_context.compiler newRenderPipelineStateWithDescriptor:metal_pipeline_desc compilerTaskOptions:nil error:nil];
	gpu_assert(metal_pipeline != nil, "Failed to compile render pipeline.");
	
	Gpu_Render_Pipeline_Data* pipeline_data = &gpu_pool_get_entry(&_context.pool, pipeline_handle.handle)->render_pipeline;
	pipeline_data->pipeline_state = (__bridge_retained void*)metal_pipeline;
	
	return pipeline_handle;
}
void gpu_render_pipeline_destroy(Gpu_Render_Pipeline pipeline) {
	Gpu_Render_Pipeline_Data* pipeline_data = &gpu_pool_get_entry(&_context.pool, pipeline.handle)->render_pipeline;
	(void)(__bridge_transfer id<MTLRenderPipelineState>)pipeline_data->pipeline_state;
	gpu_pool_return_handle(&_context.pool, pipeline.handle);
}

static inline MTLPrimitiveType gpu_primitive_kind_get_metal_primitive_type(Gpu_Primitive_Kind kind) {
	switch (kind) {
		case Gpu_Primitive_Kind_Triangle: return MTLPrimitiveTypeTriangle;
		case Gpu_Primitive_Kind_Triangle_Strip: return MTLPrimitiveTypeTriangleStrip;
	}
}

static inline bool gpu_render_pass_equals(const Gpu_Render_Pass* a, const Gpu_Render_Pass* b) {
	// Trivially comparable
	return (memcmp(a, b, sizeof(Gpu_Render_Pass)) == 0);
}


void gpu_copy(Gpu_Command_Buffer cb, Gpu_Slice src, Gpu_Slice dst) {
	Gpu_Command_Buffer_Data* cb_data = &gpu_pool_get_entry(&_context.pool, cb.handle)->command_buffer;
	gpu_assert(src.size == dst.size, "Size of src and dst must match.");
	
	gpu_begin_compute_encoder(cb);
	
	Gpu_Heap_Data* src_heap = &gpu_pool_get_entry(&_context.pool, src.heap.handle)->heap;
	Gpu_Heap_Data* dst_heap = &gpu_pool_get_entry(&_context.pool, dst.heap.handle)->heap;
	
	id<MTLBuffer> src_buffer = (__bridge id<MTLBuffer>)src_heap->buffer;
	id<MTLBuffer> dst_buffer = (__bridge id<MTLBuffer>)dst_heap->buffer;
	id<MTL4ComputeCommandEncoder> compute_encoder = (__bridge id<MTL4ComputeCommandEncoder>)cb_data->encoder;
	[compute_encoder copyFromBuffer:src_buffer sourceOffset:src.offset toBuffer:dst_buffer destinationOffset:dst.offset size:src.size];
}

void gpu_draw(Gpu_Command_Buffer cb, const Gpu_Render_Pass* pass, Gpu_Render_Pipeline pipeline, Gpu_Slice data, Gpu_Primitive_Kind primitive_kind, u32 vertex_count, u32 instance_count) {
	Gpu_Command_Buffer_Data* cb_data = &gpu_pool_get_entry(&_context.pool, cb.handle)->command_buffer;

	// Start render encoder, if not already started.
	if (!cb_data->has_current_render_pass || !gpu_render_pass_equals(&cb_data->current_render_pass, pass)) {
		gpu_begin_render_encoder(cb, pass);
	}
	
	id<MTL4RenderCommandEncoder> metal_encoder = (__bridge id<MTL4RenderCommandEncoder>)cb_data->encoder;
	
	// Bind pipeline, if not already bound.
	if (cb_data->current_render_pipeline.handle != pipeline.handle) {
		cb_data->current_render_pipeline = pipeline;
		Gpu_Render_Pipeline_Data* pipeline_data = &gpu_pool_get_entry(&_context.pool, pipeline.handle)->render_pipeline;
		id<MTLRenderPipelineState> metal_pipeline_state = (__bridge id<MTLRenderPipelineState>)pipeline_data->pipeline_state;
		[metal_encoder setRenderPipelineState:metal_pipeline_state];
	}
	
	id<MTL4ArgumentTable> metal_argument_table = (__bridge id<MTL4ArgumentTable>)cb_data->argument_table;
	
	if (gpu_slice_is_null(data)) {
		[metal_argument_table setAddress:0 atIndex:0];
	} else {
		[metal_argument_table setAddress:gpu_slice_get_gpu_ptr(data) atIndex:0];
	}
	
	[metal_encoder drawPrimitives:gpu_primitive_kind_get_metal_primitive_type(primitive_kind) vertexStart:0 vertexCount:vertex_count instanceCount:instance_count];
}

void gpu_dispatch(Gpu_Command_Buffer cb, Gpu_Compute_Pipeline pipeline, Gpu_Slice data, vec3_u32 threads_per_threadgroup, vec3_u32 threadgroup_count) {
	Gpu_Command_Buffer_Data* cb_data = &gpu_pool_get_entry(&_context.pool, cb.handle)->command_buffer;
	gpu_begin_compute_encoder(cb);

	Gpu_Compute_Pipeline_Data* pipeline_data = &gpu_pool_get_entry(&_context.pool, pipeline.handle)->compute_pipeline;
	
	id<MTL4ComputeCommandEncoder> metal_compute_encoder = (__bridge id<MTL4ComputeCommandEncoder>)cb_data->encoder;
	id<MTLComputePipelineState> metal_pipeline_state = (__bridge id<MTLComputePipelineState>)pipeline_data->pipeline_state;
	id<MTL4ArgumentTable> metal_argument_table = (__bridge id<MTL4ArgumentTable>)cb_data->argument_table;
	
	[metal_compute_encoder setComputePipelineState:metal_pipeline_state];
	[metal_argument_table setAddress:gpu_slice_get_gpu_ptr(data) atIndex:0];
	[metal_compute_encoder dispatchThreadgroups:MTLSizeMake(threadgroup_count.x, threadgroup_count.y, threadgroup_count.z) threadsPerThreadgroup:MTLSizeMake(threads_per_threadgroup.x, threads_per_threadgroup.y, threads_per_threadgroup.z)];
}

void gpu_barrier(Gpu_Command_Buffer cb) {
	Gpu_Command_Buffer_Data* cb_data = &gpu_pool_get_entry(&_context.pool, cb.handle)->command_buffer;
	if (cb_data->encoder == NULL) {
		return;
	}
	
	id<MTL4CommandEncoder> metal_encoder = (__bridge id<MTL4CommandEncoder>)cb_data->encoder;
	
	MTLStages stages;
	if ([metal_encoder conformsToProtocol:@protocol(MTL4ComputeCommandEncoder)]) {
		stages = (MTLStageBlit | MTLStageDispatch);
	} else if ([metal_encoder conformsToProtocol:@protocol(MTL4RenderCommandEncoder)]) {
		stages = (MTLStageVertex | MTLStageFragment);
	} else {
		return;
	}
	
	[metal_encoder barrierAfterEncoderStages:stages beforeEncoderStages:stages visibilityOptions:MTL4VisibilityOptionDevice];
}

void gpu_swapchain_destroy(Gpu_Swapchain swapchain) {
	Gpu_Swapchain_Data* swapchain_data = &gpu_pool_get_entry(&_context.pool, swapchain.handle)->swapchain;
	(void)(__bridge_transfer CAMetalLayer*)swapchain_data->metal_layer;
	gpu_pool_return_handle(&_context.pool, swapchain.handle);
}
Gpu_Swapchain_Image gpu_swapchain_get_next_image(Gpu_Swapchain swapchain) {
	Gpu_Swapchain_Data* swapchain_data = &gpu_pool_get_entry(&_context.pool, swapchain.handle)->swapchain;
	CAMetalLayer* metal_layer = (__bridge CAMetalLayer*)swapchain_data->metal_layer;
	id<CAMetalDrawable> metal_drawable = [metal_layer nextDrawable];
	
	const Gpu_Swapchain_Image image_handle = {
		.handle = gpu_pool_borrow_handle(&_context.pool),
	};
	Gpu_Swapchain_Image_Data* image_data = &gpu_pool_get_entry(&_context.pool, image_handle.handle)->swapchain_image;
	image_data->drawable = (__bridge_retained void*)metal_drawable;
	image_data->texture = gpu_texture_new_from_metal_texture((__bridge void*)metal_drawable.texture);
	
	return image_handle;
}
Gpu_Texture gpu_swapchain_image_get_texture(Gpu_Swapchain_Image swapchain_image) {
	Gpu_Swapchain_Image_Data* image_data = &gpu_pool_get_entry(&_context.pool, swapchain_image.handle)->swapchain_image;
	return image_data->texture;
}
void gpu_swapchain_image_wait(Gpu_Swapchain_Image swapchain_image, Gpu_Queue queue) {
	Gpu_Swapchain_Image_Data* image_data = &gpu_pool_get_entry(&_context.pool, swapchain_image.handle)->swapchain_image;
	Gpu_Queue_Data* queue_data = &gpu_pool_get_entry(&_context.pool, queue.handle)->queue;
	id<MTL4CommandQueue> metal_queue = (__bridge id<MTL4CommandQueue>)queue_data->queue;
	id<CAMetalDrawable> metal_drawable = (__bridge id<CAMetalDrawable>)image_data->drawable;
	[metal_queue waitForDrawable:metal_drawable];
}
void gpu_swapchain_image_signal(Gpu_Swapchain_Image swapchain_image, Gpu_Queue queue) {
	Gpu_Swapchain_Image_Data* image_data = &gpu_pool_get_entry(&_context.pool, swapchain_image.handle)->swapchain_image;
	Gpu_Queue_Data* queue_data = &gpu_pool_get_entry(&_context.pool, queue.handle)->queue;
	id<MTL4CommandQueue> metal_queue = (__bridge id<MTL4CommandQueue>)queue_data->queue;
	id<CAMetalDrawable> metal_drawable = (__bridge id<CAMetalDrawable>)image_data->drawable;
	[metal_queue signalDrawable:metal_drawable];
}
void gpu_swapchain_image_present(Gpu_Swapchain_Image swapchain_image) {
	Gpu_Swapchain_Image_Data* image_data = &gpu_pool_get_entry(&_context.pool, swapchain_image.handle)->swapchain_image;
	id<CAMetalDrawable> metal_drawable = (__bridge_transfer id<CAMetalDrawable>)image_data->drawable;
	[metal_drawable present];
	gpu_texture_destroy(image_data->texture);
	gpu_pool_return_handle(&_context.pool, swapchain_image.handle);
}

void* gpu_get_metal_device(void) {
	return (__bridge void*)_context.device;
}
Gpu_Texture gpu_texture_new_from_metal_texture(void* metal_texture) {
	id<MTLTexture> bridged_texture = (__bridge id<MTLTexture>)metal_texture;
	
	const Gpu_Texture texture_handle = {
		.handle = gpu_pool_borrow_handle(&_context.pool),
	};
	Gpu_Texture_Data* texture_data = &gpu_pool_get_entry(&_context.pool, texture_handle.handle)->texture;
	*texture_data = (Gpu_Texture_Data) {
		.texture = (void*)CFRetain(metal_texture),
		.gpu_address = bridged_texture.gpuResourceID._impl,
	};
	return texture_handle;
}
Gpu_Swapchain gpu_swapchain_new_from_metal_layer(void* metal_layer, Gpu_Swapchain_Desc swapchain_desc) {
	gpu_assert(metal_layer != NULL, "metal_layer can't be null.");
	
	CAMetalLayer* bridged_layer = (__bridge CAMetalLayer*)metal_layer;
	bridged_layer.device = _context.device;
	bridged_layer.displaySyncEnabled = swapchain_desc.vsync;
	bridged_layer.pixelFormat = gpu_format_get_metal_format(swapchain_desc.format);
	
	const Gpu_Swapchain swapchain_handle = {
		.handle = gpu_pool_borrow_handle(&_context.pool),
	};
	Gpu_Swapchain_Data* swapchain_data = &gpu_pool_get_entry(&_context.pool, swapchain_handle.handle)->swapchain;
	swapchain_data->metal_layer = (void*)CFRetain(metal_layer);
	return swapchain_handle;
}

#endif

_Pragma("clang assume_nonnull end")
