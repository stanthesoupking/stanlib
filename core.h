/// Core

#pragma once

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#define sl_inline static inline
#define sl_concat(a, b) a ## b
#define sl_min(a, b) ((a) < (b) ? (a) : (b))
#define sl_max(a, b) ((a) > (b) ? (a) : (b))
#define sl_array_count(x) (sizeof(x) / sizeof(*x))

#if defined(_MSC_VER)
#define sl_aligned_struct(alignment) __declspec(align(alignment)) struct
#define sl_align_of(type) __alignof(__typeof__(type))
#else
#define sl_aligned_struct(alignment) struct __attribute__((aligned(alignment)))
#define sl_align_of(x) alignof(typeof(x))
#endif

#define SL_PI 3.14159265359f

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
#include <stdalign.h>
#include <stdlib.h>
#include <stdio.h>

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

sl_inline void sl_assert(bool condition, const char* message) {
	if (!condition) {
		printf("sl_assert: %s\n", message);
		abort();
	}
}

sl_inline void sl_abort(const char* message) {
	printf("sl_abort: %s\n", message);
	abort();
}

sl_inline u32 sl_next_pow2_exp_u32(u32 x) {
	if (x <= 1) return 0;
#if defined(_MSC_VER)
	unsigned long index;
	_BitScanReverse(&index, x - 1);
	return (u32)(index + 1);
#else
	return (u32)(32u - __builtin_clz(x - 1));
#endif
}
sl_inline u64 sl_next_pow2_exp_u64(u64 x) {
	if (x <= 1) return 0;
#if defined(_MSC_VER)
	unsigned long index;
	_BitScanReverse64(&index, x - 1);
	return (u64)(index + 1);
#else
	return (u64)(64u - __builtin_clzll(x - 1));
#endif
}

// vec2
typedef sl_aligned_struct(2) vec2_u8 {
	u8 x, y;
} vec2_u8;
typedef sl_aligned_struct(4) vec2_u16 {
	u16 x, y;
} vec2_u16;
typedef sl_aligned_struct(8) vec2_u32 {
	u32 x, y;
} vec2_u32;
typedef sl_aligned_struct(16) vec2_u64 {
	u64 x, y;
} vec2_u64;
typedef sl_aligned_struct(2) vec2_s8 {
	s8 x, y;
} vec2_s8;
typedef sl_aligned_struct(4) vec2_s16 {
	s16 x, y;
} vec2_s16;
typedef sl_aligned_struct(8) vec2_s32 {
	s32 x, y;
} vec2_s32;
typedef sl_aligned_struct(16) vec2_s64 {
	s64 x, y;
} vec2_s64;
typedef sl_aligned_struct(8) vec2_f32 {
	f32 x, y;
} vec2_f32;
typedef sl_aligned_struct(16) vec2_f64 {
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

sl_inline vec4_u8 add_vec4_u8(vec4_u8 a, vec4_u8 b) {
	return (vec4_u8) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w,
	};
}
sl_inline vec4_u16 add_vec4_u16(vec4_u16 a, vec4_u16 b) {
	return (vec4_u16) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + a.z,
		.w = a.w + b.w,
	};
}
sl_inline vec4_u32 add_vec4_u32(vec4_u32 a, vec4_u32 b) {
	return (vec4_u32) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w,
	};
}
sl_inline vec4_u64 add_vec4_u64(vec4_u64 a, vec4_u64 b) {
	return (vec4_u64) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w,
	};
}
sl_inline vec4_s8 add_vec4_s8(vec4_s8 a, vec4_s8 b) {
	return (vec4_s8) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w,
	};
}
sl_inline vec4_s16 add_vec4_s16(vec4_s16 a, vec4_s16 b) {
	return (vec4_s16) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w,
	};
}
sl_inline vec4_s32 add_vec4_s32(vec4_s32 a, vec4_s32 b) {
	return (vec4_s32) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w,
	};
}
sl_inline vec4_s64 add_vec4_s64(vec4_s64 a, vec4_s64 b) {
	return (vec4_s64) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w,
	};
}
sl_inline vec4_f32 add_vec4_f32(vec4_f32 a, vec4_f32 b) {
	return (vec4_f32) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w,
	};
}
sl_inline vec4_f64 add_vec4_f64(vec4_f64 a, vec4_f64 b) {
	return (vec4_f64) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w,
	};
}

