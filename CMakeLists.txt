cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(
  rxwayland
  VERSION 0.0.0
  DESCRIPTION "The reactive extensions for Wayland protocol"
  HOMEPAGE_URL "https://github.com/guhwanbae/rxwayland")

include(FindPkgConfig)
pkg_check_modules(WAYLAND_SERVER wayland-server REQUIRED)
find_package(fmt)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME}
                           INTERFACE include ${WAYLAND_SERVER_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${WAYLAND_SERVER_LDFLAGS}
                                                fmt::fmt)

# TODO: Should I release this package with rxwaylandConfig.cmake?
include(GNUInstallDirs)
configure_file(pkgconfig/rxwayland.pc.in rxwayland.pc @ONLY)
install(DIRECTORY include/rxwayland DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/rxwayland.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
