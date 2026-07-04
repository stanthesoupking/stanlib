
#include <stanlib/core.h>
#include <stanlib/gpu.h>
#include <stanlib/blitter.h>
#include <stanlib/text.h>

#include <string.h>

#include "shaders.h"

// MARK: Blitter Resources

typedef enum SL_Blitter_Draw_Kind {
	SL_Blitter_Draw_Kind_Normal,
	SL_Blitter_Draw_Kind_Swizzle_RRRR,
} SL_Blitter_Draw_Kind;

typedef struct SL_Blitter_Resources {
	Allocator* allocator;
	Gpu_Render_Pipeline normal_pipeline;
	Gpu_Render_Pipeline swizzle_rrrr_pipeline;
	Gpu_Sampler nearest_sampler;
	Gpu_Sampler linear_sampler;
} SL_Blitter_Resources;

Gpu_Render_Pipeline sl_blitter_new_render_pipeline(bool swizzle_rrrr) {
	// should be passed in
	const Gpu_Render_Pass_Layout render_pass_layout = {
		.attachments = {
			[0] = {
				.format = Gpu_Format_BGRA8_sRGB,
				.initial_layout = Gpu_Texture_Layout_General,
				.final_layout = Gpu_Texture_Layout_Present,
				.load_op = Gpu_Load_Op_Load,
			},
		},
		.attachment_count = 1,
	};

	const Gpu_Layout_Binding blit_quad_bindings[] = {
		{
			.kind = Gpu_Binding_Kind_Slice,
			.index = 0,
		},
		{
			.kind = Gpu_Binding_Kind_Sampled_Texture,
			.index = 1,
		},
		{
			.kind = Gpu_Binding_Kind_Sampler,
			.index = 2,
		},
	};
	const Gpu_Function_Constant constants[] = {
		{
			.kind = Gpu_Function_Constant_Kind_Bool,
			.v_bool = swizzle_rrrr,
			.index = 0,
		},
	};
	const Gpu_Render_Pipeline_Desc render_pipeline_desc = {
		.render_pass_layout = render_pass_layout,
		.vertex_blob = gpu_new_shader_blob(&blit_vs_blob),
		.fragment_blob = gpu_new_shader_blob(&blit_fs_blob),
		.vertex_entry_point = "vs_main",
		.fragment_entry_point = "fs_main",
		.bindings = blit_quad_bindings,
		.binding_count = sl_array_count(blit_quad_bindings),
		.primitive_kind = Gpu_Primitive_Kind_Triangle_Strip,
		.alpha_blending = true,
		.constants = constants,
		.constant_count = sl_array_count(constants),
	};
	return gpu_new_render_pipeline(&render_pipeline_desc);
}

SL_Blitter_Resources* sl_blitter_resources_new(Allocator* allocator) {
	const Gpu_Sampler_Desc linear_sampler_desc = {
		.min_filter = Gpu_Filter_Linear,
		.mag_filter = Gpu_Filter_Linear,
	};
	Gpu_Sampler linear_sampler = gpu_new_sampler(&linear_sampler_desc);

	const Gpu_Sampler_Desc nearest_sampler_desc = {
		.min_filter = Gpu_Filter_Nearest,
		.mag_filter = Gpu_Filter_Nearest,
	};
	Gpu_Sampler nearest_sampler = gpu_new_sampler(&nearest_sampler_desc);

	SL_Blitter_Resources* result;
	allocator_new(allocator, result, 1);
	*result = (SL_Blitter_Resources) {
		.allocator = allocator,
		.normal_pipeline = sl_blitter_new_render_pipeline(false),
		.swizzle_rrrr_pipeline = sl_blitter_new_render_pipeline(true),
		.linear_sampler = linear_sampler,
		.nearest_sampler = nearest_sampler,
	};
	return result;
}

void sl_blitter_resources_destroy(SL_Blitter_Resources* resources) {

}

Gpu_Render_Pipeline sl_blitter_resources_get_render_pipeline_for_draw_kind(SL_Blitter_Resources* resources, SL_Blitter_Draw_Kind kind) {
	switch (kind) {
		case SL_Blitter_Draw_Kind_Normal: return resources->normal_pipeline;
		case SL_Blitter_Draw_Kind_Swizzle_RRRR: return resources->swizzle_rrrr_pipeline;
	}
}

// MARK: Blitter

sl_seq(Textured_Quad_f32, Textured_Quad_f32_Seq, textured_quad_f32_seq);

