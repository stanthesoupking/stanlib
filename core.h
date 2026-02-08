/// Core

#pragma once

#define sl_aligned_struct(alignment) struct __attribute__((aligned(alignment)))
#define sl_inline static inline

#ifdef __METAL_VERSION__
typedef uchar u8;
typedef ushort u16;
typedef uint u32;
typedef ulong u64;
typedef char s8;
typedef short s16;
typedef int s32;
typedef long s64;
typedef float f32;

// vec2
typedef uchar2 vec2_u8;
typedef ushort2 vec2_u16;
typedef uint2 vec2_u32;
typedef ulong2 vec2_u64;
typedef char2 vec2_s8;
typedef short2 vec2_s16;
typedef int2 vec2_s32;
typedef long2 vec2_s64;
typedef float2 vec2_f32;

// vec3
typedef uchar3 vec3_u8;
typedef ushort3 vec3_u16;
typedef uint3 vec3_u32;
typedef ulong3 vec3_u64;
typedef char3 vec3_s8;
typedef short3 vec3_s16;
typedef int3 vec3_s32;
typedef long3 vec3_s64;
typedef float3 vec3_f32;

// vec4
typedef uchar4 vec4_u8;
typedef ushort4 vec4_u16;
typedef uint4 vec4_u32;
typedef ulong4 vec4_u64;
typedef char4 vec4_s8;
typedef short4 vec4_s16;
typedef int4 vec4_s32;
typedef long4 vec4_s64;
typedef float4 vec4_f32;

// mat4x4
typedef float4x4 mat4x4_f32;
#else
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <math.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;

// vec2
typedef sl_aligned_struct(4) vec2_u8 {
	u8 x, y;
} vec2_u8;
typedef sl_aligned_struct(8) vec2_u16 {
	u16 x, y;
} vec2_u16;
typedef sl_aligned_struct(16) vec2_u32 {
	u32 x, y;
} vec2_u32;
typedef sl_aligned_struct(32) vec2_u64 {
	u64 x, y;
} vec2_u64;
typedef sl_aligned_struct(4) vec2_s8 {
	s8 x, y;
} vec2_s8;
typedef sl_aligned_struct(8) vec2_s16 {
	s16 x, y;
} vec2_s16;
typedef sl_aligned_struct(16) vec2_s32 {
	s32 x, y;
} vec2_s32;
typedef sl_aligned_struct(32) vec2_s64 {
	s64 x, y;
} vec2_s64;
typedef sl_aligned_struct(16) vec2_f32 {
	f32 x, y;
} vec2_f32;
typedef sl_aligned_struct(32) vec2_f64 {
	f64 x, y;
} vec2_f64;

// vec3
typedef sl_aligned_struct(4) vec3_u8 {
	u8 x, y, z, _padding;
} vec3_u8;
typedef sl_aligned_struct(8) vec3_u16 {
	u16 x, y, z, _padding;
} vec3_u16;
typedef sl_aligned_struct(16) vec3_u32 {
	u32 x, y, z, _padding;
} vec3_u32;
typedef sl_aligned_struct(32) vec3_u64 {
	u64 x, y, z, _padding;
} vec3_u64;
typedef sl_aligned_struct(4) vec3_s8 {
	s8 x, y, z, _padding;
} vec3_s8;
typedef sl_aligned_struct(8) vec3_s16 {
	s16 x, y, z, _padding;
} vec3_s16;
typedef sl_aligned_struct(16) vec3_s32 {
	s32 x, y, z, _padding;
} vec3_s32;
typedef sl_aligned_struct(32) vec3_s64 {
	s64 x, y, z, _padding;
} vec3_s64;
typedef sl_aligned_struct(16) vec3_f32 {
	f32 x, y, z, _padding;
} vec3_f32;
typedef sl_aligned_struct(32) vec3_f64 {
	f64 x, y, z, _padding;
} vec3_f64;

