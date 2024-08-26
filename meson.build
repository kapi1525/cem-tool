project(
    'cem-tool',
    'cpp', 'c',
    version: '1.0.0',
    license: 'MIT',
    default_options: ['warning_level=1', 'cpp_std=c++17', 'c_std=c11', 'default_library=static'],
    meson_version: '>=1.0',
)


cem_tool_args = []

# fun
# minizip depends on zlib to support file compression/decompression
meson.override_dependency('zlib', subproject('zlib').get_variable('zlib_dep'))

cem_tool_deps = [
    dependency('threads'),
    dependency('nlohmann_json'),
    dependency('minizip', fallback: 'minizip-ng'),
]


cem_tool_files = files(
    'src/entry.cpp',
    'src/cem_tool.cpp',
    'src/fusion_ext.cpp',
    'src/zip_archive.cpp',
    'src/string_helper.cpp',
)

cem_tool_inc = include_directories(
    'inc',
)


if host_machine.cpu_family() != 'x86'
    error('This tool requires to be 32bit to load extensions and extract info from them, current compiler is targeting: ' + host_machine.cpu() + '.')
    # TODO?
    # warning('This tool requires to be 32bit to load extensions and extract info from them, current compiler is targeting: ' + host_machine.cpu() + ', extension loading disabled.')
    # cem_tool_args += '-DNO_EXT_LOAD'
endif


cem_tool = executable(
    'cem-tool',
    cem_tool_files,
    cpp_args: cem_tool_args,
    dependencies: cem_tool_deps,
    include_directories: cem_tool_inc,
)



fs = import('fs')

# mmfs2.dll is required to open extension dlls.
# this dll was copied from fusion 295.10.
fs.copyfile('lib/mmfs2.dll', 'mmfs2.dll')