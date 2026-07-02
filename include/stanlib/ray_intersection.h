#pragma once

#include <stanlib/core.h>

typedef struct SL_Ray {
	vec3_f32 origin;
	vec3_f32 direction;
	vec3_f32 direction_recip;
} SL_Ray;

SL_Ray sl_ray_from_ndc(mat4x4_f32 to_world_from_ndc, vec2_f32 ndc);

SL_Ray sl_ray_advance(SL_Ray ray, f32 amount);

typedef struct SL_Ray_Intersection {
	bool hit;
	f32 near;
	f32 far;
	vec3_f32 normal;
} SL_Ray_Intersection;

SL_Ray_Intersection sl_ray_intersect_box(SL_Ray ray, Box_f32 box);
