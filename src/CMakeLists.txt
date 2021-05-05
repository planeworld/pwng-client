find_package(Threads)
find_package(RapidJSON)

set(HEADERS
  managers/network_manager.hpp
  managers/ui_manager.hpp
  systems/name_system.hpp
  avg_filter.hpp
  color_palette.hpp
  components.hpp
  message_handler.hpp
  performance_timers.hpp
  pwng_client.hpp
  scale_unit.hpp
  timer.hpp
)

set(SOURCES
  ${ImGui_INCLUDE_DIR}/imgui.cpp
  managers/network_manager.cpp
  managers/ui_manager.cpp
  pwng_client.cpp
)

add_executable(pwng-client ${LIB_NOISE_HEADERS} ${HEADERS} ${SOURCES})

target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/src/")
target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/src/managers")
target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/src/systems")
target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/install/include/")
target_include_directories(pwng-client PRIVATE ${MAGNUM_INCLUDE_DIR})
target_include_directories(pwng-client PRIVATE ${ImGui_INCLUDE_DIR})

target_link_libraries(pwng-client PRIVATE
  Corrade::Main
  Magnum::Application
  Magnum::GL
  Magnum::Magnum
  Magnum::Primitives
  Magnum::Shaders
  MagnumIntegration::ImGui
  OpenGL::GL
  Threads::Threads
  ${LIBNOISE_LIBRARY_LOCAL}
)

set_property(TARGET pwng-client PROPERTY CXX_STANDARD 17)

install(TARGETS pwng-client DESTINATION bin)
