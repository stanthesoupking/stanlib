#include "stanlib/core.h"
#include <stanlib/ds.h>

#include <stdio.h>
#include <string.h>

#define DS_MAGIC_BYTES_LENGTH 8
const char DS_MAGIC_BYTES[DS_MAGIC_BYTES_LENGTH] = "CALDERA_";

typedef struct DS_Reader {
	Allocator* allocator;
	FILE* file;

	u8* value_buffer_data;
	u64 value_buffer_size;
} DS_Reader;

typedef struct DS_Writer {
	Allocator* allocator;
	FILE* file;
} DS_Writer;

bool ds_read_buffer(FILE* file, Mutable_Buffer buffer) {
	return fread_unlocked(buffer.data, buffer.size, 1, file) > 0;
}
bool ds_read_u64(FILE* file, u64* out_v) {
	Mutable_Buffer buffer = {
		.data = out_v,
		.size = sizeof(*out_v),
	};
	return ds_read_buffer(file, buffer);
}

bool ds_write_buffer(FILE* file, Immutable_Buffer buffer) {
	return fwrite(buffer.data, buffer.size, 1, file) > 0;
}
bool ds_write_u64(FILE* file, u64 v) {
	return ds_write_buffer(file, immutable_buffer_for(v));
}

DS_Reader* ds_reader_open(Allocator* allocator, const char* path) {
	FILE* file = fopen(path, "rb");
	if (!file) {
		return NULL;
	}

	// Check magic bytes
	char file_magic[DS_MAGIC_BYTES_LENGTH] = {};
	unsigned long file_magic_read_elements = fread(file_magic, DS_MAGIC_BYTES_LENGTH, 1, file);
	if ((file_magic_read_elements < 1) || (memcmp(DS_MAGIC_BYTES, file_magic, DS_MAGIC_BYTES_LENGTH) != 0)) {
		fclose(file);
		return NULL;
	}

	DS_Reader* reader;
	allocator_new(allocator, reader, 1);
	*reader = (DS_Reader) {
		.allocator = allocator,
		.file = file,
	};
	return reader;
}
void ds_reader_close(DS_Reader* reader) {
	Allocator* allocator = reader->allocator;
	fclose(reader->file);
	allocator_free(allocator, reader->value_buffer_data, reader->value_buffer_size);
	allocator_free(allocator, reader, 1);
}
bool ds_reader_continue(DS_Reader* reader, DS_Primitive* out_primitive) {

	u64 primitive_kind;
	if (!ds_read_u64(reader->file, &primitive_kind)) {
		return false;
	}

	DS_Key key;
	for (u8 i = 0; i < DS_KEY_LENGTH; i++) {
		if (!ds_read_u64(reader->file, &key.elements[i])) {
			return false;
		}
	}

	u64 value_length;
	if (!ds_read_u64(reader->file, &value_length)) {
		return false;
	}

	if (value_length > reader->value_buffer_size) {
		const u64 new_buffer_length = value_length;
		allocator_resize(reader->allocator, reader->value_buffer_data, reader->value_buffer_size, new_buffer_length);
		reader->value_buffer_size = new_buffer_length;
	}

	Mutable_Buffer value_buffer = {
		.data = reader->value_buffer_data,
		.size = value_length,
	};
	if ((value_length > 0) && !ds_read_buffer(reader->file, value_buffer)) {
		return false;
	}

	*out_primitive = (DS_Primitive) {
		.kind = primitive_kind,
		.key = key,
		.value = sl_immutable_buffer_from_mutable(value_buffer),
	};

	return true;
}

DS_Writer* ds_writer_open(Allocator* allocator, const char* path) {
	FILE* file = fopen(path, "wb");
	if (!file) {
		return NULL;
	}

	// Write magic bytes
	fwrite(DS_MAGIC_BYTES, DS_MAGIC_BYTES_LENGTH, 1, file);

	DS_Writer* writer;
	allocator_new(allocator, writer, 1);
	*writer = (DS_Writer) {
		.allocator = allocator,
		.file = file,
	};
	return writer;
}
void ds_writer_close(DS_Writer* writer) {
	Allocator* allocator = writer->allocator;
	fclose(writer->file);
	allocator_free(allocator, writer, 1);
}
bool ds_writer_continue(DS_Writer* writer, DS_Primitive primitive) {

	// kind: u64, key: u64[4], value_length: u64, value: bytes[value_length]
	ds_write_u64(writer->file, primitive.kind);
	for (u8 i = 0; i < DS_KEY_LENGTH; i++) {
		ds_write_u64(writer->file, primitive.key.elements[i]);
	}
	ds_write_u64(writer->file, primitive.value.size);
	if (primitive.value.size > 0) {
		ds_write_buffer(writer->file, primitive.value);
	}

	return true;
}