typedef struct SL_Blitter_Draw {
	SL_Blitter_Draw_Kind kind;
	Gpu_Texture texture;
	Range_u32 range;
} SL_Blitter_Draw;
sl_seq(SL_Blitter_Draw, SL_Blitter_Draw_Seq, sl_blitter_draw_seq);

typedef struct SL_Blitter {
	Allocator* allocator;
	SL_Blitter_Resources* resources;
	Gpu_Command_Buffer command_buffer;

	mat4x4_f32 to_ndc_from_viewport;

	Textured_Quad_f32_Seq quads;
	SL_Blitter_Draw_Seq draws;
} SL_Blitter;

void sl_blitter_cleanup(SL_Blitter** blitter_ptr) {
	SL_Blitter* blitter = *blitter_ptr;
	textured_quad_f32_seq_destroy(&blitter->quads);
	sl_blitter_draw_seq_destroy(&blitter->draws);
	Allocator* allocator = blitter->allocator;
	allocator_free(allocator, blitter, 1);
	*blitter_ptr = NULL;
}

void sl_blitter_begin(SL_Blitter** blitter_ptr, const SL_Blitter_Desc* desc) {
	mat4x4_f32 to_ndc_from_viewport = ortho_mat4x4_f32(0.0f, desc->viewport.x, desc->viewport.y, 0.0f, 0.0f, 1.0f);

	allocator_new(desc->allocator, *blitter_ptr, 1);
	**blitter_ptr = (SL_Blitter) {
		.allocator = desc->allocator,
		.resources = desc->resources,
		.command_buffer = desc->command_buffer,
		.to_ndc_from_viewport = to_ndc_from_viewport,
		.quads = textured_quad_f32_seq_new(desc->allocator, 256),
		.draws = sl_blitter_draw_seq_new(desc->allocator, 256),
	};
}
void sl_blitter_draw_textured_quads_raw(SL_Blitter* blitter, Gpu_Texture texture, const Textured_Quad_f32* quads, u32 quad_count, SL_Blitter_Draw_Kind kind) {
	const Range_u32 quad_range = {
		.start = (u32)textured_quad_f32_seq_get_count(&blitter->quads),
		.end = (u32)textured_quad_f32_seq_get_count(&blitter->quads) + quad_count,
	};
	for (u32 i = 0; i < quad_count; i++) {
		Textured_Quad_f32 quad = quads[i];

		// Transform to NDC
		for (u32 j = 0; j < 4; j++) {
			quad.position[j] = mul_mat4x4_vec4_f32(blitter->to_ndc_from_viewport, quad.position[j]);
		}

		textured_quad_f32_seq_push(&blitter->quads, quad);
	}

	// adjust or push draw
	const u32 draw_count = (u32)sl_blitter_draw_seq_get_count(&blitter->draws);
	SL_Blitter_Draw* last_draw = (draw_count > 0) ? sl_blitter_draw_seq_get_ptr(&blitter->draws, draw_count - 1) : NULL;
	if (last_draw && sl_handle_equals(last_draw->texture, texture) && (last_draw->kind == kind)) {
		last_draw->range.end = quad_range.end;
	} else {
		sl_blitter_draw_seq_push(&blitter->draws, (SL_Blitter_Draw) {
			.kind = kind,
			.texture = texture,
			.range = quad_range,
		});
	}
}
void sl_blitter_draw_text(SL_Blitter* blitter, SL_Font* font, Gpu_Texture texture, const char* string, vec2_f32 position, vec4_f32 color) {
	u32 quad_count;
	sl_font_generate_geometry_for_string(font, texture, string, position, color, NULL, &quad_count);

	Textured_Quad_f32* quads;
	allocator_new(blitter->allocator, quads, quad_count);
	sl_font_generate_geometry_for_string(font, texture, string, position, color, quads, NULL);

	sl_blitter_draw_textured_quads_raw(blitter, texture, quads, quad_count, SL_Blitter_Draw_Kind_Normal);
	allocator_free(blitter->allocator, quads, quad_count);
}
void sl_blitter_draw_texture(SL_Blitter* blitter, Gpu_Texture texture, vec2_f32 position, vec4_f32 color) {
	const Gpu_Texture_Desc* texture_desc = gpu_get_texture_desc(texture);

	const vec2_f32 p_start = position;
	const vec2_f32 p_end = add_vec2_f32(p_start, (vec2_f32) { texture_desc->size.x, texture_desc->size.y });

	const Textured_Quad_f32 quad = {
		.position = {
			{ p_start.x, p_start.y, 0.0f, 1.0f },
			{ p_end.x, p_start.y, 0.0f, 1.0f },
			{ p_start.x, p_end.y, 0.0f, 1.0f },
			{ p_end.x, p_end.y, 0.0f, 1.0f },
		},
		.tint = {
			color,
			color,
			color,
			color,
		},
		.uv = {
			{ 0, 0 },
			{ 1, 0 },
			{ 0, 1 },
			{ 1, 1 }
		},
	};

	sl_blitter_draw_textured_quads(blitter, texture, &quad, 1);
}

