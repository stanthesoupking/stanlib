#include "stanlib/string.h"
#include "stanlib/core.h"
#include <string.h>

#define SL_STRING_INITIAL_CAPACITY 16

typedef struct SL_String {
	Allocator* allocator;

	char* buffer_data;
	u64 buffer_capacity;

	u64 length;
	u64 rc;
} SL_String;

SL_String* sl_string_new(Allocator* allocator) {
	SL_String* string;
	allocator_new(allocator, string, 1);
	*string = (SL_String) {
		.allocator = allocator,
		.buffer_capacity = SL_STRING_INITIAL_CAPACITY,
		.rc = 1,
	};
	allocator_new(allocator, string->buffer_data, SL_STRING_INITIAL_CAPACITY);
	string->buffer_data[0] = 0;
	return string;
}

void sl_string_retain(SL_String* string) {
	string->rc++;
}

void sl_string_release(SL_String* string) {
	string->rc--;
	if (string->rc == 0) {
		Allocator* allocator = string->allocator;
		allocator_free(allocator, string->buffer_data, string->buffer_capacity);
		allocator_free(allocator, string, 1);
	}
}

u64 sl_string_get_length(const SL_String* string) {
	return string->length;
}

void sl_string_ensure_capacity(SL_String* string, u64 capacity) {
	if (string->buffer_capacity < capacity) {
		allocator_resize(string->allocator, string->buffer_data, string->buffer_capacity, capacity);
	}
}

SL_String* sl_string_copy(Allocator* allocator, const SL_String* basis) {
	SL_String* string = sl_string_new(allocator);
	const u64 basis_length = sl_string_get_length(basis);
	sl_string_ensure_capacity(string, basis_length + 1);
	sl_memcpy(string->buffer_data, basis->buffer_data, basis_length);
	string->buffer_data[basis_length] = 0;
	string->length = basis_length;
	return string;
}

SL_String* sl_string_new_from_c(Allocator* allocator, const char* cstring) {
	const u64 cstring_length = strlen(cstring);

	SL_String* string = sl_string_new(allocator);
	sl_string_ensure_capacity(string, cstring_length + 1);
	sl_memcpy(string->buffer_data, cstring, cstring_length);
	string->buffer_data[cstring_length] = 0;
	string->length = cstring_length;
	return string;
}
const char* sl_string_get_c(const SL_String* string) {
	return string->buffer_data;
}

void sl_string_append(SL_String* string, const SL_String* other) {
	const u64 new_length = string->length + other->length;
	sl_string_ensure_capacity(string, new_length + 1);
	sl_memcpy(string->buffer_data + string->length, other->buffer_data, other->length);
	string->buffer_data[new_length] = 0;
	string->length = new_length;
}
void sl_string_append_c(SL_String* string, const char* other) {
	const u64 other_length = strlen(other);
	const u64 new_length = string->length + other_length;
	sl_string_ensure_capacity(string, new_length + 1);
	sl_memcpy(string->buffer_data + string->length, other, other_length);
	string->buffer_data[new_length] = 0;
	string->length = new_length;
}
