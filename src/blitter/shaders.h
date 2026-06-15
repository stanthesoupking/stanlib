// machine generated, do not edit.
#include <stanlib/core.h>
#include <stanlib/gpu.h>

extern const u32 blit_vs_spv[];
extern const u32 blit_vs_metallib[];
static const Gpu_Shader_Blob_Desc blit_vs_blob = {
	.spv = (Immutable_Buffer) { .data = blit_vs_spv, .size = 1980 },
	.metallib = (Immutable_Buffer) { .data = blit_vs_metallib, .size = 4016 },
};

extern const u32 blit_fs_spv[];
extern const u32 blit_fs_metallib[];
static const Gpu_Shader_Blob_Desc blit_fs_blob = {
	.spv = (Immutable_Buffer) { .data = blit_fs_spv, .size = 1128 },
	.metallib = (Immutable_Buffer) { .data = blit_fs_metallib, .size = 4940 },
};

