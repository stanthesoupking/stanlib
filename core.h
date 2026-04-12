/// Core

#pragma once

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#define sl_inline static inline
#define sl_concat(a, b) a ## b
#define sl_min(a, b) ((a) < (b) ? (a) : (b))
#define sl_max(a, b) ((a) > (b) ? (a) : (b))
#define sl_clamp(v, min_v, max_v) (sl_min(sl_max(v, min_v), max_v))
#define sl_array_count(x) (sizeof(x) / sizeof(*x))

#define sl_lru_make_newest(entry, lru) do { \
	(entry)->lru_newer = NULL; \
	if ((lru)->newest != NULL) { \
		(lru)->newest->lru_newer = (entry); \
		(entry)->lru_older = (lru)->newest; \
	} else { \
		(entry)->lru_older = NULL; \
	} \
	(lru)->newest = (entry); \
	if ((lru)->oldest == NULL) { \
		(lru)->oldest = (entry); \
	} \
} while(0)

#define sl_lru_remove(entry, lru) do { \
	if ((entry)->lru_older != NULL) { \
		(entry)->lru_older->lru_newer = (entry)->lru_newer; \
	} else { \
		(lru)->oldest = (entry)->lru_newer; \
	} \
	if ((entry)->lru_newer != NULL) { \
		(entry)->lru_newer->lru_older = (entry)->lru_older; \
	} else { \
		(lru)->newest = (entry)->lru_older; \
	} \
	(entry)->lru_newer = NULL; \
	(entry)->lru_older = NULL; \
} while(0)

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

sl_inline vec2_u8 splat_vec2_u8(u8 v) {
	return (vec2_u8) {
		.x = v,
		.y = v,
	};
}
sl_inline vec2_u16 splat_vec2_u16(u16 v) {
	return (vec2_u16) {
		.x = v,
		.y = v,
	};
}
sl_inline vec2_u32 splat_vec2_u32(u32 v) {
	return (vec2_u32) {
		.x = v,
		.y = v,
	};
}
sl_inline vec2_u64 splat_vec2_u64(u64 v) {
	return (vec2_u64) {
		.x = v,
		.y = v,
	};
}
sl_inline vec2_s8 splat_vec2_s8(s8 v) {
	return (vec2_s8) {
		.x = v,
		.y = v,
	};
}
sl_inline vec2_s16 splat_vec2_s16(s16 v) {
	return (vec2_s16) {
		.x = v,
		.y = v,
	};
}
sl_inline vec2_s32 splat_vec2_s32(s32 v) {
	return (vec2_s32) {
		.x = v,
		.y = v,
	};
}
sl_inline vec2_s64 splat_vec2_s64(s64 v) {
	return (vec2_s64) {
		.x = v,
		.y = v,
	};
}
sl_inline vec2_f32 splat_vec2_f32(f32 v) {
	return (vec2_f32) {
		.x = v,
		.y = v,
	};
}
sl_inline vec2_f64 splat_vec2_f64(f64 v) {
	return (vec2_f64) {
		.x = v,
		.y = v,
	};
}

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

sl_inline vec2_s8 neg_vec2_s8(vec2_s8 a) {
	return (vec2_s8) {
		.x = -a.x,
		.y = -a.y,
	};
}
sl_inline vec2_s16 neg_vec2_s16(vec2_s16 a) {
	return (vec2_s16) {
		.x = -a.x,
		.y = -a.y,
	};
}
sl_inline vec2_s32 neg_vec2_s32(vec2_s32 a) {
	return (vec2_s32) {
		.x = -a.x,
		.y = -a.y,
	};
}
sl_inline vec2_s64 neg_vec2_s64(vec2_s64 a) {
	return (vec2_s64) {
		.x = -a.x,
		.y = -a.y,
	};
}
sl_inline vec2_f32 neg_vec2_f32(vec2_f32 a) {
	return (vec2_f32) {
		.x = -a.x ,
		.y = -a.y,
	};
}
sl_inline vec2_f64 neg_vec2_f64(vec2_f64 a) {
	return (vec2_f64) {
		.x = -a.x,
		.y = -a.y,
	};
}

