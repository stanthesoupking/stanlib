#include "stanlib/core.h"
#include <math.h>
#include <stanlib/ray_intersection.h>

SL_Ray sl_ray_from_ndc(mat4x4_f32 to_world_from_ndc, vec2_f32 ndc) {
	vec4_f32 clip_near = { ndc.x, ndc.y, 0.0f, 1.0f };
	vec4_f32 clip_far  = { ndc.x, ndc.y, 1.0f, 1.0f };

	vec4_f32 world_near = mul_mat4x4_vec4_f32(to_world_from_ndc, clip_near);
	vec4_f32 world_far  = mul_mat4x4_vec4_f32(to_world_from_ndc, clip_far);

	vec3_f32 world_near_xyz = { world_near.x, world_near.y, world_near.z };
	vec3_f32 world_far_xyz = { world_far.x, world_far.y, world_far.z };

	world_near_xyz = div_vec3_f32(world_near_xyz, splat_vec3_f32(world_near.w));
	world_far_xyz = div_vec3_f32(world_far_xyz, splat_vec3_f32(world_far.w));

	vec3_f32 ray_origin = world_near_xyz;
	vec3_f32 ray_dir = normalize_vec3_f32(sub_vec3_f32(world_far_xyz, world_near_xyz));

	const f32 epsilon = 0.0001f;
	ray_dir = (vec3_f32) {
		fabsf(ray_dir.x) > epsilon ? ray_dir.x : 1e-4f,
		fabsf(ray_dir.y) > epsilon ? ray_dir.y : 1e-4f,
		fabsf(ray_dir.z) > epsilon ? ray_dir.z : 1e-4f,
	};

	const vec3_f32 ray_dir_recip = {
		1.0f / ray_dir.x,
		1.0f / ray_dir.y,
		1.0f / ray_dir.z,
	};

	return (SL_Ray) {
		.origin = ray_origin,
		.direction = ray_dir,
		.direction_recip = ray_dir_recip,
	};
}

SL_Ray sl_ray_advance(SL_Ray ray, f32 amount) {
	return (SL_Ray) {
		.origin = add_vec3_f32(ray.origin, mul_vec3_f32(ray.direction, splat_vec3_f32(amount))),
		.direction = ray.direction,
		.direction_recip = ray.direction_recip,
	};
}

SL_Ray_Intersection sl_ray_intersect_box(SL_Ray ray, Box_f32 box) {
	const vec3_f32 t1 = mul_vec3_f32(sub_vec3_f32(box.start, ray.origin), ray.direction_recip);
	const vec3_f32 t2 = mul_vec3_f32(sub_vec3_f32(box.end, ray.origin), ray.direction_recip);

	const vec3_f32 tmin3 = min_vec3_f32(t1, t2);
	const vec3_f32 tmax3 = max_vec3_f32(t1, t2);

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