sl_inline vec4_u8 sub_vec4_u8(vec4_u8 a, vec4_u8 b) {
	return (vec4_u8) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w,
	};
}
sl_inline vec4_u16 sub_vec4_u16(vec4_u16 a, vec4_u16 b) {
	return (vec4_u16) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w,
	};
}
sl_inline vec4_u32 sub_vec4_u32(vec4_u32 a, vec4_u32 b) {
	return (vec4_u32) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w,
	};
}
sl_inline vec4_u64 sub_vec4_u64(vec4_u64 a, vec4_u64 b) {
	return (vec4_u64) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w,
	};
}
sl_inline vec4_s8 sub_vec4_s8(vec4_s8 a, vec4_s8 b) {
	return (vec4_s8) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w,
	};
}
sl_inline vec4_s16 sub_vec4_s16(vec4_s16 a, vec4_s16 b) {
	return (vec4_s16) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w,
	};
}
sl_inline vec4_s32 sub_vec4_s32(vec4_s32 a, vec4_s32 b) {
	return (vec4_s32) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w,
	};
}
sl_inline vec4_s64 sub_vec4_s64(vec4_s64 a, vec4_s64 b) {
	return (vec4_s64) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w,
	};
}
sl_inline vec4_f32 sub_vec4_f32(vec4_f32 a, vec4_f32 b) {
	return (vec4_f32) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w,
	};
}
sl_inline vec4_f64 sub_vec4_f64(vec4_f64 a, vec4_f64 b) {
	return (vec4_f64) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w,
	};
}

sl_inline vec4_u8 div_vec4_u8(vec4_u8 a, vec4_u8 b) {
	return (vec4_u8) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w,
	};
}
sl_inline vec4_u16 div_vec4_u16(vec4_u16 a, vec4_u16 b) {
	return (vec4_u16) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w,
	};
}
sl_inline vec4_u32 div_vec4_u32(vec4_u32 a, vec4_u32 b) {
	return (vec4_u32) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w,
	};
}
sl_inline vec4_u64 div_vec4_u64(vec4_u64 a, vec4_u64 b) {
	return (vec4_u64) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w,
	};
}
sl_inline vec4_s8 div_vec4_s8(vec4_s8 a, vec4_s8 b) {
	return (vec4_s8) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w,
	};
}
sl_inline vec4_s16 div_vec4_s16(vec4_s16 a, vec4_s16 b) {
	return (vec4_s16) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w,
	};
}
sl_inline vec4_s32 div_vec4_s32(vec4_s32 a, vec4_s32 b) {
	return (vec4_s32) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w,
	};
}
sl_inline vec4_s64 div_vec4_s64(vec4_s64 a, vec4_s64 b) {
	return (vec4_s64) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w,
	};
}
sl_inline vec4_f32 div_vec4_f32(vec4_f32 a, vec4_f32 b) {
	return (vec4_f32) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w,
	};
}
sl_inline vec4_f64 div_vec4_f64(vec4_f64 a, vec4_f64 b) {
	return (vec4_f64) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w,
	};
}

sl_inline vec4_u8 mul_vec4_u8(vec4_u8 a, vec4_u8 b) {
	return (vec4_u8) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w,
	};
}
sl_inline vec4_u16 mul_vec4_u16(vec4_u16 a, vec4_u16 b) {
	return (vec4_u16) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w,
	};
}
sl_inline vec4_u32 mul_vec4_u32(vec4_u32 a, vec4_u32 b) {
	return (vec4_u32) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w,
	};
}
sl_inline vec4_u64 mul_vec4_u64(vec4_u64 a, vec4_u64 b) {
	return (vec4_u64) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w,
	};
}
sl_inline vec4_s8 mul_vec4_s8(vec4_s8 a, vec4_s8 b) {
	return (vec4_s8) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w,
	};
}
sl_inline vec4_s16 mul_vec4_s16(vec4_s16 a, vec4_s16 b) {
	return (vec4_s16) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w,
	};
}
sl_inline vec4_s32 mul_vec4_s32(vec4_s32 a, vec4_s32 b) {
	return (vec4_s32) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w,
	};
}
sl_inline vec4_s64 mul_vec4_s64(vec4_s64 a, vec4_s64 b) {
	return (vec4_s64) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w,
	};
}
sl_inline vec4_f32 mul_vec4_f32(vec4_f32 a, vec4_f32 b) {
	return (vec4_f32) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w,
	};
}
sl_inline vec4_f64 mul_vec4_f64(vec4_f64 a, vec4_f64 b) {
	return (vec4_f64) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w,
	};
}

