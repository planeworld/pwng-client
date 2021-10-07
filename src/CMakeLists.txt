find_package(Threads)
find_package(RapidJSON)

set(HEADERS
  managers/json_manager.hpp
  managers/network_manager.hpp
  managers/ui_manager.hpp
  shaders/blur_shader_5x1.hpp
  shaders/main_display_shader.hpp
  systems/name_system.hpp
  systems/render_system.hpp
  avg_filter.hpp
  color_palette.hpp
  components.hpp
  message_handler.hpp
  network_message.hpp
  performance_timers.hpp
  pwng_client.hpp
  scale_unit.hpp
  shader_path.hpp
  sim_timer.hpp
  timer.hpp
)

set(SOURCES
  ${ImGui_INCLUDE_DIR}/imgui.cpp
  managers/json_manager.cpp
  managers/network_manager.cpp
  managers/ui_manager.cpp
  systems/render_system.cpp
  pwng_client.cpp
  sim_timer.cpp
)

add_executable(pwng-client ${LIB_NOISE_HEADERS} ${HEADERS} ${SOURCES})

target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/src/")
target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/src/managers")
target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/src/shaders")
target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/src/systems")
target_include_directories(pwng-client PRIVATE "${PROJECT_SOURCE_DIR}/install/include/")
target_include_directories(pwng-client PRIVATE ${MAGNUM_INCLUDE_DIR})
target_include_directories(pwng-client PRIVATE ${ImGui_INCLUDE_DIR})

target_link_libraries(pwng-client PRIVATE
  Corrade::Main
  Magnum::GlfwApplication
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
install(DIRECTORY "${PROJECT_SOURCE_DIR}/src/shaders/glsl/" DESTINATION "${PROJECT_SOURCE_DIR}/install/share/shaders/")
