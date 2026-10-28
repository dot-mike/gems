add_library(commandos_ddraw SHARED
  ddraw.cpp
  ddraw.def
)

install(TARGETS commandos_ddraw
  RUNTIME DESTINATION .
  COMPONENT commandos_behind_enemy_lines
)

target_compile_options(commandos_ddraw PRIVATE
  -Wall
  -Wextra
  -pedantic
  -Werror
  -Wno-cast-function-type
)

target_include_directories(commandos_ddraw PUBLIC ${PROJECT_SOURCE_DIR})
target_compile_features(commandos_ddraw PRIVATE cxx_std_23)
target_compile_definitions(commandos_ddraw PRIVATE -DNOMINMAX)
target_link_libraries(commandos_ddraw PRIVATE stdc++exp)
target_link_options(commandos_ddraw PRIVATE "-Wl,--enable-stdcall-fixup")
set_target_properties(commandos_ddraw PROPERTIES PREFIX "" OUTPUT_NAME ddraw)