// vec4
typedef sl_aligned_struct(4) vec4_u8 {
	u8 x, y, z, w;
} vec4_u8;
typedef sl_aligned_struct(8) vec4_u16 {
	u16 x, y, z, w;
} vec4_u16;
typedef sl_aligned_struct(16) vec4_u32 {
	u32 x, y, z, w;
} vec4_u32;
typedef sl_aligned_struct(32) vec4_u64 {
	u64 x, y, z, w;
} vec4_u64;
typedef sl_aligned_struct(4) vec4_s8 {
	s8 x, y, z, w;
} vec4_s8;
typedef sl_aligned_struct(8) vec4_s16 {
	s16 x, y, z, w;
} vec4_s16;
typedef sl_aligned_struct(16) vec4_s32 {
	s32 x, y, z, w;
} vec4_s32;
typedef sl_aligned_struct(32) vec4_s64 {
	s64 x, y, z, w;
} vec4_s64;
typedef sl_aligned_struct(16) vec4_f32 {
	f32 x, y, z, w;
} vec4_f32;
typedef sl_aligned_struct(32) vec4_f64 {
	f64 x, y, z, w;
} vec4_f64;

// mat4x4
typedef sl_aligned_struct(16) mat4x4_f32 {
	vec4_f32 x, y, z, w;
} mat4x4_f32;
typedef sl_aligned_struct(32) mat4x4_f64 {
	vec4_f64 x, y, z, w;
} mat4x4_f64;

#define identity_mat4x4_f32 ((mat4x4_f32) {\
	.x = { 1, 0, 0, 0 },\
	.y = { 0, 1, 0, 0 },\
	.z = { 0, 0, 1, 0 },\
	.w = { 0, 0, 0, 1 },\
})
#define identity_mat4x4_f64 ((mat4x4_f64) {\
	.x = { 1, 0, 0, 0 },\
	.y = { 0, 1, 0, 0 },\
	.z = { 0, 0, 1, 0 },\
	.w = { 0, 0, 0, 1 },\
})

sl_inline vec2_u8 add_vec2_u8(vec2_u8 a, vec2_u8 b) {
	return (vec2_u8) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}
sl_inline vec2_u16 add_vec2_u16(vec2_u16 a, vec2_u16 b) {
	return (vec2_u16) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}
sl_inline vec2_u32 add_vec2_u32(vec2_u32 a, vec2_u32 b) {
	return (vec2_u32) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}
sl_inline vec2_u64 add_vec2_u64(vec2_u64 a, vec2_u64 b) {
	return (vec2_u64) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}
sl_inline vec2_s8 add_vec2_s8(vec2_s8 a, vec2_s8 b) {
	return (vec2_s8) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}
sl_inline vec2_s16 add_vec2_s16(vec2_s16 a, vec2_s16 b) {
	return (vec2_s16) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}
sl_inline vec2_s32 add_vec2_s32(vec2_s32 a, vec2_s32 b) {
	return (vec2_s32) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}
sl_inline vec2_s64 add_vec2_s64(vec2_s64 a, vec2_s64 b) {
	return (vec2_s64) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}
sl_inline vec2_f32 add_vec2_f32(vec2_f32 a, vec2_f32 b) {
	return (vec2_f32) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}
sl_inline vec2_f64 add_vec2_f64(vec2_f64 a, vec2_f64 b) {
	return (vec2_f64) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}

sl_inline vec2_u8 sub_vec2_u8(vec2_u8 a, vec2_u8 b) {
	return (vec2_u8) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}
sl_inline vec2_u16 sub_vec2_u16(vec2_u16 a, vec2_u16 b) {
	return (vec2_u16) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}
sl_inline vec2_u32 sub_vec2_u32(vec2_u32 a, vec2_u32 b) {
	return (vec2_u32) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}
sl_inline vec2_u64 sub_vec2_u64(vec2_u64 a, vec2_u64 b) {
	return (vec2_u64) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}
sl_inline vec2_s8 sub_vec2_s8(vec2_s8 a, vec2_s8 b) {
	return (vec2_s8) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}
