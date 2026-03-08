#pragma once

#include "core.h"

typedef struct Fly_Cam {
	vec3_f32 position;
	vec2_f32 rotation;
} Fly_Cam;

sl_inline mat4x4_f32 fly_cam_get_to_world_from_view(const Fly_Cam* fly) {
	return mul_mat4x4_f32(translate_mat4x4_f32(fly->position), mul_mat4x4_f32(rotate_y_mat4x4_f32(fly->rotation.x), rotate_x_mat4x4_f32(fly->rotation.y)));
}

sl_inline mat4x4_f32 fly_cam_get_to_view_from_world(const Fly_Cam* fly) {
	return mul_mat4x4_f32(mul_mat4x4_f32(rotate_x_mat4x4_f32(-fly->rotation.y), rotate_y_mat4x4_f32(-fly->rotation.x)), translate_mat4x4_f32(fly->position));
}

sl_inline void fly_cam_update(Fly_Cam* fly, vec3_f32 movement, vec2_f32 rotation) {
	const mat4x4_f32 to_world_from_view = fly_cam_get_to_world_from_view(fly);
	const vec4_f32 world_movement_v4 = mul_mat4x4_vec4_f32(to_world_from_view, (vec4_f32) { movement.x, movement.y, movement.z, 0.0f });
	const vec3_f32 world_movement_v3 = { world_movement_v4.x, world_movement_v4.y, world_movement_v4.z };
	fly->position = add_vec3_f32(fly->position, world_movement_v3);

	fly->rotation = add_vec2_f32(fly->rotation, rotation);

	// Prevent the camera from exceeding 90 degrees of rotation.
	f32 y_clamp = 0.05f;
	fly->rotation.y = sl_clamp(fly->rotation.y, -(SL_PI * 0.5f) + y_clamp, (SL_PI * 0.5f) - y_clamp);
}