void sl_blitter_draw_textured_quads(SL_Blitter* blitter, Gpu_Texture texture, const Textured_Quad_f32* quads, u32 quad_count) {
	sl_blitter_draw_textured_quads_raw(blitter, texture, quads, quad_count, SL_Blitter_Draw_Kind_Normal);
}

//void sl_blitter_execute_command(SL_Blitter* blitter, SL_Blitter_Command command) {
//	switch (command.kind) {
//		case SL_Blitter_Command_Kind_Draw_Text: {
//			sl_blitter_draw_text(blitter, command.draw_text.font, command.draw_text.string, command.draw_text.position, command.draw_text.color);
//		} break;
//
//		case SL_Blitter_Command_Kind_Draw_Texture: {
//			sl_blitter_draw_texture(blitter, command.draw_texture.texture, command.draw_texture.position, command.draw_texture.color);
//		} break;
//
//		case SL_Blitter_Command_Kind_Draw_Textured_Quads: {
//			sl_blitter_draw_textured_quads(blitter, command.draw_textured_quads.texture, command.draw_textured_quads.quads, command.draw_textured_quads.quad_count);
//		} break;
//	}
//}

void sl_blitter_end(SL_Blitter** blitter_ptr, Gpu_Slice* parameters_slice) {
	SL_Blitter* blitter = *blitter_ptr;

	const u32 quad_count = (u32)textured_quad_f32_seq_get_count(&blitter->quads);

	Gpu_Slice quads_slice;
	bool got_quads_slice = gpu_slice_suballocate(*parameters_slice, gpu_size_and_align_of_type(Textured_Quad_f32, quad_count), &quads_slice, parameters_slice);
	if (!got_quads_slice) {
		// ran out of space
		sl_blitter_cleanup(blitter_ptr);
		return;
	}

	Textured_Quad_f32* quads_slice_ptr = gpu_get_slice_host_ptr(quads_slice);
	for (u32 quad_idx = 0; quad_idx < quad_count; quad_idx++) {
		quads_slice_ptr[quad_idx] = textured_quad_f32_seq_get(&blitter->quads, quad_idx);
	}
	gpu_flush_slice_to_gpu(quads_slice);

	const u32 draw_count = (u32)sl_blitter_draw_seq_get_count(&blitter->draws);
	for (u32 draw_idx = 0; draw_idx < draw_count; draw_idx++) {
		const SL_Blitter_Draw draw = sl_blitter_draw_seq_get(&blitter->draws, draw_idx);

		Gpu_Slice draw_slice = {
			.heap = quads_slice.heap,
			.offset = quads_slice.offset + sizeof(Textured_Quad_f32) * draw.range.start,
			.size = sizeof(Textured_Quad_f32) * (draw.range.end - draw.range.start),
		};

		const Gpu_Binding draw_bindings[] = {
			{
				.kind = Gpu_Binding_Kind_Slice,
				.slice = draw_slice,
				.index = 0,
			},
			{
				.kind = Gpu_Binding_Kind_Sampled_Texture,
				.texture = draw.texture,
				.index = 1,
			},
			{
				.kind = Gpu_Binding_Kind_Sampler,
				.texture = blitter->resources->nearest_sampler,
				.index = 2,
			},
		};
		const Gpu_Draw_Desc draw_desc = {
			.pipeline = sl_blitter_resources_get_render_pipeline_for_draw_kind(blitter->resources, draw.kind),
			.bindings = draw_bindings,
			.binding_count = sl_array_count(draw_bindings),
			.instance_count = (draw.range.end - draw.range.start),
			.vertex_count = 4,
		};
		gpu_draw(blitter->command_buffer, &draw_desc);
	}

	sl_blitter_cleanup(blitter_ptr);
}
