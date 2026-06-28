#include "stanlib/core.h"
#include <math.h>
#include <stanlib/ray_intersection.h>

SL_Ray sl_ray_advance(SL_Ray ray, f32 amount) {
	return (SL_Ray) {
		.origin = add_vec3_f32(ray.origin, mul_vec3_f32(ray.direction, splat_vec3_f32(amount))),
		.direction = ray.direction,
	};
}

SL_Ray_Intersection sl_ray_intersect_box(SL_Ray ray, Box_f32 box) {
	const f32 epsilon = 0.0001f;
	const vec3_f32 direction_recip = {
		fabsf(ray.direction.x) > epsilon ? (1.0f / ray.direction.x) : 0.0f,
		fabsf(ray.direction.y) > epsilon ? (1.0f / ray.direction.y) : 0.0f,
		fabsf(ray.direction.z) > epsilon ? (1.0f / ray.direction.z) : 0.0f,
	};
	const vec3_f32 t1 = mul_vec3_f32(sub_vec3_f32(box.start, ray.origin), direction_recip);
	const vec3_f32 t2 = mul_vec3_f32(sub_vec3_f32(box.end, ray.origin), direction_recip);

	const vec3_f32 tmin3 = min_vec3_f32(t1, t2);
	const vec3_f32 tmax3 = min_vec3_f32(t1, t2);

	s16 axis;
	if (tmin3.x > tmin3.y) {
		if (tmin3.x > tmin3.z) {
			axis = 0;
		} else {
			axis = 2;
		}
	} else {
		if (tmin3.y > tmin3.z) {
			axis = 1;
		} else {
			axis = 2;
		}
	}
	vec3_f32 normal = splat_vec3_f32(0.0f);
	switch (axis) {
		case 0:
			normal.x = 1.0f;
			break;

		case 1:
			normal.y = 1.0f;
			break;

		case 2:
			normal.z = 1.0f;
			break;
	}
	normal = mul_vec3_f32(normal, neg_vec3_f32(sign_vec3_f32(ray.direction)));

	const f32 near = sl_max(sl_max(tmin3.x, tmin3.y), tmin3.z);
	const f32 far = sl_min(sl_min(tmax3.x, tmax3.y), tmax3.z);
	const bool hit = (far >= sl_max(near, 0.0));

	return (SL_Ray_Intersection) {
		.hit = hit,
		.near = near,
		.far = far,
		.normal = normal,
	};
}