sl_inline vec2_s16 sub_vec2_s16(vec2_s16 a, vec2_s16 b) {
	return (vec2_s16) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}
sl_inline vec2_s32 sub_vec2_s32(vec2_s32 a, vec2_s32 b) {
	return (vec2_s32) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}
sl_inline vec2_s64 sub_vec2_s64(vec2_s64 a, vec2_s64 b) {
	return (vec2_s64) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}
sl_inline vec2_f32 sub_vec2_f32(vec2_f32 a, vec2_f32 b) {
	return (vec2_f32) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}
sl_inline vec2_f64 sub_vec2_f64(vec2_f64 a, vec2_f64 b) {
	return (vec2_f64) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}

sl_inline vec2_u8 div_vec2_u8(vec2_u8 a, vec2_u8 b) {
	return (vec2_u8) {
		.x = a.x / b.x,
		.y = a.y / b.y,
	};
}
sl_inline vec2_u16 div_vec2_u16(vec2_u16 a, vec2_u16 b) {
	return (vec2_u16) {
		.x = a.x / b.x,
		.y = a.y / b.y,
	};
}
sl_inline vec2_u32 div_vec2_u32(vec2_u32 a, vec2_u32 b) {
	return (vec2_u32) {
		.x = a.x / b.x,
		.y = a.y / b.y,
	};
}
sl_inline vec2_u64 div_vec2_u64(vec2_u64 a, vec2_u64 b) {
	return (vec2_u64) {
		.x = a.x / b.x,
		.y = a.y / b.y,
	};
}
sl_inline vec2_s8 div_vec2_s8(vec2_s8 a, vec2_s8 b) {
	return (vec2_s8) {
		.x = a.x / b.x,
		.y = a.y / b.y,
	};
}
sl_inline vec2_s16 div_vec2_s16(vec2_s16 a, vec2_s16 b) {
	return (vec2_s16) {
		.x = a.x / b.x,
		.y = a.y / b.y,
	};
}
sl_inline vec2_s32 div_vec2_s32(vec2_s32 a, vec2_s32 b) {
	return (vec2_s32) {
		.x = a.x / b.x,
		.y = a.y / b.y,
	};
}
sl_inline vec2_s64 div_vec2_s64(vec2_s64 a, vec2_s64 b) {
	return (vec2_s64) {
		.x = a.x / b.x,
		.y = a.y / b.y,
	};
}
sl_inline vec2_f32 div_vec2_f32(vec2_f32 a, vec2_f32 b) {
	return (vec2_f32) {
		.x = a.x / b.x,
		.y = a.y / b.y,
	};
}
sl_inline vec2_f64 div_vec2_f64(vec2_f64 a, vec2_f64 b) {
	return (vec2_f64) {
		.x = a.x / b.x,
		.y = a.y / b.y,
	};
}

sl_inline vec2_u8 mul_vec2_u8(vec2_u8 a, vec2_u8 b) {
	return (vec2_u8) {
		.x = a.x * b.x,
		.y = a.y * b.y,
	};
}
sl_inline vec2_u16 mul_vec2_u16(vec2_u16 a, vec2_u16 b) {
	return (vec2_u16) {
		.x = a.x * b.x,
		.y = a.y * b.y,
	};
}
sl_inline vec2_u32 mul_vec2_u32(vec2_u32 a, vec2_u32 b) {
	return (vec2_u32) {
		.x = a.x * b.x,
		.y = a.y * b.y,
	};
}
sl_inline vec2_u64 mul_vec2_u64(vec2_u64 a, vec2_u64 b) {
	return (vec2_u64) {
		.x = a.x * b.x,
		.y = a.y * b.y,
	};
}
sl_inline vec2_s8 mul_vec2_s8(vec2_s8 a, vec2_s8 b) {
	return (vec2_s8) {
		.x = a.x * b.x,
		.y = a.y * b.y,
	};
}
sl_inline vec2_s16 mul_vec2_s16(vec2_s16 a, vec2_s16 b) {
	return (vec2_s16) {
		.x = a.x * b.x,
		.y = a.y * b.y,
	};
}
sl_inline vec2_s32 mul_vec2_s32(vec2_s32 a, vec2_s32 b) {
	return (vec2_s32) {
		.x = a.x * b.x,
		.y = a.y * b.y,
	};
}
sl_inline vec2_s64 mul_vec2_s64(vec2_s64 a, vec2_s64 b) {
	return (vec2_s64) {
		.x = a.x * b.x,
		.y = a.y * b.y,
	};
}
sl_inline vec2_f32 mul_vec2_f32(vec2_f32 a, vec2_f32 b) {
	return (vec2_f32) {
		.x = a.x * b.x,
		.y = a.y * b.y,
	};
}
sl_inline vec2_f64 mul_vec2_f64(vec2_f64 a, vec2_f64 b) {
	return (vec2_f64) {
		.x = a.x * b.x,
		.y = a.y * b.y,
	};
}

