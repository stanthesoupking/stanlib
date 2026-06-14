import stanlib as sl

include_paths = ["include"]

sl.compile_shader("src/blitter/shaders/blit.slang", "blit_vs", "vs_main", include_paths)
sl.compile_shader("src/blitter/shaders/blit.slang", "blit_fs", "fs_main", include_paths)