sl_inline vec2_u8 min_vec2_u8(vec2_u8 a, vec2_u8 b) {
	return (vec2_u8) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
	};
}
sl_inline vec2_u16 min_vec2_u16(vec2_u16 a, vec2_u16 b) {
	return (vec2_u16) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
	};
}
sl_inline vec2_u32 min_vec2_u32(vec2_u32 a, vec2_u32 b) {
	return (vec2_u32) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
	};
}
sl_inline vec2_u64 min_vec2_u64(vec2_u64 a, vec2_u64 b) {
	return (vec2_u64) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
	};
}
sl_inline vec2_s8 min_vec2_s8(vec2_s8 a, vec2_s8 b) {
	return (vec2_s8) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
	};
}
sl_inline vec2_s16 min_vec2_s16(vec2_s16 a, vec2_s16 b) {
	return (vec2_s16) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
	};
}
sl_inline vec2_s32 min_vec2_s32(vec2_s32 a, vec2_s32 b) {
	return (vec2_s32) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
	};
}
sl_inline vec2_s64 min_vec2_s64(vec2_s64 a, vec2_s64 b) {
	return (vec2_s64) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
	};
}
sl_inline vec2_f32 min_vec2_f32(vec2_f32 a, vec2_f32 b) {
	return (vec2_f32) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
	};
}
sl_inline vec2_f64 min_vec2_f64(vec2_f64 a, vec2_f64 b) {
	return (vec2_f64) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
	};
}
sl_inline vec2_u8 max_vec2_u8(vec2_u8 a, vec2_u8 b) {
	return (vec2_u8) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
	};
}
sl_inline vec2_u16 max_vec2_u16(vec2_u16 a, vec2_u16 b) {
	return (vec2_u16) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
	};
}
sl_inline vec2_u32 max_vec2_u32(vec2_u32 a, vec2_u32 b) {
	return (vec2_u32) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
	};
}
sl_inline vec2_u64 max_vec2_u64(vec2_u64 a, vec2_u64 b) {
	return (vec2_u64) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
	};
}
sl_inline vec2_s8 max_vec2_s8(vec2_s8 a, vec2_s8 b) {
	return (vec2_s8) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
	};
}
sl_inline vec2_s16 max_vec2_s16(vec2_s16 a, vec2_s16 b) {
	return (vec2_s16) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
	};
}
sl_inline vec2_s32 max_vec2_s32(vec2_s32 a, vec2_s32 b) {
	return (vec2_s32) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
	};
}
sl_inline vec2_s64 max_vec2_s64(vec2_s64 a, vec2_s64 b) {
	return (vec2_s64) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
	};
}
sl_inline vec2_f32 max_vec2_f32(vec2_f32 a, vec2_f32 b) {
	return (vec2_f32) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
	};
}
sl_inline vec2_f64 max_vec2_f64(vec2_f64 a, vec2_f64 b) {
	return (vec2_f64) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
	};
}

