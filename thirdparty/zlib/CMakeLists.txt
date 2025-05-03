cmake_minimum_required(VERSION 2.4.4...3.15.0)

# i have no idea what i am doing

project(zlib C)

set(VERSION "1.3.1")

set(ZLIB_PUBLIC_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/zconf.h
    zlib.h
)
set(ZLIB_PRIVATE_HDRS
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zutil.h
)
set(ZLIB_SRCS
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
)

add_library(zlib STATIC ${ZLIB_SRCS})

target_include_directories(zlib
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
    target_compile_definitions(zlib
        PRIVATE
            _CRT_SECURE_NO_DEPRECATE
            _CRT_NONSTDC_NO_DEPRECATE
    )
endif()

install(FILES zlib.h zconf.h
    DESTINATION include
)
