#pragma once

#include <stanlib/core.h>

// data stream

// 256 bits
#define DS_KEY_LENGTH 4
typedef struct DS_Key {
	u64 elements[DS_KEY_LENGTH];
} DS_Key;

typedef enum DS_Primitive_Kind {
	DS_Primitive_Kind_Prop,
	DS_Primitive_Kind_Push,
	DS_Primitive_Kind_Pop,
} DS_Primitive_Kind;

typedef struct DS_Primitive {
	DS_Primitive_Kind kind;
	DS_Key key;
	Immutable_Buffer value;
} DS_Primitive;

typedef struct DS_Reader DS_Reader;
typedef struct DS_Writer DS_Writer;

DS_Reader* ds_reader_open(Allocator* allocator, const char* path);
void ds_reader_close(DS_Reader* reader);
bool ds_reader_continue(DS_Reader* reader, DS_Primitive* out_primitive);

DS_Writer* ds_writer_open(Allocator* allocator, const char* path);
void ds_writer_close(DS_Writer* writer);
bool ds_writer_continue(DS_Writer* writer, DS_Primitive primitive);
