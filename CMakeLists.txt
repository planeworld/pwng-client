cmake_minimum_required(VERSION 3.1)

project(pwng-client)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    add_compile_options(-mtune=native -Wall -pedantic)
else()
    add_compile_options(-mtune=native -Wall -Wextra -pedantic)
endif()

set(LIBNOISE_LIBRARY_LOCAL
        ${PROJECT_SOURCE_DIR}/3rdparty/libnoise2d/build/src/libnoise2d.so
)
set(LIB_NOISE_HEADERS
        ${PROJECT_SOURCE_DIR}/3rdparty/libnoise2d/src/noise.h
)

add_subdirectory(./src)
