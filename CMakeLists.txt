cmake_minimum_required(VERSION 3.10)
project(ndtf C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(thirdparty/zlib)

file(GLOB SRC_FILES "src/*.c")

add_library(ndtf STATIC ${SRC_FILES})

target_include_directories(ndtf
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ndtf PRIVATE zlib)
