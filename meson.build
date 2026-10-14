project('mauikit-archiver', 'cpp', version: '4.0.5', meson_version: '>=1.0.0', default_options: ['cpp_std=c++17', 'buildtype=release'])

cpp_compiler = meson.get_compiler('cpp')
if cpp_compiler.get_id() == 'gcc' and cpp_compiler.has_argument('-Wno-sfinae-incomplete')
    add_project_arguments('-Wno-sfinae-incomplete', language: 'cpp')
endif
qt6 = import('qt6')
qt = dependency('qt6', version: '>=6.4', modules: ['Core', 'Quick', 'Qml', 'Sql', 'Network'])
kf6_include_prefix = get_option('prefix') / 'include' / 'KF6'
kf6_core = declare_dependency(
    include_directories: include_directories(kf6_include_prefix / 'KCoreAddons'),
    dependencies: [cpp_compiler.find_library('KF6CoreAddons')],
)
kf6_i18n = declare_dependency(
    include_directories: include_directories(kf6_include_prefix / 'KI18n'),
    dependencies: [cpp_compiler.find_library('KF6I18n')],
)
kf6_archive = declare_dependency(
    include_directories: include_directories(kf6_include_prefix / 'KArchive'),
    dependencies: [cpp_compiler.find_library('KF6Archive')],
)
mauikit = dependency('MauiKit4', version: '>=4.0.5')
filebrowsing = dependency('MauiKitFileBrowsing4', version: '>=4.0.5')
cpp_args = host_machine.cpu_family() == 'x86_64' ? ['-march=x86-64-v3'] : []
archiver_export_header = configure_file(
    input: 'meson-content.in',
    output: 'archiver_export.h',
    configuration: {'CONTENT': ('''#pragma once
#define ARCHIVER_EXPORT __attribute__((visibility("default")))
''')}
)
archiver_version_header = configure_file(
    input: 'meson-content.in',
    output: 'archiver_version.h',
    configuration: {'CONTENT': '#pragma once\n#define Archiver_VERSION_STRING "' + meson.project_version() + '"\n#define GIT_BRANCH "Stable"\n#define GIT_COMMIT_HASH "' + meson.project_version() + '"\n'},
)
archiver_moc = qt6.preprocess(moc_headers: ['src/code/compressedfile.h', 'src/code/temporaryfile.h'], dependencies: [qt])
lib = shared_library('MauiKitArchiver4', files('src/code/moduleinfo.cpp', 'src/code/compressedfile.cpp', 'src/code/temporaryfile.cpp') + archiver_moc + [archiver_export_header, archiver_version_header], dependencies: [qt, kf6_core, kf6_i18n, kf6_archive, mauikit, filebrowsing], cpp_args: cpp_args, version: '4.0.5', soversion: '4', install: true)
plugin_sources = qt6.preprocess(moc_headers: ['src/code/archiver_plugin.h'], dependencies: [qt])
plugin = shared_module('MauiKitArchiver4plugin', ['src/code/archiver_plugin.cpp'] + plugin_sources, link_with: lib, dependencies: [qt, kf6_core, kf6_i18n, kf6_archive, mauikit, filebrowsing], install: true, install_dir: get_option('libdir') / 'qt6/qml/org/mauikit/archiver')
qml_dir = get_option('libdir') / 'qt6/qml/org/mauikit/archiver'
install_data(configure_file(
    input: 'meson-content.in',
    output: 'qmldir',
    configuration: {'CONTENT': ('module org.mauikit.archiver\nplugin MauiKitArchiver4plugin\n')}
), install_dir: qml_dir)
install_subdir('src/controls', install_dir: qml_dir, strip_directory: true)
install_headers(files('src/code/moduleinfo.h') + [archiver_export_header, archiver_version_header], subdir: 'MauiKit4/Archiver')
pkg = import('pkgconfig')
pkg.generate(lib, name: 'MauiKit Archiver 4', filebase: 'MauiKitArchiver4', version: meson.project_version(), description: 'MauiKit archiver library', requires: 'MauiKit4 MauiKitFileBrowsing4')

cmake_targets = configure_file(
    input: 'meson-content.in',
    output: 'MauiKitArchiver4Targets.cmake',
    configuration: {'CONTENT': (('''get_filename_component(_MAUIKIT_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
if(NOT TARGET MauiKitArchiver4)
  add_library(MauiKitArchiver4 SHARED IMPORTED)
  set_target_properties(MauiKitArchiver4 PROPERTIES
    IMPORTED_LOCATION "${_MAUIKIT_PREFIX}/@LIBDIR@/libMauiKitArchiver4.so.4.0.5"
    INTERFACE_INCLUDE_DIRECTORIES "${_MAUIKIT_PREFIX}/include/MauiKit4/Archiver")
endif()
if(NOT TARGET MauiKit4::Archiver)
  add_library(MauiKit4::Archiver ALIAS MauiKitArchiver4)
endif()
if(NOT TARGET MauiKit::Archiver)
  add_library(MauiKit::Archiver ALIAS MauiKitArchiver4)
endif()
''')).replace('@LIBDIR@', (get_option('libdir')))}
)
cmake_config = configure_file(
    input: 'meson-content.in',
    output: 'MauiKitArchiver4Config.cmake',
    configuration: {'CONTENT': ('''include(CMakeFindDependencyMacro)
find_dependency(Qt6 6.4 COMPONENTS Core)
include("${CMAKE_CURRENT_LIST_DIR}/MauiKitArchiver4Targets.cmake")
set(MauiKitArchiver4_FOUND TRUE)
''')}
)
cmake_version = configure_file(
    input: 'meson-content.in',
    output: 'MauiKitArchiver4ConfigVersion.cmake',
    configuration: {'CONTENT': (('''set(PACKAGE_VERSION "@VERSION@")
if(PACKAGE_FIND_VERSION VERSION_LESS_EQUAL PACKAGE_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
endif()
if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
  set(PACKAGE_VERSION_EXACT TRUE)
endif()
''')).replace('@VERSION@', (meson.project_version()))}
)
install_data([cmake_targets, cmake_config, cmake_version], install_dir: get_option('libdir') / 'cmake' / 'MauiKitArchiver4')
