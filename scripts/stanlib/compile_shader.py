import struct
import subprocess
import platform
from pathlib import Path
from enum import Enum
from typing import Protocol, Iterable, Optional

def padded_bytes(bytes: bytes, padding: int) -> bytes:
	if len(bytes) % 4 != 0:
		return bytes + b"\x00" * (4 - len(bytes) % 4)
	else:
		return bytes

def c_source_from_bytes(bytes: bytes, var_name: str) -> str:
	assert len(bytes) % 4 == 0, "bytes must be padded"

	words = struct.unpack(f"<{len(bytes) // 4}I", bytes)

	result_lines = [f"const u32 {var_name}[] = {{"]
	for i in range(0, len(words), 8):
		result_lines.append("\t" + ", ".join(f"0x{w:08X}" for w in words[i : i + 8]) + ",")
	result_lines.append("};")

	result = "\n".join(result_lines)
	return result

class CompilerSettings:
	def __init__(self, include_sources: bool = False, macos_min: str = "14.0"):
		self.include_sources = include_sources
		self.macos_min = macos_min

class Target(Enum):
	SPV = 1
	METALLIB = 2

def target_suffix(target: Target) -> str:
	if target == Target.SPV:
		return "_spv"
	elif target == Target.METALLIB:
		return "_metallib"
	else:
		return None

def target_blob_member(target: Target) -> str:
	if target == Target.SPV:
		return ".spv"
	elif target == Target.METALLIB:
		return ".metallib"
	else:
		return None

class ShaderSource(Protocol):
	def compile(self, target: Target, settings: CompilerSettings) -> bytes:
		...

class SlangSource:
	def __init__(self, path: str, entry_point: str, include_paths: Iterable[str]):
		self.path = path
		self.entry_point = entry_point
		self.include_paths = include_paths

	def compile_spv(self, settings: CompilerSettings) -> bytes:
		temp_file_spv = "temp.spv"
		Path(temp_file_spv).unlink(missing_ok=True)

		args = [
			"slangc",
			self.path,
			"-target",
			"spirv",
			"-O2",
			"-entry",
			self.entry_point,
			"-o",
			temp_file_spv,
			"-fvk-use-entrypoint-name",
		]

		for include_path in self.include_paths:
			args.append("-I")
			args.append(include_path)

		subprocess.run(args, check=True)

		with open(temp_file_spv, "rb") as file:
			compiled_bytes = file.read()

		Path(temp_file_spv).unlink(missing_ok=True)

		return compiled_bytes

	def compile_metallib(self, settings: CompilerSettings) -> bytes:
		temp_file_metal = "temp.metal"
		temp_file_air = "temp.air"
		temp_file_metallib = "temp.metallib"
		Path(temp_file_metal).unlink(missing_ok=True)
		Path(temp_file_air).unlink(missing_ok=True)
		Path(temp_file_metallib).unlink(missing_ok=True)

		args = [
			"slangc",
			self.path,
			"-target",
			"metal",
			"-O2",
			"-entry",
			self.entry_point,
			"-o",
			temp_file_metal,
			"-fvk-use-entrypoint-name",
		]

		for include_path in self.include_paths:
			args.append("-I")
			args.append(include_path)

		subprocess.run(args, check=True)

		# produce .air
		args = [
			"xcrun",
			"metal",
			f"-mmacosx-version-min={settings.macos_min}",
			"-O2",
			"-ffast-math",
			"-c",
			temp_file_metal,
			"-o",
			temp_file_air,
		]

		if settings.include_sources:
			args.append("-frecord-sources=flat")

		subprocess.run(args, check=True)

		# produce .metallib
		args = [
			"xcrun",
			"metallib",
			temp_file_air,
			"-o",
			temp_file_metallib,
		]
		subprocess.run(args, check=True)

		with open(temp_file_metallib, "rb") as file:
			compiled_bytes = file.read()

		Path(temp_file_metal).unlink(missing_ok=True)
		Path(temp_file_air).unlink(missing_ok=True)
		Path(temp_file_metallib).unlink(missing_ok=True)

		return compiled_bytes

	def compile(self, target: Target, settings: CompilerSettings) -> Optional[bytes]:
		if target == Target.SPV:
			return self.compile_spv(settings)
		elif target == Target.METALLIB:
			if platform.system() == "Darwin":
				return self.compile_metallib(settings)
			else:
				return None
		else:
			return None

class Shader:
	def __init__(self, name: str, sources: dict[Target, ShaderSource]):
		self.name = name
		self.sources = sources

class ImmutableBuffer:
	def __init__(self, data: str, size: int):
		self.data = data
		self.size = size

	def declaration(self) -> str:
		return f"(Immutable_Buffer) {{ .data = {self.data}, .size = {self.size} }}"