sl_inline vec2_f32 rotate_vec2_f32(vec2_f32 v, f32 a) {
	f32 cos_a = cosf(a);
	f32 sin_a = sinf(a);
	return (vec2_f32) {
		.x = v.x * cos_a - v.y * sin_a,
		.y = v.x * sin_a + v.y * cos_a,
	};
}
sl_inline vec2_f64 rotate_vec2_f64(vec2_f64 v, f64 a) {
	f64 cos_a = cos(a);
	f64 sin_a = sin(a);
	return (vec2_f64) {
		.x = v.x * cos_a - v.y * sin_a,
		.y = v.x * sin_a + v.y * cos_a,
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

typedef struct Mutable_Buffer {
	void* data;
	u64 size;
} Mutable_Buffer;

typedef struct Immutable_Buffer {
	const void* data;
	u64 size;
} Immutable_Buffer;

#define MUTABLE_BUFFER_NULL ((Mutable_Buffer) {0})
#define IMMUTABLE_BUFFER_NULL ((Immutable_Buffer) {0})

sl_inline bool mutable_buffer_is_null(Mutable_Buffer buf) {
	return (buf.data == NULL);
}
sl_inline bool immutable_buffer_is_null(Immutable_Buffer buf) {
	return (buf.data == NULL);
}
sl_inline Immutable_Buffer sl_immutable_buffer_from_mutable(Mutable_Buffer buf) {
	return (Immutable_Buffer) {
		.data = buf.data,
		.size = buf.size
	};
}

typedef void* (*Allocator_New_Fn)(void* ctx, u64 size, u64 alignment);
typedef void* (*Allocator_Resize_Fn)(void* ctx, void* ptr, u64 old_size, u64 new_size, u64 alignment);
typedef void (*Allocator_Free_Fn)(void* ctx, void* ptr, u64 size, u64 alignment);

typedef struct Allocator {
	void* ctx;
	Allocator_New_Fn new;
	Allocator_Resize_Fn resize;
	Allocator_Free_Fn free;
} Allocator;

#define allocator_new(allocator, ptr, count)\
	ptr = allocator->new(allocator->ctx, sizeof(*ptr) * (u64)(count), sl_align_of(*ptr))
#define allocator_resize(allocator, ptr, old_count, new_count)\
	ptr = allocator->resize(allocator->ctx, ptr, sizeof(*ptr) * (u64)(old_count), sizeof(*ptr) * (u64)(new_count), sl_align_of(*ptr))
#define allocator_free(allocator, ptr, count)\
	allocator->free(allocator->ctx, ptr, sizeof(*ptr) * (u64)(count), sl_align_of(*ptr))

static void* allocator_libc_new(void* ctx, u64 size, u64 alignment) {
	return malloc(size);
}
static void* allocator_libc_resize(void* ctx, void* ptr, u64 old_size, u64 new_size, u64 alignment) {
	return realloc(ptr, new_size);
}
static void allocator_libc_free(void* ctx, void* ptr, u64 size, u64 alignment) {
	free(ptr);
}
static Allocator allocator_libc = {
	.ctx = NULL,
	.new = allocator_libc_new,
	.resize = allocator_libc_resize,
	.free = allocator_libc_free,
};

#define sl_seq(element, type, function_prefix)\
	typedef struct type {\
		Allocator* allocator;\
		element** chunks;\
		u64 chunk_size_exp;\
		u64 chunk_count;\
		u64 element_count;\
	} type;\
	\
	sl_inline u64 sl_concat(function_prefix, _chunk_storage_for_count)(u64 chunk_count) {\
		if (chunk_count == 0) {\
			return 0;\
		}\
		return 1ULL << sl_next_pow2_exp_u64(chunk_count);\
	}\
	sl_inline void sl_concat(function_prefix, _ensure_capacity)(type* s, u64 capacity) {\
		const u64 new_chunk_count = (capacity + (1ull << s->chunk_size_exp)) >> s->chunk_size_exp;\
		if (s->chunk_count < new_chunk_count) {\
			const u64 old_storage_count = sl_concat(function_prefix, _chunk_storage_for_count)(s->chunk_count);\
			const u64 new_storage_count = sl_concat(function_prefix, _chunk_storage_for_count)(new_chunk_count);\
			if (old_storage_count != new_storage_count) {\
				allocator_resize(s->allocator, s->chunks, old_storage_count, new_storage_count);\
			}\
			\
			for (u64 chunk_idx = s->chunk_count; chunk_idx < new_chunk_count; chunk_idx++) {\
				allocator_new(s->allocator, s->chunks[chunk_idx], 1ull << s->chunk_size_exp);\
			}\
			s->chunk_count = new_chunk_count;\
		}\
	}\
	sl_inline type sl_concat(function_prefix, _new)(Allocator* allocator, u64 initial_capacity) {\
		const u64 chunk_size_exp = sl_next_pow2_exp_u64(sl_max(4096 / sizeof(type), 1));\
		type s = {\
			.allocator = allocator,\
			.chunks = NULL,\
			.chunk_size_exp = chunk_size_exp,\
			.chunk_count = 0,\
			.element_count = 0,\
		};\
		sl_concat(function_prefix, _ensure_capacity)(&s, initial_capacity);\
		return s;\
	}\
	sl_inline void sl_concat(function_prefix, _destroy)(type* s) {\
		for (u32 chunk_idx = 0; chunk_idx < s->chunk_count; chunk_idx++) {\
			allocator_free(s->allocator, s->chunks[chunk_idx], 1ull << s->chunk_size_exp);\
		}\
		const u64 storage_count = sl_concat(function_prefix, _chunk_storage_for_count)(s->chunk_count);\
		allocator_free(s->allocator, s->chunks, storage_count);\
		*s = (type) {0};\
	}\
	sl_inline element* sl_concat(function_prefix, _get_ptr)(type* s, u64 idx) {\
		sl_assert(idx < s->element_count, "Index exceeds count of sequence.");\
		return &s->chunks[idx >> s->chunk_size_exp][idx & ((1ull << s->chunk_size_exp) - 1)];\
	}\
	sl_inline element sl_concat(function_prefix, _get)(type* s, u64 idx) {\
		return *sl_concat(function_prefix, _get_ptr)(s, idx);\
	}\
	sl_inline void sl_concat(function_prefix, _push)(type* s, element e) {\
		sl_concat(function_prefix, _ensure_capacity)(s, s->element_count + 1);\
		const u64 idx = s->element_count++;\
		*sl_concat(function_prefix, _get_ptr)(s, idx) = e;\
	}\
	sl_inline element* sl_concat(function_prefix, _push_reserve)(type* s) {\
		sl_concat(function_prefix, _ensure_capacity)(s, s->element_count + 1);\
		const u64 idx = s->element_count++;\
		return sl_concat(function_prefix, _get_ptr)(s, idx);\
	}\
	sl_inline void sl_concat(function_prefix, _reserve_count)(type* s, u32 count) {\
		sl_concat(function_prefix, _ensure_capacity)(s, s->element_count + count);\
		s->element_count += count;\
	}\
	sl_inline bool sl_concat(function_prefix, _pop)(type* s, element* out_e) {\
		if (s->element_count == 0) {\
			return false;\
		}\
		*out_e = *sl_concat(function_prefix, _get_ptr)(s, --s->element_count);\
		return true;\
	}\
	sl_inline u64 sl_concat(function_prefix, _get_count)(type* s) {\
		return s->element_count;\
	}\
	sl_inline void sl_concat(function_prefix, _remove)(type* s, u64 idx) {\
		sl_assert(idx < s->element_count, "Index exceeds count of sequence.");\
		for (u64 i = idx; i < s->element_count - 1; i++) {\
			*sl_concat(function_prefix, _get_ptr)(s, i) = *sl_concat(function_prefix, _get_ptr)(s, i + 1);\
		}\
		--s->element_count;\
	}

typedef struct Seq_u8 {
	Allocator* allocator; u8** chunks; u64 chunk_size_exp; u64 chunk_count; u64 element_count;
} Seq_u8; static inline u64 seq_u8_chunk_storage_for_count(u64 chunk_count) {
	if (chunk_count == 0) {
		return 0;
	} return 1ULL << sl_next_pow2_exp_u64(chunk_count);
} static inline void seq_u8_ensure_capacity(Seq_u8* s, u64 capacity) {
	const u64 new_chunk_count = (capacity + (1ull << s->chunk_size_exp)) >> s->chunk_size_exp; if (s->chunk_count < new_chunk_count) {
		const u64 old_storage_count = seq_u8_chunk_storage_for_count(s->chunk_count); const u64 new_storage_count = seq_u8_chunk_storage_for_count(new_chunk_count); if (old_storage_count != new_storage_count) {
			s->chunks = s->allocator->resize(s->allocator->ctx, s->chunks, sizeof(*s->chunks) * (u64)(old_storage_count), sizeof(*s->chunks) * (u64)(new_storage_count), __alignof(__typeof__(*s->chunks)));
		} for (u64 chunk_idx = s->chunk_count; chunk_idx < new_chunk_count; chunk_idx++) {
			s->chunks[chunk_idx] = s->allocator->new(s->allocator->ctx, sizeof(*s->chunks[chunk_idx]) * (u64)(1ull << s->chunk_size_exp), __alignof(__typeof__(*s->chunks[chunk_idx])));
		} s->chunk_count = new_chunk_count;
	}
} static inline Seq_u8 seq_u8_new(Allocator* allocator, u64 initial_capacity) {
	const u64 chunk_size_exp = sl_next_pow2_exp_u64(((4096 / sizeof(Seq_u8)) > (1) ? (4096 / sizeof(Seq_u8)) : (1))); Seq_u8 s = { .allocator = allocator, .chunks = ((void*)0), .chunk_size_exp = chunk_size_exp, .chunk_count = 0, .element_count = 0, }; seq_u8_ensure_capacity(&s, initial_capacity); return s;
} static inline void seq_u8_destroy(Seq_u8* s) {
	for (u32 chunk_idx = 0; chunk_idx < s->chunk_count; chunk_idx++) {
		s->allocator->free(s->allocator->ctx, s->chunks[chunk_idx], sizeof(*s->chunks[chunk_idx]) * (u64)(1ull << s->chunk_size_exp), __alignof(__typeof__(*s->chunks[chunk_idx])));
	} const u64 storage_count = seq_u8_chunk_storage_for_count(s->chunk_count); s->allocator->free(s->allocator->ctx, s->chunks, sizeof(*s->chunks) * (u64)(storage_count), __alignof(__typeof__(*s->chunks))); *s = (Seq_u8){ 0 };
} static inline u8* seq_u8_get_ptr(Seq_u8* s, u64 idx) {
	sl_assert(idx < s->element_count, "Index exceeds count of sequence."); return &s->chunks[idx >> s->chunk_size_exp][idx & ((1ull << s->chunk_size_exp) - 1)];
} static inline u8 seq_u8_get(Seq_u8* s, u64 idx) {
	return *seq_u8_get_ptr(s, idx);
} static inline void seq_u8_push(Seq_u8* s, u8 e) {
	seq_u8_ensure_capacity(s, s->element_count + 1); const u64 idx = s->element_count++; *seq_u8_get_ptr(s, idx) = e;
} static inline _Bool seq_u8_pop(Seq_u8* s, u8* out_e) {
	if (s->element_count == 0) {
		return 0;
	} *out_e = *seq_u8_get_ptr(s, --s->element_count); return 1;
} static inline u64 seq_u8_get_count(Seq_u8* s) {
	return s->element_count;
} static inline void seq_u8_remove(Seq_u8* s, u64 idx) {
	sl_assert(idx < s->element_count, "Index exceeds count of sequence."); for (u64 i = idx; i < s->element_count - 1; i++) {
		*seq_u8_get_ptr(s, i) = *seq_u8_get_ptr(s, i + 1);
	} --s->element_count;
};
sl_seq(u16, Seq_u16, seq_u16);
sl_seq(u32, Seq_u32, seq_u32);
sl_seq(u64, Seq_u64, seq_u64);
sl_seq(s8, Seq_s8, seq_s8);
sl_seq(s16, Seq_s16, seq_s16);
sl_seq(s32, Seq_s32, seq_s32);
sl_seq(s64, Seq_s64, seq_s64);
sl_seq(f32, Seq_f32, seq_f32);
sl_seq(f64, Seq_f64, seq_f64);

sl_seq(vec2_u8, Seq_Vec2_u8, seq_vec2_u8);
sl_seq(vec2_u16, Seq_Vec2_u16, seq_vec2_u16);
sl_seq(vec2_u32, Seq_Vec2_u32, seq_vec2_u32);
sl_seq(vec2_u64, Seq_Vec2_u64, seq_vec2_u64);
sl_seq(vec2_s8, Seq_Vec2_s8, seq_vec2_s8);
sl_seq(vec2_s16, Seq_Vec2_s16, seq_vec2_s16);
sl_seq(vec2_s32, Seq_Vec2_s32, seq_vec2_s32);
sl_seq(vec2_s64, Seq_Vec2_s64, seq_vec2_s64);
sl_seq(vec2_f32, Seq_Vec2_f32, seq_vec2_f32);
sl_seq(vec2_f64, Seq_Vec2_f64, seq_vec2_f64);

sl_seq(vec3_u8, Seq_Vec3_u8, seq_vec3_u8);
sl_seq(vec3_u16, Seq_Vec3_u16, seq_vec3_u16);
sl_seq(vec3_u32, Seq_Vec3_u32, seq_vec3_u32);
sl_seq(vec3_u64, Seq_Vec3_u64, seq_vec3_u64);
sl_seq(vec3_s8, Seq_Vec3_s8, seq_vec3_s8);
sl_seq(vec3_s16, Seq_Vec3_s16, seq_vec3_s16);
sl_seq(vec3_s32, Seq_Vec3_s32, seq_vec3_s32);
sl_seq(vec3_s64, Seq_Vec3_s64, seq_vec3_s64);
sl_seq(vec3_f32, Seq_Vec3_f32, seq_vec3_f32);
sl_seq(vec3_f64, Seq_Vec3_f64, seq_vec3_f64);

sl_seq(vec4_u8, Seq_Vec4_u8, seq_vec4_u8);
sl_seq(vec4_u16, Seq_Vec4_u16, seq_vec4_u16);
sl_seq(vec4_u32, Seq_Vec4_u32, seq_vec4_u32);
sl_seq(vec4_u64, Seq_Vec4_u64, seq_vec4_u64);
sl_seq(vec4_s8, Seq_Vec4_s8, seq_vec4_s8);
sl_seq(vec4_s16, Seq_Vec4_s16, seq_vec4_s16);
sl_seq(vec4_s32, Seq_Vec4_s32, seq_vec4_s32);
sl_seq(vec4_s64, Seq_Vec4_s64, seq_vec4_s64);
sl_seq(vec4_f32, Seq_Vec4_f32, seq_vec4_f32);
sl_seq(vec4_f64, Seq_Vec4_f64, seq_vec4_f64);

sl_seq(mat4x4_f32, Seq_Mat4x4_f32, seq_mat4x4_f32);
sl_seq(mat4x4_f64, Seq_Mat4x4_f64, seq_mat4x4_f64);

#endif

#define u8_max 255u
#define u16_max 65535u
#define u32_max 4294967295u
#define u64_max 18446744073709551615ull
