cmake_minimum_required(VERSION 3.4)

project(deflate C ASM)

set(CMAKE_C_STANDARD 99)

set(LIBDEFLATE_SOURCES
    libdeflate/lib/utils.c
    libdeflate/lib/adler32.c
    libdeflate/lib/crc32.c
    libdeflate/lib/x86/cpu_features.c
    libdeflate/lib/arm/cpu_features.c
    libdeflate/lib/deflate_decompress.c
    libdeflate/lib/deflate_compress.c
    libdeflate/lib/zlib_decompress.c
    libdeflate/lib/zlib_compress.c
    libdeflate/lib/gzip_decompress.c
    libdeflate/lib/gzip_compress.c
)

set(LIBDEFLATE_HEADERS
    libdeflate/common/compiler_msc.h
    libdeflate/common/compiler_gcc.h
    libdeflate/common/common_defs.h
    libdeflate/lib/bt_matchfinder.h
    libdeflate/lib/matchfinder_common.h
    libdeflate/lib/unaligned.h
    libdeflate/lib/gzip_constants.h
    libdeflate/lib/zlib_constants.h
    libdeflate/lib/crc32_vec_template.h
    libdeflate/lib/decompress_template.h
    libdeflate/lib/deflate_compress.h
    libdeflate/lib/crc32_table.h
    libdeflate/lib/x86/decompress_impl.h
    libdeflate/lib/x86/crc32_impl.h
    libdeflate/lib/x86/cpu_features.h
    libdeflate/lib/x86/adler32_impl.h
    libdeflate/lib/x86/matchfinder_impl.h
    libdeflate/lib/x86/crc32_pclmul_template.h
    libdeflate/lib/lib_common.h
    libdeflate/lib/hc_matchfinder.h
    libdeflate/lib/cpu_features_common.h
    libdeflate/lib/arm/crc32_impl.h
    libdeflate/lib/arm/cpu_features.h
    libdeflate/lib/arm/adler32_impl.h
    libdeflate/lib/arm/matchfinder_impl.h
    libdeflate/lib/deflate_constants.h
    libdeflate/lib/adler32_vec_template.h
)

add_library(libdeflate STATIC)

target_sources(libdeflate PRIVATE
    ${LIBDEFLATE_SOURCES}
    ${LIBDEFLATE_HEADERS}
)

target_include_directories(libdeflate PRIVATE
    ./libdeflate/
)
