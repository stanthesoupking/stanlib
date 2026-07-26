#pragma once

#include <stanlib/core.h>

typedef struct SL_String SL_String;
sl_seq(SL_String*, SL_String_Seq, sl_string_seq);

SL_String* sl_string_new(Allocator* allocator);
void sl_string_retain(SL_String* string);
void sl_string_release(SL_String* string);

u64 sl_string_get_length(const SL_String* string);

SL_String* sl_string_copy(Allocator* allocator, const SL_String* basis);

SL_String* sl_string_new_from_c(Allocator* allocator, const char* cstring);
const char* sl_string_get_c(const SL_String* string);

void sl_string_append(SL_String* string, const SL_String* other);
void sl_string_append_c(SL_String* string, const char* other);
