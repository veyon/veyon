SET(VH_WINRC "${CMAKE_CURRENT_BINARY_DIR}/vnchooksrc.obj")
ADD_CUSTOM_COMMAND(OUTPUT ${VH_WINRC}
						COMMAND ${WINDRES}
							-I${CMAKE_CURRENT_SOURCE_DIR}
							-o${VH_WINRC}
							-i${ultravnc_DIR}/winvnc/vnchooks/vnchooks.rc)

set(vnchooks_SOURCES
	${ultravnc_DIR}/winvnc/vnchooks/VNCHooks.cpp
	${ultravnc_DIR}/winvnc/vnchooks/SharedData.cpp
)

add_library(vnchooks MODULE ${vnchooks_SOURCES} ${VH_WINRC})
set_source_files_properties(${vnchooks_SOURCES} PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)
target_compile_options(vnchooks PRIVATE ${VEYON_COMPILE_OPTIONS})
set_default_target_properties(vnchooks)
set_target_properties(vnchooks PROPERTIES PREFIX "")
set_target_properties(vnchooks PROPERTIES COMPILE_FLAGS "-Wno-write-strings -Wno-unused-variable -Wno-unknown-pragmas")
set_target_properties(vnchooks PROPERTIES LINK_FLAGS -Wl,-export-all-symbols)
target_link_libraries(vnchooks -ladvapi32)
