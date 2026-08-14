#include "stanlib/string.h"
#include <stanlib/platform.h>

#if defined(SL_PLATFORM_APPLE)
#include <CoreFoundation/CoreFoundation.h>
SL_String* sl_get_application_path(Allocator* allocator, SL_String_View path) {
    CFBundleRef main_bundle = CFBundleGetMainBundle();

    SL_String* str_ext = sl_string_get_path_extension(allocator, path);
    SL_String* path0 = sl_string_pop_path_extension(allocator, path);
    SL_String* str_name = sl_string_get_path_component(allocator, sl_string_view(path0));
    SL_String* str_subdirectory = sl_string_pop_path_component(allocator, path);

	CFStringRef cf_subdirectory = CFStringCreateWithCString(NULL, sl_string_view(str_subdirectory), kCFStringEncodingUTF8);
	CFStringRef cf_name = CFStringCreateWithCString(NULL, sl_string_view(str_name), kCFStringEncodingUTF8);
	CFStringRef cf_ext = CFStringCreateWithCString(NULL, sl_string_view(str_ext), kCFStringEncodingUTF8);

	sl_string_release(str_ext);
	sl_string_release(path0);
	sl_string_release(str_name);
	sl_string_release(str_subdirectory);

	CFURLRef url = CFBundleCopyResourceURL(main_bundle, cf_name, cf_ext, cf_subdirectory);

	if (cf_subdirectory != NULL) {
		CFRelease(cf_subdirectory);
	}

	CFRelease(cf_name);
	CFRelease(cf_ext);

	if (url == NULL) {
		return false;
	}

	CFStringRef cf_path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	CFRelease(url);

	if (sl_string_new_c(allocator, CFStringGetCString(cf_path, out_path, out_path_length, kCFStringEncodingUTF8))) {
		return true;
	} else {
		return false;
	}
}
#elif defined(SL_PLATFORM_LINUX)
#include <unistd.h>
#include <libgen.h>
SL_String* sl_get_application_path(Allocator* allocator, SL_String_View path) {
	char application_path[256];
	ssize_t len = readlink("/proc/self/exe", application_path, sl_array_count(application_path) - 1);
    if (len == -1) {
    	return NULL;
    };
    application_path[len] = '\0';

    const char* application_dir = dirname(application_path);

    return sl_string_append_path_component(allocator, sl_string_view_c(application_dir), path);
}
#else
SL_String* sl_get_application_path(Allocator* allocator, SL_String_View path) {
	// Not yet implemented
	return NULL;
}
#endif