sl_inline vec3_u8 splat_vec3_u8(u8 v) {
	return (vec3_u8) {
		.x = v,
		.y = v,
		.z = v,
	};
}
sl_inline vec3_u16 splat_vec3_u16(u16 v) {
	return (vec3_u16) {
		.x = v,
		.y = v,
		.z = v,
	};
}
sl_inline vec3_u32 splat_vec3_u32(u32 v) {
	return (vec3_u32) {
		.x = v,
		.y = v,
		.z = v,
	};
}
sl_inline vec3_u64 splat_vec3_u64(u64 v) {
	return (vec3_u64) {
		.x = v,
		.y = v,
		.z = v,
	};
}
sl_inline vec3_s8 splat_vec3_s8(s8 v) {
	return (vec3_s8) {
		.x = v,
		.y = v,
		.z = v,
	};
}
sl_inline vec3_s16 splat_vec3_s16(s16 v) {
	return (vec3_s16) {
		.x = v,
		.y = v,
		.z = v,
	};
}
sl_inline vec3_s32 splat_vec3_s32(s32 v) {
	return (vec3_s32) {
		.x = v,
		.y = v,
		.z = v,
	};
}
sl_inline vec3_s64 splat_vec3_s64(s64 v) {
	return (vec3_s64) {
		.x = v,
		.y = v,
		.z = v,
	};
}
sl_inline vec3_f32 splat_vec3_f32(f32 v) {
	return (vec3_f32) {
		.x = v,
		.y = v,
		.z = v,
	};
}
sl_inline vec3_f64 splat_vec3_f64(f64 v) {
	return (vec3_f64) {
		.x = v,
		.y = v,
		.z = v,
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

sl_inline vec3_s8 neg_vec3_s8(vec3_s8 a) {
	return (vec3_s8) {
		.x = -a.x,
		.y = -a.y,
		.z = -a.z,
	};
}
sl_inline vec3_s16 neg_vec3_s16(vec3_s16 a) {
	return (vec3_s16) {
		.x = -a.x,
		.y = -a.y,
		.z = -a.z,
	};
}
sl_inline vec3_s32 neg_vec3_s32(vec3_s32 a) {
	return (vec3_s32) {
		.x = -a.x,
		.y = -a.y,
		.z = -a.z,
	};
}
sl_inline vec3_s64 neg_vec_s64(vec3_s64 a) {
	return (vec3_s64) {
		.x = -a.x,
		.y = -a.y,
		.z = -a.z,
	};
}
sl_inline vec3_f32 neg_vec3_f32(vec3_f32 a) {
	return (vec3_f32) {
		.x = -a.x ,
		.y = -a.y,
		.z = -a.z,
	};
}
sl_inline vec3_f64 neg_vec3_f64(vec3_f64 a) {
	return (vec3_f64) {
		.x = -a.x,
		.y = -a.y,
		.z = -a.z,
	};
}

sl_inline vec3_u8 min_vec3_u8(vec3_u8 a, vec3_u8 b) {
	return (vec3_u8) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
		.z = sl_min(a.z, b.z),
	};
}
sl_inline vec3_u16 min_vec3_u16(vec3_u16 a, vec3_u16 b) {
	return (vec3_u16) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
		.z = sl_min(a.z, b.z),
	};
}
sl_inline vec3_u32 min_vec3_u32(vec3_u32 a, vec3_u32 b) {
	return (vec3_u32) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
		.z = sl_min(a.z, b.z),
	};
}
sl_inline vec3_u64 min_vec3_u64(vec3_u64 a, vec3_u64 b) {
	return (vec3_u64) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
		.z = sl_min(a.z, b.z),
	};
}
sl_inline vec3_s8 min_vec3_s8(vec3_s8 a, vec3_s8 b) {
	return (vec3_s8) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
		.z = sl_min(a.z, b.z),
	};
}
sl_inline vec3_s16 min_vec3_s16(vec3_s16 a, vec3_s16 b) {
	return (vec3_s16) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
		.z = sl_min(a.z, b.z),
	};
}
sl_inline vec3_s32 min_vec3_s32(vec3_s32 a, vec3_s32 b) {
	return (vec3_s32) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
		.z = sl_min(a.z, b.z),
	};
}
sl_inline vec3_s64 min_vec3_s64(vec3_s64 a, vec3_s64 b) {
	return (vec3_s64) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
		.z = sl_min(a.z, b.z),
	};
}
sl_inline vec3_f32 min_vec3_f32(vec3_f32 a, vec3_f32 b) {
	return (vec3_f32) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
		.z = sl_min(a.z, b.z),
	};
}
sl_inline vec3_f64 min_vec3_f64(vec3_f64 a, vec3_f64 b) {
	return (vec3_f64) {
		.x = sl_min(a.x, b.x),
		.y = sl_min(a.y, b.y),
		.z = sl_min(a.z, b.z),
	};
}
sl_inline vec3_u8 max_vec3_u8(vec3_u8 a, vec3_u8 b) {
	return (vec3_u8) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
		.z = sl_max(a.z, b.z),
	};
}
sl_inline vec3_u16 max_vec3_u16(vec3_u16 a, vec3_u16 b) {
	return (vec3_u16) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
		.z = sl_max(a.z, b.z),
	};
}
sl_inline vec3_u32 max_vec3_u32(vec3_u32 a, vec3_u32 b) {
	return (vec3_u32) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
		.z = sl_max(a.z, b.z),
	};
}
sl_inline vec3_u64 max_vec3_u64(vec3_u64 a, vec3_u64 b) {
	return (vec3_u64) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
		.z = sl_max(a.z, b.z),
	};
}
sl_inline vec3_s8 max_vec3_s8(vec3_s8 a, vec3_s8 b) {
	return (vec3_s8) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
		.z = sl_max(a.z, b.z),
	};
}
sl_inline vec3_s16 max_vec3_s16(vec3_s16 a, vec3_s16 b) {
	return (vec3_s16) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
		.z = sl_max(a.z, b.z),
	};
}
sl_inline vec3_s32 max_vec3_s32(vec3_s32 a, vec3_s32 b) {
	return (vec3_s32) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
		.z = sl_max(a.z, b.z),
	};
}
sl_inline vec3_s64 max_vec3_s64(vec3_s64 a, vec3_s64 b) {
	return (vec3_s64) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
		.z = sl_max(a.z, b.z),
	};
}
sl_inline vec3_f32 max_vec3_f32(vec3_f32 a, vec3_f32 b) {
	return (vec3_f32) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
		.z = sl_max(a.z, b.z),
	};
}
sl_inline vec3_f64 max_vec3_f64(vec3_f64 a, vec3_f64 b) {
	return (vec3_f64) {
		.x = sl_max(a.x, b.x),
		.y = sl_max(a.y, b.y),
		.z = sl_max(a.z, b.z),
	};
}

