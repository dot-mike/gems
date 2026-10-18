add_library(out_of_the_park_baseball_6_elicen40 SHARED
  elicen40.cpp
  ${PROJECT_SOURCE_DIR}/core/dll_main/elicen40/elicen40.def
)

install(TARGETS out_of_the_park_baseball_6_elicen40
  RUNTIME DESTINATION .
  COMPONENT out_of_the_park_baseball_6
)

target_compile_options(out_of_the_park_baseball_6_elicen40 PRIVATE
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

target_include_directories(out_of_the_park_baseball_6_elicen40 PRIVATE ${PROJECT_SOURCE_DIR})
target_compile_features(out_of_the_park_baseball_6_elicen40 PRIVATE cxx_std_26)
target_compile_definitions(out_of_the_park_baseball_6_elicen40 PRIVATE NOMINMAX)
set_target_properties(out_of_the_park_baseball_6_elicen40 PROPERTIES PREFIX "" OUTPUT_NAME elicen40)

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

add_library(out_of_the_park_baseball_6_ws2_32 SHARED
  ws2_32.cpp
  ${PROJECT_SOURCE_DIR}/core/dll_main/ws2_32/ws2_32.def
)

install(TARGETS out_of_the_park_baseball_6_ws2_32
  RUNTIME DESTINATION .
  COMPONENT out_of_the_park_baseball_6
)

target_compile_options(out_of_the_park_baseball_6_ws2_32 PRIVATE
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

target_include_directories(out_of_the_park_baseball_6_ws2_32 PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(out_of_the_park_baseball_6_ws2_32 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(out_of_the_park_baseball_6_ws2_32 PRIVATE cxx_std_26)
target_compile_definitions(out_of_the_park_baseball_6_ws2_32 PRIVATE -DNOMINMAX)
target_link_libraries(out_of_the_park_baseball_6_ws2_32 PRIVATE stdc++exp -lshlwapi)
target_link_options(out_of_the_park_baseball_6_ws2_32 PRIVATE
  "-Wl,--enable-stdcall-fixup"
  "-Wl,--kill-at"
)
set_target_properties(out_of_the_park_baseball_6_ws2_32 PROPERTIES PREFIX "" OUTPUT_NAME ws2_32)


