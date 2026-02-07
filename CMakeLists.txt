cmake_minimum_required(VERSION 3.6)

file(READ "include/ndtf/ndtf.h" NDTF_INCLUDE_FILE)
string(REGEX MATCH "#define[ ]+NDTF_VERSION_MAJOR[ ]+([0-9]+)" _ ${NDTF_INCLUDE_FILE})
set(NDTF_VERSION_MAJOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define[ ]+NDTF_VERSION_MINOR[ ]+([0-9]+)" _ ${NDTF_INCLUDE_FILE})
set(NDTF_VERSION_MINOR "${CMAKE_MATCH_1}")

set(NDTF_VERSION ${NDTF_VERSION_MAJOR}.${NDTF_VERSION_MINOR})

project(ndtf VERSION ${NDTF_VERSION} LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LIBDEFLATE_BUILD_GZIP OFF)
set(LIBDEFLATE_BUILD_SHARED_LIB OFF)
add_subdirectory("thirdparty/libdeflate")

file(GLOB SRC_FILES "src/*.c")

add_library(ndtf STATIC ${SRC_FILES})

target_include_directories(ndtf
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ndtf libdeflate::libdeflate_static)

target_compile_options(ndtf PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wno-error=implicit-function-declaration>)