def compile_shaders(output_h_path: str, output_c_path: str, shaders: Iterable[Shader], settings: CompilerSettings = CompilerSettings()):
	Path(output_h_path).unlink(missing_ok=True)
	Path(output_c_path).unlink(missing_ok=True)

	output_h_file = open(output_h_path, "w")
	output_c_file = open(output_c_path, "w")

	for file in [output_h_file, output_c_file]:
		file.write("// machine generated, do not edit.\n")
		file.write("#include <stanlib/core.h>\n")
		file.write("#include <stanlib/gpu.h>\n")
		file.write("\n")

	for shader in shaders:
		compiled_shaders: dict[Target, ImmutableBuffer] = {}
		for target in Target:
			source = shader.sources[target]
			if source is None:
				continue

			compiled_bytes = source.compile(target, settings)
			if compiled_bytes is None:
				continue

			compiled_bytes = padded_bytes(compiled_bytes, 4)

			var_name = shader.name + target_suffix(target)

			c_source = c_source_from_bytes(compiled_bytes, var_name)

			output_c_file.write(c_source)
			output_c_file.write("\n")

			output_h_file.write(f"extern const u32 {var_name}[];\n")

			compiled_shaders[target] = ImmutableBuffer(var_name, len(compiled_bytes))

		output_h_file.write(f"static const Gpu_Shader_Blob_Desc {shader.name}_blob = {{\n")
		for target, buffer in compiled_shaders.items():
			target_member = target_blob_member(target)
			buffer_declaration = buffer.declaration()
			output_h_file.write(f"\t{target_member} = {buffer_declaration},\n")
		output_h_file.write("};\n")
		output_h_file.write("\n")

	output_h_file.close()
	output_c_file.close()

# def compile_shaders(output_h_path: str, output_c_path: str, shaders: Iterable[Shader]):
# 	parent_folder_path = str(Path(str(shader_path)).parent)
# 	compiled_spv_path = parent_folder_path + "/" + c_name + ".spv"
# 	compiled_metal_lib_path = parent_folder_path + "/" + c_name + ".metallib"
# 	h_path = parent_folder_path + "/" + c_name + ".h"
# 	c_path = parent_folder_path + "/" + c_name + ".c"

# 	args = [
# 		"slangc",
# 		str(shader_path),
# 		"-target",
# 		"spirv",
# 		"-entry",
# 		entry_point,
# 		"-o",
# 		compiled_spv_path,
# 		"-fvk-use-entrypoint-name",
# 	]
# 	for include_path in include_paths:
# 		args.append("-I")
# 		args.append(include_path)

# 	subprocess.run(args)

# 	is_macos = platform.system() == "Darwin"

# 	if is_macos:
# 		args = [
# 			"slangc",
# 			str(shader_path),
# 			"-target",
# 			"metallib",
# 			"-profile",
# 			"metallib_2_3",
# 			"-entry",
# 			entry_point,
# 			"-o",
# 			compiled_metal_lib_path,
# 		]
# 		for include_path in include_paths:
# 			args.append("-I")
# 			args.append(include_path)

# 		subprocess.run(args)

# 	c = open(c_path, "w")
# 	c.write("// machine generated, do not edit.\n")
# 	c.write('#include <stanlib/core.h>\n')
# 	c.write("\n")

# 	spv_source_variable = c_name + "_spv"
# 	spv_source_len = dump_bytes_to_file(c, compiled_spv_path, spv_source_variable)

# 	metal_source_variable = c_name + "_metal"
# 	if is_macos:
# 		metal_source_len = dump_bytes_to_file(
# 			c, compiled_metal_lib_path, metal_source_variable
# 		)

# 	c.close()

# 	blob_desc_variable = c_name + "_blob"

# 	h = open(h_path, "w")
# 	h.write("// machine generated, do not edit.\n")
# 	h.write("#include <stanlib/core.h>\n")
# 	h.write("#include <stanlib/gpu.h>\n")
# 	h.write("\n")
# 	h.write(f"extern const u32 {spv_source_variable}[];\n")
# 	h.write(f"extern const u32 {metal_source_variable}[];\n")
# 	h.write("\n")
# 	h.write(f"static const Gpu_Shader_Blob_Desc {blob_desc_variable} = {{\n")
# 	h.write("\t.spv = {\n")
# 	h.write(f"\t\t.data = {spv_source_variable},\n")
# 	h.write(f"\t\t.size = {spv_source_len},\n")
# 	h.write("\t},\n")
# 	if is_macos:
# 		h.write("\t.metallib = {\n")
# 		h.write(f"\t\t.data = {metal_source_variable},\n")
# 		h.write(f"\t\t.size = {metal_source_len},\n")
# 		h.write("\t},\n")
# 	h.write("};\n")
# 	h.write("\n")
# 	h.close()

# 	os.remove(compiled_spv_path)

# 	if is_macos:
# 		os.remove(compiled_metal_lib_path)