sl_inline vec3_u8 add_vec3_u8(vec3_u8 a, vec3_u8 b) {
	return (vec3_u8) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}
sl_inline vec3_u16 add_vec3_u16(vec3_u16 a, vec3_u16 b) {
	return (vec3_u16) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + a.z,
	};
}
sl_inline vec3_u32 add_vec3_u32(vec3_u32 a, vec3_u32 b) {
	return (vec3_u32) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}
sl_inline vec3_u64 add_vec3_u64(vec3_u64 a, vec3_u64 b) {
	return (vec3_u64) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}
sl_inline vec3_s8 add_vec3_s8(vec3_s8 a, vec3_s8 b) {
	return (vec3_s8) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}
sl_inline vec3_s16 add_vec3_s16(vec3_s16 a, vec3_s16 b) {
	return (vec3_s16) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}
sl_inline vec3_s32 add_vec3_s32(vec3_s32 a, vec3_s32 b) {
	return (vec3_s32) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}
sl_inline vec3_s64 add_vec3_s64(vec3_s64 a, vec3_s64 b) {
	return (vec3_s64) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}
sl_inline vec3_f32 add_vec3_f32(vec3_f32 a, vec3_f32 b) {
	return (vec3_f32) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}
sl_inline vec3_f64 add_vec3_f64(vec3_f64 a, vec3_f64 b) {
	return (vec3_f64) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}

sl_inline vec3_u8 sub_vec3_u8(vec3_u8 a, vec3_u8 b) {
	return (vec3_u8) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}
sl_inline vec3_u16 sub_vec3_u16(vec3_u16 a, vec3_u16 b) {
	return (vec3_u16) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}
sl_inline vec3_u32 sub_vec3_u32(vec3_u32 a, vec3_u32 b) {
	return (vec3_u32) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}
sl_inline vec3_u64 sub_vec3_u64(vec3_u64 a, vec3_u64 b) {
	return (vec3_u64) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}
sl_inline vec3_s8 sub_vec3_s8(vec3_s8 a, vec3_s8 b) {
	return (vec3_s8) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}
sl_inline vec3_s16 sub_vec3_s16(vec3_s16 a, vec3_s16 b) {
	return (vec3_s16) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}
sl_inline vec3_s32 sub_vec3_s32(vec3_s32 a, vec3_s32 b) {
	return (vec3_s32) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}
sl_inline vec3_s64 sub_vec3_s64(vec3_s64 a, vec3_s64 b) {
	return (vec3_s64) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}
sl_inline vec3_f32 sub_vec3_f32(vec3_f32 a, vec3_f32 b) {
	return (vec3_f32) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}
sl_inline vec3_f64 sub_vec3_f64(vec3_f64 a, vec3_f64 b) {
	return (vec3_f64) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}

