/// Mesh
/// In at least `.c` or `.m` file:
///
/// ```
/// #define MESH_IMPLEMENTATION 1
/// #include "mesh.h"
/// ```

#include "core.h"

#include <stdio.h>
#include <string.h>

typedef struct Mesh {
	u32 vertex_positions_offset;
	u32 vertex_uvs_offset;
	u32 vertex_normals_offset;
	
	u32 vertex_count;
	
	u32 size;
} Mesh;

typedef struct Mesh_OBJ_Face_Indices{
	s32 position;
	s32 uv;
	s32 normal;
} Mesh_OBJ_Face_Indices;

sl_inline Mesh_OBJ_Face_Indices mesh_parse_face(const char* s) {
	Mesh_OBJ_Face_Indices out = { -1, -1, -1 };

	out.position = atoi(s) - 1;

	const char* slash = strchr(s, '/');
	if (!slash) return out;

	out.uv = atoi(slash + 1) - 1;

	slash = strchr(slash + 1, '/');
	if (!slash) return out;

	out.normal = atoi(slash + 1) - 1;
	return out;
}

sl_inline Mesh mesh_load(const char* path, Allocator* scratch_allocator, Mutable_Buffer buf) {
	FILE* f = fopen(path, "r");
	if (f == NULL) {
		return (Mesh) {};
	}
		
	char line_buf[256];
	u64 line_len = 0;
	
	Seq_Vec3_f32 unique_positions_seq = seq_vec3_f32_new(scratch_allocator, 64);
	Seq_Vec3_f32 unique_normals_seq = seq_vec3_f32_new(scratch_allocator, 64);
	Seq_Vec2_f32 unique_uvs_seq = seq_vec2_f32_new(scratch_allocator, 64);
	
	Seq_Vec3_f32 final_positions_seq = seq_vec3_f32_new(scratch_allocator, 64);
	Seq_Vec3_f32 final_normals_seq = seq_vec3_f32_new(scratch_allocator, 64);
	Seq_Vec2_f32 final_uvs_seq = seq_vec2_f32_new(scratch_allocator, 64);
	
	while (true) {
		char c = fgetc(f);
		
		if ((c == EOF) || (c == '\n')) {
			line_buf[line_len++] = '\0';
			
			u64 first_token_length;
			for (first_token_length = 0; first_token_length < line_len; first_token_length++) {
				if (line_buf[first_token_length] == ' ') {
					break;
				}
			}
			
			if ((first_token_length == 1) && (strncmp(line_buf, "#", first_token_length) == 0)) {
				// comment
			} else if ((first_token_length == 1) && (strncmp(line_buf, "v", first_token_length) == 0)) {
				// position
				vec3_f32 position = {};
				sscanf(line_buf, "v %f %f %f", &position.x, &position.y, &position.z);
				seq_vec3_f32_push(&unique_positions_seq, position);
			} else if ((first_token_length == 1) && (strncmp(line_buf, "f", first_token_length) == 0)) {
				// face
				const char* s = line_buf + 2; // skip "f "
				
				Mesh_OBJ_Face_Indices face_indices[8];
				u32 face_index_count = 0;
				
				while (*s && face_index_count < 8) {
					while (*s == ' ') {
						s++;
					};
					
					if (*s == '\0') {
						break;
					}
					
					face_indices[face_index_count++] = mesh_parse_face(s);
					
					while (*s && *s != ' ') {
						s++;
					};
				}
				
				for (u32 i = 1; i + 1 < face_index_count; i++) {
					// triangle fan
					u32 tri[3] = { 0, i, i + 1 };
					for (u32 v = 0; v < 3; v++) {
						const Mesh_OBJ_Face_Indices idx = face_indices[tri[v]];
						seq_vec3_f32_push(&final_positions_seq, seq_vec3_f32_get(&unique_positions_seq, idx.position));
						seq_vec3_f32_push(&final_normals_seq, seq_vec3_f32_get(&unique_normals_seq, idx.normal));
						seq_vec2_f32_push(&final_uvs_seq, seq_vec2_f32_get(&unique_uvs_seq, idx.uv));
					}
				}
				
			} else if ((first_token_length == 2) && (strncmp(line_buf, "vt", first_token_length) == 0)) {
				// uv
				vec2_f32 uv = {};
				sscanf(line_buf, "vt %f %f", &uv.x, &uv.y);
				seq_vec2_f32_push(&unique_uvs_seq, uv);
			} else if ((first_token_length == 2) && (strncmp(line_buf, "vn", first_token_length) == 0)) {
				// normal
				vec3_f32 normal = {};
				sscanf(line_buf, "vn %f %f %f", &normal.x, &normal.y, &normal.z);
				seq_vec3_f32_push(&unique_normals_seq, normal);
			} else {
				// ignored
				// printf("ignored: %s\n", line_buf);
			}
			
			// reset
			line_len = 0;
			
			if (c == EOF) {
				break;
			}
		} else {
			line_buf[line_len++] = c;
		}
	}
	
	fclose(f);
	
	const u32 vertex_count = (u32)seq_vec3_f32_get_count(&final_positions_seq);
	const u32 positions_offset = 0;
	const u32 normals_offset = positions_offset + (vertex_count * sizeof(vec3_f32));
	const u32 uvs_offset = normals_offset + (vertex_count * sizeof(vec3_f32));
	const u32 size = uvs_offset + (vertex_count * sizeof(vec2_f32));
	
	const Mesh result = {
		.vertex_count = vertex_count,
		.vertex_positions_offset = positions_offset,
		.vertex_uvs_offset = uvs_offset,
		.vertex_normals_offset = normals_offset,
		.size = size,
	};
	
	if (!mutable_buffer_is_null(buf)) {
		sl_assert(buf.size >= size, "Buffer must be large enough to contain mesh.");
		for (u32 v = 0; v < vertex_count; v++) {
			((vec3_f32*)((u8*)buf.data + positions_offset))[v] = seq_vec3_f32_get(&final_positions_seq, v);
			((vec3_f32*)((u8*)buf.data + normals_offset))[v] = seq_vec3_f32_get(&final_normals_seq, v);
			((vec2_f32*)((u8*)buf.data + uvs_offset))[v] = seq_vec2_f32_get(&final_uvs_seq, v);
		}
	}
	
	seq_vec3_f32_destroy(&unique_positions_seq);
	seq_vec3_f32_destroy(&unique_normals_seq);
	seq_vec2_f32_destroy(&unique_uvs_seq);
	seq_vec3_f32_destroy(&final_positions_seq);
	seq_vec3_f32_destroy(&final_normals_seq);
	seq_vec2_f32_destroy(&final_uvs_seq);
	
	return result;
}