sl_inline bool eq_vec3_u8(vec4_u8 a, vec4_u8 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
sl_inline bool eq_vec3_u16(vec4_u16 a, vec4_u16 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
sl_inline bool eq_vec3_u32(vec4_u32 a, vec4_u32 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
sl_inline bool eq_vec3_u64(vec3_u64 a, vec3_u64 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
sl_inline bool eq_vec3_s8(vec3_s8 a, vec3_s8 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
sl_inline bool eq_vec3_s16(vec3_s16 a, vec3_s16 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
sl_inline bool eq_vec3_s32(vec3_s32 a, vec3_s32 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
sl_inline bool eq_vec3_s64(vec3_s64 a, vec3_s64 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
sl_inline bool eq_vec3_f32(vec3_f32 a, vec3_f32 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
sl_inline bool eq_vec3_f64(vec3_f64 a, vec3_f64 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

sl_inline vec4_u8 splat_vec4_u8(u8 v) {
	return (vec4_u8) {
		.x = v,
		.y = v,
		.z = v,
		.w = v,
	};
}
sl_inline vec4_u16 splat_vec4_u16(u16 v) {
	return (vec4_u16) {
		.x = v,
		.y = v,
		.z = v,
		.w = v,
	};
}
sl_inline vec4_u32 splat_vec4_u32(u32 v) {
	return (vec4_u32) {
		.x = v,
		.y = v,
		.z = v,
		.w = v,
	};
}
sl_inline vec4_u64 splat_vec4_u64(u64 v) {
	return (vec4_u64) {
		.x = v,
		.y = v,
		.z = v,
		.w = v,
	};
}
sl_inline vec4_s8 splat_vec4_s8(s8 v) {
	return (vec4_s8) {
		.x = v,
		.y = v,
		.z = v,
		.w = v,
	};
}
sl_inline vec4_s16 splat_vec4_s16(s16 v) {
	return (vec4_s16) {
		.x = v,
		.y = v,
		.z = v,
		.w = v,
	};
}
sl_inline vec4_s32 splat_vec4_s32(s32 v) {
	return (vec4_s32) {
		.x = v,
		.y = v,
		.z = v,
		.w = v,
	};
}
sl_inline vec4_s64 splat_vec4_s64(s64 v) {
	return (vec4_s64) {
		.x = v,
		.y = v,
		.z = v,
		.w = v,
	};
}
sl_inline vec4_f32 splat_vec4_f32(f32 v) {
	return (vec4_f32) {
		.x = v,
		.y = v,
		.z = v,
		.w = v,
	};
}
sl_inline vec4_f64 splat_vec4_f64(f64 v) {
	return (vec4_f64) {
		.x = v,
		.y = v,
		.z = v,
		.w = v,
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

sl_inline vec4_s8 neg_vec4_s8(vec4_s8 a) {
	return (vec4_s8) {
		.x = -a.x,
		.y = -a.y,
		.z = -a.z,
		.w = -a.w,
	};
}
sl_inline vec4_s16 neg_vec4_s16(vec4_s16 a) {
	return (vec4_s16) {
		.x = -a.x,
		.y = -a.y,
		.z = -a.z,
		.w = -a.w,
	};
}
sl_inline vec4_s32 neg_vec4_s32(vec4_s32 a) {
	return (vec4_s32) {
		.x = -a.x,
		.y = -a.y,
		.z = -a.z,
		.w = -a.w,
	};
}
sl_inline vec4_s64 neg_vec4_s64(vec4_s64 a) {
	return (vec4_s64) {
		.x = -a.x,
		.y = -a.y,
		.z = -a.z,
		.w = -a.w,
	};
}
sl_inline vec4_f32 neg_vec4_f32(vec4_f32 a) {
	return (vec4_f32) {
		.x = -a.x ,
		.y = -a.y,
		.z = -a.z,
		.w = -a.w,
	};
}
sl_inline vec4_f64 neg_vec4_f64(vec4_f64 a) {
	return (vec4_f64) {
		.x = -a.x,
		.y = -a.y,
		.z = -a.z,
		.w = -a.w,
	};
}

sl_inline bool eq_vec4_u8(vec4_u8 a, vec4_u8 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}
sl_inline bool eq_vec4_u16(vec4_u16 a, vec4_u16 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}
sl_inline bool eq_vec4_u32(vec4_u32 a, vec4_u32 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}
sl_inline bool eq_vec4_u64(vec4_u64 a, vec4_u64 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}
sl_inline bool eq_vec4_s8(vec4_s8 a, vec4_s8 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}
sl_inline bool eq_vec4_s16(vec4_s16 a, vec4_s16 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}
sl_inline bool eq_vec4_s32(vec4_s32 a, vec4_s32 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}
sl_inline bool eq_vec4_s64(vec4_s64 a, vec4_s64 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}
sl_inline bool eq_vec4_f32(vec4_f32 a, vec4_f32 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}
sl_inline bool eq_vec4_f64(vec4_f64 a, vec4_f64 b) {
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
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

sl_inline mat4x4_f32 invert_mat4x4_f32(mat4x4_f32 mat) {
	f32 a = mat.x.x, b = mat.x.y, c = mat.x.z, d = mat.x.w,
		e = mat.y.x, f = mat.y.y, g = mat.y.z, h = mat.y.w,
		i = mat.z.x, j = mat.z.y, k = mat.z.z, l = mat.z.w,
		m = mat.w.x, n = mat.w.y, o = mat.w.z, p = mat.w.w,

		c1 = k * p - l * o, c2 = c * h - d * g, c3 = i * p - l * m,
		c4 = a * h - d * e, c5 = j * p - l * n, c6 = b * h - d * f,
		c7 = i * n - j * m, c8 = a * f - b * e, c9 = j * o - k * n,
		c10 = b * g - c * f, c11 = i * o - k * m, c12 = a * g - c * e,

		idt = 1.0f / (c8 * c1 + c4 * c9 + c10 * c3 + c2 * c7 - c12 * c5 - c6 * c11), ndt = -idt;

	mat4x4_f32 dest;
	dest.x.x = (f * c1 - g * c5 + h * c9) * idt;
	dest.x.y = (b * c1 - c * c5 + d * c9) * ndt;
	dest.x.z = (n * c2 - o * c6 + p * c10) * idt;
	dest.x.w = (j * c2 - k * c6 + l * c10) * ndt;

	dest.y.x = (e * c1 - g * c3 + h * c11) * ndt;
	dest.y.y = (a * c1 - c * c3 + d * c11) * idt;
	dest.y.z = (m * c2 - o * c4 + p * c12) * ndt;
	dest.y.w = (i * c2 - k * c4 + l * c12) * idt;

	dest.z.x = (e * c5 - f * c3 + h * c7) * idt;
	dest.z.y = (a * c5 - b * c3 + d * c7) * ndt;
	dest.z.z = (m * c6 - n * c4 + p * c8) * idt;
	dest.z.w = (i * c6 - j * c4 + l * c8) * ndt;

	dest.w.x = (e * c9 - f * c11 + g * c7) * ndt;
	dest.w.y = (a * c9 - b * c11 + c * c7) * idt;
	dest.w.z = (m * c10 - n * c12 + o * c8) * ndt;
	dest.w.w = (i * c10 - j * c12 + k * c8) * idt;

	return dest;
}
sl_inline mat4x4_f64 invert_mat4x4_f64(mat4x4_f64 mat) {
	f64 a = mat.x.x, b = mat.x.y, c = mat.x.z, d = mat.x.w,
		e = mat.y.x, f = mat.y.y, g = mat.y.z, h = mat.y.w,
		i = mat.z.x, j = mat.z.y, k = mat.z.z, l = mat.z.w,
		m = mat.w.x, n = mat.w.y, o = mat.w.z, p = mat.w.w,

		c1 = k * p - l * o, c2 = c * h - d * g, c3 = i * p - l * m,
		c4 = a * h - d * e, c5 = j * p - l * n, c6 = b * h - d * f,
		c7 = i * n - j * m, c8 = a * f - b * e, c9 = j * o - k * n,
		c10 = b * g - c * f, c11 = i * o - k * m, c12 = a * g - c * e,

		idt = 1.0f / (c8 * c1 + c4 * c9 + c10 * c3 + c2 * c7 - c12 * c5 - c6 * c11), ndt = -idt;

	mat4x4_f64 dest;
	dest.x.x = (f * c1 - g * c5 + h * c9) * idt;
	dest.x.y = (b * c1 - c * c5 + d * c9) * ndt;
	dest.x.z = (n * c2 - o * c6 + p * c10) * idt;
	dest.x.w = (j * c2 - k * c6 + l * c10) * ndt;

	dest.y.x = (e * c1 - g * c3 + h * c11) * ndt;
	dest.y.y = (a * c1 - c * c3 + d * c11) * idt;
	dest.y.z = (m * c2 - o * c4 + p * c12) * ndt;
	dest.y.w = (i * c2 - k * c4 + l * c12) * idt;

	dest.z.x = (e * c5 - f * c3 + h * c7) * idt;
	dest.z.y = (a * c5 - b * c3 + d * c7) * ndt;
	dest.z.z = (m * c6 - n * c4 + p * c8) * idt;
	dest.z.w = (i * c6 - j * c4 + l * c8) * ndt;

	dest.w.x = (e * c9 - f * c11 + g * c7) * ndt;
	dest.w.y = (a * c9 - b * c11 + c * c7) * idt;
	dest.w.z = (m * c10 - n * c12 + o * c8) * ndt;
	dest.w.w = (i * c10 - j * c12 + k * c8) * idt;

	return dest;
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

typedef struct Box_f32 {
	vec3_f32 start;
	vec3_f32 end;
} Box_f32;

sl_inline vec3_f32 box_f32_get_center(Box_f32 b) {
	return mul_vec3_f32(add_vec3_f32(b.start, b.end), (vec3_f32) { 0.5f, 0.5f, 0.5f });
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
#define immutable_buffer_for(X) ((Immutable_Buffer) { .data = &(X), .size = sizeof((X)) })
#define mutable_buffer_for(X) ((Mutable_Buffer) { .data = &(X), .size = sizeof((X)) })

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
	ptr = (allocator)->new((allocator)->ctx, sizeof(*ptr) * (u64)(count), sl_align_of(*ptr))
#define allocator_resize(allocator, ptr, old_count, new_count)\
	ptr = (allocator)->resize((allocator)->ctx, ptr, sizeof(*ptr) * (u64)(old_count), sizeof(*ptr) * (u64)(new_count), sl_align_of(*ptr))
#define allocator_free(allocator, ptr, count)\
	(allocator)->free((allocator)->ctx, ptr, sizeof(*ptr) * (u64)(count), sl_align_of(*ptr))

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
		*out_e = *sl_concat(function_prefix, _get_ptr)(s, s->element_count - 1);\
		--s->element_count;\
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
	}\
	sl_inline void sl_concat(function_prefix, _clear)(type* s) {\
		s->element_count = 0;\
	}

sl_seq(u8, Seq_u8, seq_u8);
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

sl_seq(Mutable_Buffer, Seq_Mutable_Buffer, seq_mutable_buffer);

typedef struct SL_Arena_Allocator {
	Allocator* basis_allocator;
	Allocator allocator;

	u8* buffer;
	u8* next_position;
	u64 size;
} SL_Arena_Allocator;

static void* _sl_arena_allocator_new(void* ctx, u64 size, u64 alignment) {
	SL_Arena_Allocator* arena = ctx;
	
	arena->next_position += (u64)arena->next_position % alignment;
	sl_assert(arena->next_position < (arena->buffer + arena->size), "Arena capacity exceeded.");
	void* result = arena->next_position;
	arena->next_position += size;

	return result;
}
static void* _sl_arena_allocator_resize(void* ctx, void* ptr, u64 old_size, u64 new_size, u64 alignment) {
	return _sl_arena_allocator_new(ctx, new_size, alignment);
}
static void _sl_arena_allocator_free(void* ctx, void* ptr, u64 size, u64 alignment) {}
sl_inline SL_Arena_Allocator* sl_arena_allocator_new(Allocator* allocator, u64 size) {
	u8* buffer;
	allocator_new(allocator, buffer, size);

	SL_Arena_Allocator* result;
	allocator_new(allocator, result, 1);
	*result = (SL_Arena_Allocator) {
		.basis_allocator = allocator,
		.allocator = {
			.ctx = result,
			.new = _sl_arena_allocator_new,
			.resize = _sl_arena_allocator_resize,
			.free = _sl_arena_allocator_free,
		},
		.buffer = buffer,
		.size = size,
		.next_position = buffer,
	};
	return result;
}
sl_inline void sl_arena_allocator_destroy(SL_Arena_Allocator* allocator) {
	allocator_free(allocator->basis_allocator, allocator->buffer, allocator->size);
	allocator_free(allocator->basis_allocator, allocator, 1);
	*allocator = (SL_Arena_Allocator) {0};
}
sl_inline void sl_arena_allocator_reset(SL_Arena_Allocator* allocator, u64 position) {
	allocator->next_position = allocator->buffer + position;
}
sl_inline u64 sl_arena_allocator_get_position(SL_Arena_Allocator* allocator) {
	return (u64)(allocator->next_position - allocator->buffer);
}

#define sl_hashmap(key_type, value_type, type, function_prefix, hash_func, equals_func) \
    typedef struct { \
        key_type* keys; \
        value_type* values; \
        u64* hashes; \
        u64 capacity; \
        u64 count; \
        Allocator* allocator; \
    } type; \
    \
    sl_inline type sl_concat(function_prefix, _new)(Allocator* allocator, u64 initial_capacity) { \
        type m = {0}; \
        m.allocator = allocator; \
        m.capacity = 1ULL << sl_next_pow2_exp_u64(initial_capacity); \
        m.count = 0; \
        allocator_new(allocator, m.keys, m.capacity); \
        allocator_new(allocator, m.values, m.capacity); \
        allocator_new(allocator, m.hashes, m.capacity); \
        for (u64 i = 0; i < m.capacity; i++) m.hashes[i] = 0; \
        return m; \
    } \
    \
    sl_inline void sl_concat(function_prefix, _destroy)(type* m) { \
        allocator_free(m->allocator, m->keys, m->capacity); \
        allocator_free(m->allocator, m->values, m->capacity); \
        allocator_free(m->allocator, m->hashes, m->capacity); \
        *m = (type){0}; \
    } \
    \
    sl_inline void sl_concat(function_prefix, _resize)(type* m, u64 new_capacity) { \
        key_type* new_keys; allocator_new(m->allocator, new_keys, new_capacity); \
        value_type* new_values; allocator_new(m->allocator, new_values, new_capacity); \
        u64* new_hashes; allocator_new(m->allocator, new_hashes, new_capacity); \
        for (u64 i = 0; i < new_capacity; i++) new_hashes[i] = 0; \
        for (u64 i = 0; i < m->capacity; i++) { \
            if (m->hashes[i] != 0) { \
                u64 h = m->hashes[i]; \
                u64 idx = h & (new_capacity - 1); \
                while (new_hashes[idx] != 0) idx = (idx + 1) & (new_capacity - 1); \
                new_keys[idx] = m->keys[i]; \
                new_values[idx] = m->values[i]; \
                new_hashes[idx] = h; \
            } \
        } \
        allocator_free(m->allocator, m->keys, m->capacity); \
        allocator_free(m->allocator, m->values, m->capacity); \
        allocator_free(m->allocator, m->hashes, m->capacity); \
        m->keys = new_keys; m->values = new_values; m->hashes = new_hashes; m->capacity = new_capacity; \
    } \
    \
    sl_inline void sl_concat(function_prefix, _insert)(type* m, key_type key, value_type value) { \
        if (m->count * 2 >= m->capacity) sl_concat(function_prefix, _resize)(m, m->capacity * 2); \
        u64 h = hash_func(key); \
        u64 idx = h & (m->capacity - 1); \
        while (m->hashes[idx] != 0 && !equals_func(m->keys[idx], key)) idx = (idx + 1) & (m->capacity - 1); \
        if (m->hashes[idx] == 0) m->count++; \
        m->keys[idx] = key; \
        m->values[idx] = value; \
        m->hashes[idx] = h; \
    } \
    \
    sl_inline bool sl_concat(function_prefix, _get)(type* m, key_type key, value_type* out_value) { \
        if (m->count == 0) return false; \
        u64 h = hash_func(key); \
        u64 idx = h & (m->capacity - 1); \
        u64 start = idx; \
        while (m->hashes[idx] != 0) { \
            if (equals_func(m->keys[idx], key)) { *out_value = m->values[idx]; return true; } \
            idx = (idx + 1) & (m->capacity - 1); \
            if (idx == start) break; \
        } \
        return false; \
    } \
    \
    sl_inline bool sl_concat(function_prefix, _remove)(type* m, key_type key) { \
        if (m->count == 0) return false; \
        u64 h = hash_func(key); \
        u64 idx = h & (m->capacity - 1); \
        u64 start = idx; \
        while (m->hashes[idx] != 0) { \
            if (equals_func(m->keys[idx], key)) { \
                m->hashes[idx] = 0; m->count--; \
                return true; \
            } \
            idx = (idx + 1) & (m->capacity - 1); \
            if (idx == start) break; \
        } \
        return false; \
    } \
    \
    sl_inline u64 sl_concat(function_prefix, _get_count)(type* m) { \
        return m->count; \
    }

typedef struct SL_Handle {
	u32 index;
	u32 generation;
} SL_Handle;
sl_seq(SL_Handle, SL_Handle_Seq, sl_handle_seq);
#define SL_HANDLE_NULL ((SL_Handle) { .index = 0, .generation = 0 })

#define sl_pool(element, type, function_prefix)\
	sl_seq(element, sl_concat(type, _Backing_Seq), sl_concat(function_prefix, _backing_seq));\
	typedef struct type {\
		sl_concat(type, _Backing_Seq) backing;\
		Seq_u32 free_list;\
	} type;\
	sl_inline type sl_concat(function_prefix, _new)(Allocator* allocator, u64 initial_capacity) {\
		type result = {\
			.backing = sl_concat(function_prefix, _backing_seq_new)(allocator, initial_capacity),\
			.free_list = seq_u32_new(allocator, initial_capacity),\
		};\
		return result;\
	}\
	sl_inline void sl_concat(function_prefix, _destroy)(type* p) {\
		sl_concat(function_prefix, _backing_seq_destroy)(&p->backing);\
		seq_u32_destroy(&p->free_list);\
		*p = (type) {};\
	}\
	sl_inline element* sl_concat(function_prefix, _resolve)(type* p, SL_Handle handle) {\
		element* e = sl_concat(function_prefix, _backing_seq_get_ptr)(&p->backing, handle.index);\
		if (e->generation == handle.generation) {\
			return e;\
		} else {\
			return NULL;\
		}\
	}\
	sl_inline SL_Handle sl_concat(function_prefix, _acquire)(type* p) {\
		u32 index;\
		if (!seq_u32_pop(&p->free_list, &index)) {\
			index = sl_concat(function_prefix, _backing_seq_get_count)(&p->backing);\
			sl_concat(function_prefix, _backing_seq_push)(&p->backing, (element) {0});\
		}\
		\
		element* e = sl_concat(function_prefix, _backing_seq_get_ptr)(&p->backing, index);\
		const u32 new_generation = ++e->generation;\
		*e = (element) {\
			.generation = new_generation,\
		};\
		return (SL_Handle) {\
			.index = index,\
			.generation = new_generation,\
		};\
	}\
	sl_inline void sl_concat(function_prefix, _release)(type* p, SL_Handle handle) {\
		element* e = sl_concat(function_prefix, _resolve)(p, handle);\
		sl_assert(e != NULL, "Can't release a stale handle.");\
		seq_u32_push(&p->free_list, handle.index);\
		e->generation++;\
	}\

u64 sl_hash_bytes(Immutable_Buffer buffer) {
	// FNV-1a
	const u8* bytes = (const u8*)buffer.data;
	u64 hash = 1469598103934665603ULL;

	for (u64 i = 0; i < buffer.size; i++) {
		hash ^= bytes[i];
		hash *= 1099511628211ULL;
	}

	return hash;
}

typedef struct SL_Hasher {
	u64 hash;
} SL_Hasher;

void sl_hasher_init(SL_Hasher* hasher) {
	*hasher = (SL_Hasher) {
		.hash = 1469598103934665603ULL,
	};
}
void sl_hasher_push(SL_Hasher* hasher, Immutable_Buffer buffer) {
	for (u64 i = 0; i < buffer.size; i++) {
		hasher->hash ^= (u64)((u8*)buffer.data)[i];
		hasher->hash *= 1099511628211ULL;
	}
}
u64 sl_hasher_finalise(SL_Hasher* hasher) {
	return hasher->hash;
}

#endif

typedef struct Quad_f32 {
	vec2_f32 v[4];
} Quad_f32;

#define u8_max 255u
#define u16_max 65535u
#define u32_max 4294967295u
#define u64_max 18446744073709551615ull