sl_inline vec3_u8 div_vec3_u8(vec3_u8 a, vec3_u8 b) {
	return (vec3_u8) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
	};
}
sl_inline vec3_u16 div_vec3_u16(vec3_u16 a, vec3_u16 b) {
	return (vec3_u16) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
	};
}
sl_inline vec3_u32 div_vec3_u32(vec3_u32 a, vec3_u32 b) {
	return (vec3_u32) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
	};
}
sl_inline vec3_u64 div_vec3_u64(vec3_u64 a, vec3_u64 b) {
	return (vec3_u64) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
	};
}
sl_inline vec3_s8 div_vec3_s8(vec3_s8 a, vec3_s8 b) {
	return (vec3_s8) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
	};
}
sl_inline vec3_s16 div_vec3_s16(vec3_s16 a, vec3_s16 b) {
	return (vec3_s16) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
	};
}
sl_inline vec3_s32 div_vec3_s32(vec3_s32 a, vec3_s32 b) {
	return (vec3_s32) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
	};
}
sl_inline vec3_s64 div_vec3_s64(vec3_s64 a, vec3_s64 b) {
	return (vec3_s64) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
	};
}
sl_inline vec3_f32 div_vec3_f32(vec3_f32 a, vec3_f32 b) {
	return (vec3_f32) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
	};
}
sl_inline vec3_f64 div_vec3_f64(vec3_f64 a, vec3_f64 b) {
	return (vec3_f64) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
	};
}

sl_inline vec3_u8 mul_vec3_u8(vec3_u8 a, vec3_u8 b) {
	return (vec3_u8) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
	};
}
sl_inline vec3_u16 mul_vec3_u16(vec3_u16 a, vec3_u16 b) {
	return (vec3_u16) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
	};
}
sl_inline vec3_u32 mul_vec3_u32(vec3_u32 a, vec3_u32 b) {
	return (vec3_u32) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
	};
}
sl_inline vec3_u64 mul_vec3_u64(vec3_u64 a, vec3_u64 b) {
	return (vec3_u64) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
	};
}
sl_inline vec3_s8 mul_vec3_s8(vec3_s8 a, vec3_s8 b) {
	return (vec3_s8) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
	};
}
sl_inline vec3_s16 mul_vec3_s16(vec3_s16 a, vec3_s16 b) {
	return (vec3_s16) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
	};
}
sl_inline vec3_s32 mul_vec3_s32(vec3_s32 a, vec3_s32 b) {
	return (vec3_s32) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
	};
}
sl_inline vec3_s64 mul_vec3_s64(vec3_s64 a, vec3_s64 b) {
	return (vec3_s64) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
	};
}
sl_inline vec3_f32 mul_vec3_f32(vec3_f32 a, vec3_f32 b) {
	return (vec3_f32) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
	};
}
sl_inline vec3_f64 mul_vec3_f64(vec3_f64 a, vec3_f64 b) {
	return (vec3_f64) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
	};
}

sl_inline vec4_f32 mul_mat4x4_vec4_f32(mat4x4_f32 m, vec4_f32 v) {
	return (vec4_f32) {
		m.x.x*v.x + m.y.x*v.y + m.z.x*v.z + m.w.x*v.w,
		m.x.y*v.x + m.y.y*v.y + m.z.y*v.z + m.w.y*v.w,
		m.x.z*v.x + m.y.z*v.y + m.z.z*v.z + m.w.z*v.w,
		m.x.w*v.x + m.y.w*v.y + m.z.w*v.z + m.w.w*v.w
	};
}
sl_inline vec4_f64 mul_mat4x4_vec4_f64(mat4x4_f64 m, vec4_f64 v) {
	return (vec4_f64) {
		m.x.x*v.x + m.y.x*v.y + m.z.x*v.z + m.w.x*v.w,
		m.x.y*v.x + m.y.y*v.y + m.z.y*v.z + m.w.y*v.w,
		m.x.z*v.x + m.y.z*v.y + m.z.z*v.z + m.w.z*v.w,
		m.x.w*v.x + m.y.w*v.y + m.z.w*v.z + m.w.w*v.w
	};
}

