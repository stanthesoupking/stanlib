#pragma once

#include <stanlib/core.h>

typedef struct SL_Ray {
	vec3_f32 origin;
	vec3_f32 direction;
} SL_Ray;

typedef struct SL_Ray_Intersection {
	bool hit;
	f32 near;
	f32 far;
	vec3_f32 normal;
} SL_Ray_Intersection;

SL_Ray_Intersection sl_ray_intersect_box(SL_Ray ray, Box_f32 box);
