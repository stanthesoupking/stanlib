#pragma once

#include <stanlib/core.h>

// Strings
// A string is read-only, all modifications are COW and result in a new string.

typedef struct SL_String_View {
	const char* str; // null terminated
	u64 length;
} SL_String_View;

typedef struct SL_String SL_String;
sl_seq(SL_String*, SL_String_Seq, sl_string_seq);

SL_String_View sl_string_view(const SL_String* str);
SL_String_View sl_string_view_c(const char* cstr);

SL_String* sl_string_new(Allocator* allocator, SL_String_View view);
SL_String* sl_string_new_c(Allocator* allocator, const char* cstr); // convenience

void sl_string_retain(SL_String* string);
void sl_string_release(SL_String* string);

SL_String* sl_string_concat(Allocator* allocator, SL_String_View a, SL_String_View b);

SL_String* sl_string_append_path_component(Allocator* allocator, SL_String_View path, SL_String_View component);

bool sl_string_equals(SL_String_View a, SL_String_View b);
bool sl_string_starts_with(SL_String_View s, SL_String_View prefix);
bool sl_string_ends_with(SL_String_View s, SL_String_View suffix);
