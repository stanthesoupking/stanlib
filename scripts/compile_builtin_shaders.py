import stanlib as sl

include_paths = ["include"]

blit_vs_slang = sl.SlangSource("src/blitter/shaders/blit.slang", "vs_main", include_paths)
blit_fs_slang = sl.SlangSource("src/blitter/shaders/blit.slang", "fs_main", include_paths)

shaders = [
	sl.Shader("blit_vs", { sl.Target.SPV: blit_vs_slang, sl.Target.METALLIB: blit_vs_slang }),
	sl.Shader("blit_fs", { sl.Target.SPV: blit_fs_slang, sl.Target.METALLIB: blit_fs_slang }),
]

sl.compile_shaders("src/blitter/shaders.h", "src/blitter/shaders.c", shaders)
