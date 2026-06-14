#!/bin/python3

import os
import platform
import struct
import subprocess
from pathlib import Path


def dump_bytes_to_file(file, source_file, source_variable):
    compiled_bytes = open(source_file, "rb").read()

    # pad to 4 bytes
    if len(compiled_bytes) % 4 != 0:
        compiled_bytes += b"\x00" * (4 - len(compiled_bytes) % 4)

    words = struct.unpack(f"<{len(compiled_bytes) // 4}I", compiled_bytes)

    file.write(f"const u32 {source_variable}[] = {{\n")
    for i in range(0, len(words), 8):
        file.write("\t" + ", ".join(f"0x{w:08X}" for w in words[i : i + 8]) + ",\n")
    file.write("};\n")

    return len(compiled_bytes)


def compile_shader(shader_path, c_name, entry_point, include_paths):
    parent_folder_path = str(Path(str(shader_path)).parent)
    compiled_spv_path = parent_folder_path + "/" + c_name + ".spv"
    compiled_metal_lib_path = parent_folder_path + "/" + c_name + ".metallib"
    h_path = parent_folder_path + "/" + c_name + ".h"
    c_path = parent_folder_path + "/" + c_name + ".c"

    args = [
        "slangc",
        str(shader_path),
        "-target",
        "spirv",
        "-entry",
        entry_point,
        "-o",
        compiled_spv_path,
        "-fvk-use-entrypoint-name",
    ]
    for include_path in include_paths:
        args.append("-I")
        args.append(include_path)

    subprocess.run(args)

    is_macos = platform.system() == "Darwin"

    if is_macos:
        args = [
            "slangc",
            str(shader_path),
            "-target",
            "metallib",
            "-entry",
            entry_point,
            "-o",
            compiled_metal_lib_path,
        ]
        for include_path in include_paths:
            args.append("-I")
            args.append(include_path)

        subprocess.run(args)

    c = open(c_path, "w")
    c.write("// machine generated, do not edit.\n")
    c.write('#include <stanlib/core.h>\n')
    c.write("\n")

    spv_source_variable = c_name + "_spv"
    spv_source_len = dump_bytes_to_file(c, compiled_spv_path, spv_source_variable)

    metal_source_variable = c_name + "_metal"
    if is_macos:
        metal_source_len = dump_bytes_to_file(
            c, compiled_metal_lib_path, metal_source_variable
        )

    c.close()

    blob_desc_variable = c_name + "_blob"

    h = open(h_path, "w")
    h.write("// machine generated, do not edit.\n")
    h.write("#include <stanlib/core.h>\n")
    h.write("#include <stanlib/gpu.h>\n")
    h.write("\n")
    h.write(f"extern const u32 {spv_source_variable}[];\n")
    h.write(f"extern const u32 {metal_source_variable}[];\n")
    h.write("\n")
    h.write(f"static const Gpu_Shader_Blob_Desc {blob_desc_variable} = {{\n")
    h.write("\t.spv = {\n")
    h.write(f"\t\t.data = {spv_source_variable},\n")
    h.write(f"\t\t.size = {spv_source_len},\n")
    h.write("\t},\n")
    if is_macos:
        h.write("\t.metallib = {\n")
        h.write(f"\t\t.data = {metal_source_variable},\n")
        h.write(f"\t\t.size = {metal_source_len},\n")
        h.write("\t},\n")
    h.write("};\n")
    h.write("\n")
    h.close()

    os.remove(compiled_spv_path)

    if is_macos:
        os.remove(compiled_metal_lib_path)
