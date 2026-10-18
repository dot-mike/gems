add_library(out_of_the_park_baseball_6_wsock32 SHARED
  wsock32.cpp
  ${PROJECT_SOURCE_DIR}/core/dll_main/wsock32/wsock32.def
)

install(TARGETS out_of_the_park_baseball_6_wsock32
  RUNTIME DESTINATION .
  COMPONENT out_of_the_park_baseball_6
)

target_compile_options(out_of_the_park_baseball_6_wsock32 PRIVATE
  -Wall
  -Wextra
  -pedantic
  -Werror
  -Wconversion-null
  -Wmissing-declarations
  -Wno-error=missing-declarations
  -Woverlength-strings
  -Wpointer-arith
  -Wunused-local-typedefs
  -Wunused-result
  -Wvarargs
  -Wvla
  -Wwrite-strings
  -Wno-cast-function-type
  -Wno-missing-declarations
  -freflection
)

target_include_directories(out_of_the_park_baseball_6_wsock32 PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(out_of_the_park_baseball_6_wsock32 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(out_of_the_park_baseball_6_wsock32 PRIVATE cxx_std_26)
target_compile_definitions(out_of_the_park_baseball_6_wsock32 PRIVATE -DNOMINMAX)
target_link_libraries(out_of_the_park_baseball_6_wsock32 PRIVATE stdc++exp -lshlwapi)
target_link_options(out_of_the_park_baseball_6_wsock32 PRIVATE
  "-Wl,--enable-stdcall-fixup"
  "-Wl,--kill-at"
)
set_target_properties(out_of_the_park_baseball_6_wsock32 PROPERTIES PREFIX "" OUTPUT_NAME wsock32)