sl_inline mat4x4_f32 mul_mat4x4_f32(mat4x4_f32 a, mat4x4_f32 b) {
	return (mat4x4_f32) {
		mul_mat4x4_vec4_f32(a, b.x),
		mul_mat4x4_vec4_f32(a, b.y),
		mul_mat4x4_vec4_f32(a, b.z),
		mul_mat4x4_vec4_f32(a, b.w),
	};
}
sl_inline mat4x4_f64 mul_mat4x4_f64(mat4x4_f64 a, mat4x4_f64 b) {
	return (mat4x4_f64) {
		mul_mat4x4_vec4_f64(a, b.x),
		mul_mat4x4_vec4_f64(a, b.y),
		mul_mat4x4_vec4_f64(a, b.z),
		mul_mat4x4_vec4_f64(a, b.w),
	};
}

sl_inline mat4x4_f32 invert_mat4x4_f32(mat4x4_f32 m) {
	f32 a00 = m.x.x, a01 = m.y.x, a02 = m.z.x, a03 = m.w.x;
	f32 a10 = m.x.y, a11 = m.y.y, a12 = m.z.y, a13 = m.w.y;
	f32 a20 = m.x.z, a21 = m.y.z, a22 = m.z.z, a23 = m.w.z;
	f32 a30 = m.x.w, a31 = m.y.w, a32 = m.z.w, a33 = m.w.w;

	f32 b00 = a00*a11 - a01*a10;
	f32 b01 = a00*a12 - a02*a10;
	f32 b02 = a00*a13 - a03*a10;
	f32 b03 = a01*a12 - a02*a11;
	f32 b04 = a01*a13 - a03*a11;
	f32 b05 = a02*a13 - a03*a12;
	f32 b06 = a20*a31 - a21*a30;
	f32 b07 = a20*a32 - a22*a30;
	f32 b08 = a20*a33 - a23*a30;
	f32 b09 = a21*a32 - a22*a31;
	f32 b10 = a21*a33 - a23*a31;
	f32 b11 = a22*a33 - a23*a32;

	f32 det =
		b00*b11 - b01*b10 + b02*b09 +
		b03*b08 - b04*b07 + b05*b06;

	f32 inv_det = 1.0f / det;

	return (mat4x4_f32){
		(vec4_f32) {
			( a11*b11 - a12*b10 + a13*b09) * inv_det,
			(-a10*b11 + a12*b08 - a13*b07) * inv_det,
			( a31*b05 - a32*b04 + a33*b03) * inv_det,
			(-a30*b05 + a32*b02 - a33*b01) * inv_det
		},
		(vec4_f32) {
			(-a01*b11 + a02*b10 - a03*b09) * inv_det,
			( a00*b11 - a02*b08 + a03*b07) * inv_det,
			(-a21*b05 + a22*b04 - a23*b03) * inv_det,
			( a20*b05 - a22*b02 + a23*b01) * inv_det
		},
		(vec4_f32) {
			( a01*b10 - a02*b09 + a03*b08) * inv_det,
			(-a00*b10 + a02*b06 - a03*b06) * inv_det,
			( a21*b04 - a22*b03 + a23*b02) * inv_det,
			(-a20*b04 + a22*b01 - a23*b00) * inv_det
		},
		(vec4_f32) {
			(-a01*b09 + a02*b08 - a03*b07) * inv_det,
			( a00*b09 - a02*b07 + a03*b06) * inv_det,
			(-a21*b03 + a22*b01 - a23*b00) * inv_det,
			( a20*b03 - a22*b01 + a23*b00) * inv_det
		},
	};
}
sl_inline mat4x4_f64 invert_mat4x4_f64(mat4x4_f64 m) {
	f64 a00 = m.x.x, a01 = m.y.x, a02 = m.z.x, a03 = m.w.x;
	f64 a10 = m.x.y, a11 = m.y.y, a12 = m.z.y, a13 = m.w.y;
	f64 a20 = m.x.z, a21 = m.y.z, a22 = m.z.z, a23 = m.w.z;
	f64 a30 = m.x.w, a31 = m.y.w, a32 = m.z.w, a33 = m.w.w;

	f64 b00 = a00*a11 - a01*a10;
	f64 b01 = a00*a12 - a02*a10;
	f64 b02 = a00*a13 - a03*a10;
	f64 b03 = a01*a12 - a02*a11;
	f64 b04 = a01*a13 - a03*a11;
	f64 b05 = a02*a13 - a03*a12;
	f64 b06 = a20*a31 - a21*a30;
	f64 b07 = a20*a32 - a22*a30;
	f64 b08 = a20*a33 - a23*a30;
	f64 b09 = a21*a32 - a22*a31;
	f64 b10 = a21*a33 - a23*a31;
	f64 b11 = a22*a33 - a23*a32;

	f64 det =
		b00*b11 - b01*b10 + b02*b09 +
		b03*b08 - b04*b07 + b05*b06;

	f64 inv_det = 1.0 / det;

	return (mat4x4_f64) {
		(vec4_f64) {
			( a11*b11 - a12*b10 + a13*b09) * inv_det,
			(-a10*b11 + a12*b08 - a13*b07) * inv_det,
			( a31*b05 - a32*b04 + a33*b03) * inv_det,
			(-a30*b05 + a32*b02 - a33*b01) * inv_det
		},
		(vec4_f64) {
			(-a01*b11 + a02*b10 - a03*b09) * inv_det,
			( a00*b11 - a02*b08 + a03*b07) * inv_det,
			(-a21*b05 + a22*b04 - a23*b03) * inv_det,
			( a20*b05 - a22*b02 + a23*b01) * inv_det
		},
		(vec4_f64) {
			( a01*b10 - a02*b09 + a03*b08) * inv_det,
			(-a00*b10 + a02*b06 - a03*b06) * inv_det,
			( a21*b04 - a22*b03 + a23*b02) * inv_det,
			(-a20*b04 + a22*b01 - a23*b00) * inv_det
		},
		(vec4_f64) {
			(-a01*b09 + a02*b08 - a03*b07) * inv_det,
			( a00*b09 - a02*b07 + a03*b06) * inv_det,
			(-a21*b03 + a22*b01 - a23*b00) * inv_det,
			( a20*b03 - a22*b01 + a23*b00) * inv_det
		},
	};
}

