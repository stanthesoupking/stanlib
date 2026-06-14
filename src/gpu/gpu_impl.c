#include <stanlib/gpu.h>

#if !defined(GPU_BACKEND)
#define GPU_BACKEND GPU_BACKEND_DEFAULT
#endif

#include "gpu_impl_common.c"

#if GPU_BACKEND == GPU_BACKEND_VULKAN
#include "gpu_impl_vk.c"
#elif GPU_BACKEND == GPU_BACKEND_METAL
#include "gpu_impl_metal.m"
#else
#error Unknown backend selected.
#endif
