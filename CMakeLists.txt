cmake_minimum_required(VERSION 3.20)
project(stanlib)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IS_IOS TRUE)
    set(IS_APPLE TRUE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "18.0")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(IS_MAC TRUE)
    set(IS_APPLE TRUE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(STANLIB_SOURCES
    "src/blitter/shaders.c"
    "src/blitter/blitter.c"
    "src/gpu/gpu_impl.c"
    "src/text.c"
    "src/ui.c"
)

add_library(stanlib "${STANLIB_SOURCES}")

if(IS_APPLE)
    enable_language(OBJC)
    target_compile_options(stanlib PRIVATE -fobjc-arc)
	set_source_files_properties("src/gpu/gpu_impl.c" PROPERTIES LANGUAGE OBJC)
	target_link_libraries(stanlib PRIVATE
    	"-framework Metal"
        "-framework CoreGraphics"
        "-framework IOKit"
        "-framework IOSurface"
        "-framework QuartzCore"
        "-framework Foundation"
    )
endif()

target_include_directories(stanlib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