sl_inline mat4x4_f32 ortho_mat4x4_f32(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
	f32 rl = right - left;
	f32 tb = top - bottom;
	f32 fn = far - near;
	return (mat4x4_f32) {
		.x = {
			2.0f/rl, 0, 0, 0,
		},
		.y = {
			0, 2.0f/tb, 0, 0,
		},
		.z = {
			0, 0, 1.0f/fn, 0,
		},
		.w = {
			-(right+left)/rl,
			-(top+bottom)/tb,
			-near/fn,
			1
		},
	};
}
sl_inline mat4x4_f64 ortho_mat4x4_f64(f64 left, f64 right, f64 bottom, f64 top, f64 near, f64 far) {
	f64 rl = right - left;
	f64 tb = top - bottom;
	f64 fn = far - near;
	return (mat4x4_f64) {
		.x = {
			2.0/rl, 0, 0, 0,
		},
		.y = {
			0, 2.0/tb, 0, 0,
		},
		.z = {
			0, 0, 1.0/fn, 0,
		},
		.w = {
			-(right+left)/rl,
			-(top+bottom)/tb,
			-near/fn,
			1
		},
	};
}

sl_inline mat4x4_f32 perspective_mat4x4_f32(f32 fovy_radians, f32 aspect, f32 near, f32 far) {
	f32 f = 1.0f / tanf(fovy_radians * 0.5f);
	f32 fn = far - near;

	return (mat4x4_f32) {
		.x = { f/aspect, 0, 0, 0 },
		.y = { 0, f, 0, 0 },
		.z = { 0, 0, far/fn, 1 },
		.w = { 0, 0, -near*far/fn, 0 },
	};
}
sl_inline mat4x4_f64 perspective_mat4x4_f64(f64 fovy_radians, f64 aspect, f64 near, f64 far) {
	f64 f = 1.0 / tan(fovy_radians * 0.5);
	f64 fn = far - near;

	return (mat4x4_f64) {
		.x = { f/aspect, 0, 0, 0 },
		.y = { 0, f, 0, 0 },
		.z = { 0, 0, far/fn, 1 },
		.w = { 0, 0, -near*far/fn, 0 },
	};
}

