find_package(Threads)

set(HEADERS
)

set(SOURCES
  pwng_client.cpp
)

add_executable(pwng-client ${LIB_NOISE_HEADERS} ${HEADERS} ${SOURCES})

target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/src/")
target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/install/include/")

target_link_libraries(pwng-client PRIVATE
    Threads::Threads
    ${LIBNOISE_LIBRARY_LOCAL}
)


set_property(TARGET pwng-client PROPERTY CXX_STANDARD 17)

install(TARGETS pwng-client DESTINATION bin)
