cmake_minimum_required(VERSION 3.22)

project(
  gems
  VERSION "2026.9.29.0"
  DESCRIPTION "collection of game fixes"
  LANGUAGES C CXX
)

include(GenerateExportHeader)

set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(core)
add_subdirectory(games)

set(CPACK_PACKAGE_NAME "gems")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_GROUPING IGNORE)
set(CPACK_COMPONENTS_ALL
  odyssey_the_search_for_ulyssess
  out_of_the_park_baseball_65
  out_of_the_park_baseball_6
)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_SOURCE_IGNORE_FILES "build")
include(CPack)
