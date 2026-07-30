#include "stanlib/string.h"
#include "stanlib/core.h"
#include <string.h>
#include <stdarg.h>

#if SL_PLATFORM_WINDOWS
#define SL_PATH_SEPARATOR '\\'
#else
#define SL_PATH_SEPARATOR '/'
#endif

typedef struct SL_String {
	Allocator* allocator;
	u32 length;
	SL_Ref_Count rc;
	char data[];
} SL_String;

SL_String_View sl_string_view_c(const char* cstr) {
	return (SL_String_View) {
		.str = cstr,
		.length = strlen(cstr),
	};
}
SL_String_View sl_string_view(const SL_String* str) {
	return (SL_String_View) {
		.str = str->data,
		.length = str->length,
	};
}

sl_inline u64 sl_string_allocation_size_for_length(u64 length) {
	return sizeof(SL_String) + length + 1;
}

void sl_string_retain(SL_String* string) {
	if (string == NULL) {
		return;
	}

	sl_ref_count_retain(&string->rc);
}

void sl_string_release(SL_String* string) {
	if (string == NULL) {
		return;
	}

	if (sl_ref_count_release(&string->rc)) {
		Allocator* allocator = string->allocator;
		allocator->free(allocator->ctx, string, sl_string_allocation_size_for_length(string->length), sl_align_of(SL_String));
	}
}

SL_String* sl_string_new_uninit(Allocator* allocator, u64 length) {
	SL_String* string = allocator->new(allocator->ctx, sl_string_allocation_size_for_length(length), sl_align_of(SL_String));
	*string = (SL_String) {
		.allocator = allocator,
		.length = length,
	};
	sl_ref_count_init(&string->rc);
	return string;
}

SL_String* sl_string_new(Allocator* allocator, SL_String_View view) {
	SL_String* string = sl_string_new_uninit(allocator, view.length);
	sl_memcpy(string->data, view.str, view.length + 1);
	return string;
}
SL_String* sl_string_new_c(Allocator* allocator, const char* cstr) {
	return sl_string_new(allocator, sl_string_view_c(cstr));
}

SL_String* sl_string_format(Allocator* allocator, const char* format, ...) {
	va_list args;
    va_start(args, format);
    const s32 string_length = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (string_length < 0) {
        return NULL;
    }

    SL_String* result = sl_string_new_uninit(allocator, (u64)string_length);

    va_start(args, format);
    vsnprintf(result->data, (size_t)string_length + 1, format, args);
    va_end(args);

    return result;
}

SL_String* sl_string_concat(Allocator* allocator, SL_String_View a, SL_String_View b) {
	const u64 new_length = a.length + b.length;

	SL_String* result = sl_string_new_uninit(allocator, new_length);
	sl_memcpy(result->data, a.str, a.length);
	sl_memcpy(result->data + a.length, b.str, b.length);
	result->data[new_length] = 0;
	return result;
}

SL_String* sl_string_append_path_component(Allocator* allocator, SL_String_View path, SL_String_View component) {
	const bool has_seperator = (path.str[path.length - 1] == SL_PATH_SEPARATOR);
	const u64 new_length = path.length + component.length + (has_seperator ? 0 : 1);

	SL_String* result = sl_string_new_uninit(allocator, new_length);
	char* next_offset = result->data;

	sl_memcpy(next_offset, path.str, path.length);
	next_offset += path.length;

	if (!has_seperator) {
		next_offset[0] = SL_PATH_SEPARATOR;
		next_offset += 1;
	}

	sl_memcpy(next_offset, component.str, component.length);
	next_offset += component.length;

	sl_debug_assert(next_offset == (result->data + new_length), "Mismatch between length and offset.");

	next_offset[0] = '\0';

	return result;
}

bool sl_string_equals(SL_String_View a, SL_String_View b) {
	if (a.length != b.length) {
		return false;
	}

	for (u32 i = 0; i < a.length; i++) {
		if (a.str[i] != b.str[i]) {
			return false;
		}
	}

	return true;
}
bool sl_string_starts_with(SL_String_View s, SL_String_View prefix) {
	if (s.length < prefix.length) {
		return false;
	}

	for (u32 i = 0; i < prefix.length; i++) {
		if (s.str[i] != prefix.str[i]) {
			return false;
		}
	}

	return true;
}
bool sl_string_ends_with(SL_String_View s, SL_String_View suffix) {
	if (s.length < suffix.length) {
		return false;
	}

	for (u32 i = 0; i < suffix.length; i++) {
		if (s.str[s.length - suffix.length + i] != suffix.str[i]) {
			return false;
		}
	}

	return true;
}
