
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#if defined(__TINYC__)
// TCC doesn't support simd.
#define STBI_NO_SIMD
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#pragma clang diagnostic pop