sl_inline mat4x4_f32 translate_mat4x4_f32(vec3_f32 translation) {
	mat4x4_f32 m = identity_mat4x4_f32;
	m.w.x = translation.x;
	m.w.y = translation.y;
	m.w.z = translation.z;
	return m;
}
sl_inline mat4x4_f64 translate_mat4x4_f64(vec3_f64 translation) {
	mat4x4_f64 m = identity_mat4x4_f64;
	m.w.x = translation.x;
	m.w.y = translation.y;
	m.w.z = translation.z;
	return m;
}

sl_inline mat4x4_f32 scale_mat4x4_f32(vec3_f32 scale) {
	return (mat4x4_f32) {
		.x = { scale.x, 0, 0, 0 },
		.y = { 0, scale.y, 0, 0 },
		.z = { 0, 0, scale.z, 0 },
		.w = { 0, 0, 0, 1 },
	};
}
sl_inline mat4x4_f64 scale_mat4x4_f64(vec3_f64 scale) {
	return (mat4x4_f64) {
		.x = { scale.x, 0, 0, 0 },
		.y = { 0, scale.y, 0, 0 },
		.z = { 0, 0, scale.z, 0 },
		.w = { 0, 0, 0, 1 },
	};
}

sl_inline mat4x4_f32 rotate_x_mat4x4_f32(f32 angle) {
	f32 c = cosf(angle);
	f32 s = sinf(angle);
	return (mat4x4_f32) {
		.x = { 1, 0, 0, 0 },
		.y = { 0, c, s, 0 },
		.z = { 0, -s, c, 0 },
		.w = { 0, 0, 0, 1 },
	};
}
sl_inline mat4x4_f64 rotate_x_mat4x4_f64(f64 angle) {
	f64 c = cos(angle);
	f64 s = sin(angle);
	return (mat4x4_f64) {
		.x = { 1, 0, 0, 0 },
		.y = { 0, c, s, 0 },
		.z = { 0, -s, c, 0 },
		.w = { 0, 0, 0, 1 },
	};
}

sl_inline mat4x4_f32 rotate_y_mat4x4_f32(f32 angle) {
	f32 c = cosf(angle);
	f32 s = sinf(angle);
	return (mat4x4_f32) {
		.x = { c, 0, -s, 0 },
		.y = { 0, 1, 0, 0 },
		.z = { s, 0, c, 0 },
		.w = { 0, 0, 0, 1 },
	};
}
sl_inline mat4x4_f64 rotate_y_mat4x4_f64(f64 angle) {
	f64 c = cos(angle);
	f64 s = sin(angle);
	return (mat4x4_f64) {
		.x = { c, 0, -s, 0 },
		.y = { 0, 1, 0, 0 },
		.z = { s, 0, c, 0 },
		.w = { 0, 0, 0, 1 },
	};
}

sl_inline mat4x4_f32 rotate_z_mat4x4_f32(f32 angle) {
	f32 c = cosf(angle);
	f32 s = sinf(angle);
	return (mat4x4_f32){
		.x = { c, s, 0, 0 },
		.y = { -s, c, 0, 0 },
		.z = { 0, 0, 1, 0 },
		.w = { 0, 0, 0, 1 },
	};
}
sl_inline mat4x4_f64 rotate_z_mat4x4_f64(f64 angle) {
	f64 c = cos(angle);
	f64 s = sin(angle);
	return (mat4x4_f64){
		.x = { c, s, 0, 0 },
		.y = { -s, c, 0, 0 },
		.z = { 0, 0, 1, 0 },
		.w = { 0, 0, 0, 1 },
	};
}

#endif

#define u8_max 255u
#define u16_max 65535u
#define u32_max 4294967295u
#define u64_max 18446744073709551615ull
